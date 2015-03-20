#include <CL\cl.h>
#include <CL/cl_ext.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

struct Image
{
	std::vector<char> pixel;
	int width, height;
};

Image LoadImage (const char* path)
{
	std::ifstream in (path, std::ios::binary);

	std::string s;
	in >> s;

	if (s != "P6") {
		exit (1);
	}

	// Skip comments
	for (;;) {
		getline (in, s);

		if (s.empty ()) {
			continue;
		}

		if (s [0] != '#') {
			break;
		}
	}

	std::stringstream str (s);
	int width, height, maxColor;
	str >> width >> height;
	in >> maxColor;

	if (maxColor != 255) {
		exit (1);
	}

	{
		// Skip until end of line
		std::string tmp;
		getline(in, tmp);
	}

	std::vector<char> data (width * height * 3);
	in.read (reinterpret_cast<char*> (data.data ()), data.size ());

	const Image img = { data, width, height };
	return img;
}

void SaveImage (const Image& img, const char* path)
{
	std::ofstream out (path, std::ios::binary);

	out << "P6\n";
	out << img.width << " " << img.height << "\n";
	out << "255\n";
	out.write (img.pixel.data (), img.pixel.size ());
}

Image RGBtoRGBA (const Image& input)
{
	Image result;
	result.width = input.width;
	result.height = input.height;

	for (std::size_t i = 0; i < input.pixel.size (); i += 3) {
		result.pixel.push_back (input.pixel [i + 0]);
		result.pixel.push_back (input.pixel [i + 1]);
		result.pixel.push_back (input.pixel [i + 2]);
		result.pixel.push_back (0);
	}

	return result;
}

Image RGBAtoRGB (const Image& input)
{
	Image result;
	result.width = input.width;
	result.height = input.height;

	for (std::size_t i = 0; i < input.pixel.size (); i += 4) {
		result.pixel.push_back (input.pixel [i + 0]);
		result.pixel.push_back (input.pixel [i + 1]);
		result.pixel.push_back (input.pixel [i + 2]);
	}

	return result;
}

void CheckError (cl_int error)
{
	if (error != CL_SUCCESS) {
		std::cerr << "OpenCL call failed with error " << error << std::endl;
		std::exit (1);
	}
}

void main(){

  cl_int error;
  cl_platform_id platform;
  error = clGetPlatformIDs(1, &platform, nullptr);

  cl_device_id device;
  error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, nullptr);

  cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
  cl_context ctx = clCreateContext( cps, 1, &device, nullptr, nullptr, &error);

  cl_command_queue q = clCreateCommandQueue(ctx, device, 0, &error);

  Image in_img = RGBtoRGBA( LoadImage("test.ppm") );
  Image out_img = in_img;
  const cl_image_format img_format = { CL_RGBA, CL_UNORM_INT8 };
  const cl_image_format filter_format = { CL_R, CL_FLOAT };
  cl_image_info info;

  const int filter_width = 5;
  float kernel_integral = 0.0f;
  const float filter[filter_width][filter_width] = {
    {0.2f, 0.4f, 0.6f, 0.4f, 0.2f},
    {0.4f, 0.6f, 0.8f, 0.6f, 0.4f},
    {0.5f, 0.8f, 1.0f, 0.8f, 0.5f},
    {0.4f, 0.6f, 0.8f, 0.6f, 0.4f},
    {0.2f, 0.4f, 0.6f, 0.4f, 0.2f}
  };
  for(int i = 0; i < filter_width; i++)
    for(int j = 0; j < filter_width; j++)
      kernel_integral += filter[i][j];

  cl_mem d_in_img = clCreateImage2D( ctx, CL_MEM_READ_ONLY, &img_format, in_img.width, in_img.height, 0, nullptr, &error );
  CheckError(error);
  cl_mem d_filter = clCreateBuffer( ctx, CL_MEM_READ_ONLY, filter_width * filter_width * sizeof(float), 0, &error );
  CheckError(error);
  cl_mem d_out_img = clCreateImage2D( ctx, CL_MEM_WRITE_ONLY, &img_format, in_img.width, in_img.height, 0, nullptr, &error );
  CheckError(error);

  CheckError(error);

  size_t img_origin[3] = { 0, 0, 0 };
  size_t img_region[3] = { in_img.width, in_img.height, 1 };
  clEnqueueWriteImage( q, d_in_img, CL_TRUE, img_origin, img_region, in_img.width * 4 * sizeof(char), in_img.height * in_img.width * 4 * sizeof(char),  const_cast<char*>(in_img.pixel.data()), 0, nullptr, nullptr );
  CheckError(error);
  
  clEnqueueWriteBuffer( q, d_filter, CL_TRUE, 0, filter_width * filter_width * sizeof(float), filter, 0, nullptr, nullptr);
  CheckError(error);

  cl_sampler sampler = clCreateSampler( ctx, CL_FALSE, CL_ADDRESS_NONE, CL_FILTER_NEAREST, &error );
  CheckError(error);

  // Reading and compiling the kernel
  std::ifstream sourceFile("convolution_kernel.cl");
  if(!sourceFile){
    std::cout << "Cannot read find the convolution_kernel.cl file\n";
    exit(0);
  }
  std::string source_str(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
  const char* source = source_str.c_str();

  cl_program prg = clCreateProgramWithSource( ctx, 1, &source, nullptr, &error );
  CheckError(error);

  error = clBuildProgram( prg, 0, nullptr, nullptr, nullptr, nullptr );
  CheckError(error);

  cl_kernel kernel = clCreateKernel( prg, "convolution", &error );
  CheckError(error);

  clSetKernelArg( kernel, 0, sizeof(cl_mem), &d_in_img );
  clSetKernelArg( kernel, 1, sizeof(cl_mem), &d_out_img );
  clSetKernelArg( kernel, 2, sizeof(cl_int), &in_img.width );
  clSetKernelArg( kernel, 3, sizeof(cl_int), &in_img.height );
  clSetKernelArg( kernel, 4, sizeof(cl_mem), &d_filter );
  clSetKernelArg( kernel, 5, sizeof(cl_int), &filter_width);
  clSetKernelArg( kernel, 6, sizeof(cl_float), &kernel_integral);
  clSetKernelArg( kernel, 7, sizeof(cl_sampler), &sampler);

  
  size_t localws[2] = { 16, 16 };
  size_t globalws[2] = { (in_img.width / 16 + 1) * 16, (in_img.height / 16 + 1) * 16 };
  error = clEnqueueNDRangeKernel( q, kernel, 2, nullptr, globalws,localws, 0, nullptr, nullptr );
  CheckError(error);

  error = clEnqueueReadImage( q, d_out_img, CL_TRUE, img_origin, img_region, in_img.width * 4 * sizeof(char), in_img.height * in_img.width * 4 * sizeof(char), const_cast<char*>(out_img.pixel.data()), 0, nullptr, nullptr );
  CheckError(error);

  SaveImage( RGBAtoRGB(out_img), "output.ppm" );

}