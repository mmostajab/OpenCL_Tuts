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
  error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, &device, nullptr);

  cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
  cl_context ctx = clCreateContext( cps, 1, &device, nullptr, nullptr, &error);

  cl_command_queue q = clCreateCommandQueue(ctx, device, 0, &error);

  Image in_img = RGBtoRGBA( LoadImage("test.ppm") );
  Image out_img = in_img;
  const cl_image_format format = { CL_RGBA, CL_UNORM_INT8 };
  cl_image_info info;

  cl_mem d_in_img = clCreateImage2D( ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &format, in_img.width, in_img.height, 0, const_cast<char*>(in_img.pixel.data()), &cinErr );
  cl_mem d_out_img = clCreateImage2D( ctx, CL_MEM_WRITE_ONLY, &format, in_img.width, in_img.height, 0, nullptr, &cinErr );

  cl_image_desc
}