#include <CL/cl.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

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

//#define USE_IMAGE

void main(){

  cl_platform_id platform[2];
  cl_device_id device;
  
  std::string device_name;
  size_t device_name_len;

  clGetPlatformIDs(2, platform, NULL);
  clGetDeviceIDs(platform[1], CL_DEVICE_TYPE_ALL, 1, &device, NULL);
  
  clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &device_name_len);
  device_name.resize( device_name_len );
  clGetDeviceInfo(device, CL_DEVICE_NAME, device_name_len, const_cast<char*>(device_name.data()), NULL);
  std::cout << "Device: " << device_name << std::endl;
  
  cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM, (cl_context_properties)(platform[1]), 0
  };
  cl_context ctx = clCreateContext(cps, 1, &device, nullptr, nullptr, nullptr);
  cl_command_queue q = clCreateCommandQueue(ctx, device, 0, nullptr);

  // loading the test image
  cl_int cinErr;

#ifndef USE_IMAGE

  Image in_img = LoadImage("test.ppm");
  Image out_img = in_img;
  cl_mem d_in_img  = clCreateBuffer(ctx, CL_MEM_READ_ONLY, 3 * in_img.width * in_img.height * sizeof(char), nullptr, &cinErr);
  cl_mem d_out_img = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 3 * out_img.width * out_img.height * sizeof(char), nullptr, &cinErr);
  cinErr = clEnqueueWriteBuffer(q, d_in_img, CL_TRUE, 0, 3 * in_img.width * in_img.height * sizeof(char), &(in_img.pixel[0]), 0, nullptr, nullptr);

#else

  Image in_img = RGBtoRGBA( LoadImage("test.ppm") );
  Image out_img = in_img;
  const cl_image_format format = { CL_RGBA, CL_UNORM_INT8 };
  cl_mem d_in_img = clCreateImage2D( ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &format, in_img.width, in_img.height, 0, const_cast<char*>(in_img.pixel.data()), &cinErr );
  cl_mem d_out_img = clCreateImage2D( ctx, CL_MEM_WRITE_ONLY, &format, in_img.width, in_img.height, 0, nullptr, &cinErr );

#endif

  CheckError( cinErr );

  // Reading and compiling the kernel
  std::ifstream sourceFile("rot_kernel.cl");
  if(!sourceFile){
    std::cout << "Cannot read find the mat_multiply_kernel.cl file\n";
    exit(0);
  }
  std::string source_str(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
  const char* source = source_str.c_str();

  cl_program prg = clCreateProgramWithSource( ctx, 1, &source, nullptr, nullptr);

  cl_int cierr_num = clBuildProgram(prg, 0, nullptr, nullptr, nullptr, nullptr);
  cl_kernel kernel = clCreateKernel(prg, "rotate_img", nullptr);

  const float thetha = 3.14 / 4;
  const cl_float sinThetha = sin( thetha );
  const cl_float cosThetha = cos( thetha );
  cierr_num = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_out_img);
  cierr_num = clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_in_img);
  cierr_num = clSetKernelArg(kernel, 2, sizeof(cl_int), &(in_img.width));
  cierr_num = clSetKernelArg(kernel, 3, sizeof(cl_int), &(in_img.height));
  cierr_num = clSetKernelArg(kernel, 4, sizeof(cl_float), &sinThetha);
  cierr_num = clSetKernelArg(kernel, 5, sizeof(cl_float), &cosThetha);

  size_t localws[2] = { 16, 16 };
  size_t worldws[2] = { (in_img.width / localws[0] + 1) * 16, (in_img.height / localws[1] + 1) * 16 };
  clEnqueueNDRangeKernel(q, kernel, 2, NULL, worldws, localws, 0, nullptr, nullptr);

#ifndef USE_IMAGE
  cinErr = clEnqueueReadBuffer(q, d_out_img, CL_TRUE, 0, 3 * out_img.width * out_img.height * sizeof(char), (void*)&(out_img.pixel[0]), 0, nullptr, nullptr);
  SaveImage( out_img , "output.ppm");
#else
  std::size_t origin[3] = { 0 };
  std::size_t region[3] = { out_img.width, out_img.height, 1 };
  cinErr = clEnqueueReadImage(q, d_out_img, CL_TRUE, origin, region, 0, 0, out_img.pixel.data(), 0, nullptr, nullptr);
  SaveImage(RGBAtoRGB( out_img ), "output.ppm");
#endif
}