#define __NO_STD_VECTOR		// Use cl::vector instead of STL version
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>

int main(int argv, char* argc[])
{
	const int N_ELEMENTS = 1024;
	int* A = new int[N_ELEMENTS];
	int* B = new int[N_ELEMENTS];
	int* C = new int[N_ELEMENTS];

	for(int i = 0; i < N_ELEMENTS; i++)
	{
		A[i] = i;
		B[i] = i;
	}

	try
	{
		cl::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		cl::vector<cl::Device> devices;
		platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);

		cl::Context context(devices);
		cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);

		cl::Buffer bufferA = cl::Buffer(context, CL_MEM_READ_ONLY,  N_ELEMENTS * sizeof(int));
		cl::Buffer bufferB = cl::Buffer(context, CL_MEM_READ_ONLY,  N_ELEMENTS * sizeof(int));
		cl::Buffer bufferC = cl::Buffer(context, CL_MEM_WRITE_ONLY, N_ELEMENTS * sizeof(int));

		queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, N_ELEMENTS * sizeof(int), A);
		queue.enqueueWriteBuffer(bufferB, CL_TRUE, 0, N_ELEMENTS * sizeof(int), B);

		std::ifstream sourceFile("vector_add_kernel.cl");
		if(!sourceFile)
		{
			std::cout << "The cl file does not exist!\n";
			return 0;
		}
		std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
		cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));

		cl::Program program = cl::Program(context, source);
		program.build(devices);

		cl::Kernel vecadd_kernel(program, "vecAdd");

		vecadd_kernel.setArg(0, bufferA);
		vecadd_kernel.setArg(1, bufferB);
		vecadd_kernel.setArg(2, bufferC);

		// Execute the kernel
		cl::NDRange global(N_ELEMENTS);
		cl::NDRange local(256);

		queue.enqueueNDRangeKernel(vecadd_kernel, cl::NullRange, global, local);

		queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, N_ELEMENTS * sizeof(int), C);

		bool result = true;
		for(int i = 0; i < N_ELEMENTS; i++)
		{
			//printf("The value = %i \n", C[i]);
			if(C[i] != A[i] + B[i])
			{
				result = false;
				break;
			}
		}

		if(result == true)
			std::cout << "output is correct.\n";
		else
			std::cout << "output is incorrect.\n";
	}
	catch(cl::Error error)
	{
		std::cout << error.what() << "(" << error.err() << ")" << std::endl;
	}

	return 0;
}