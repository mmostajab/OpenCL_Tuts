// This program implements a vector addition using OpenCL
// System Include
#include <stdio.h>
#include <stdlib.h>

// OpenCL include
#include <CL/cl.h>

const char* programsource =
	"__kernel 											\n"
	"void vecadd(	__global int* A,					\n"
	"				__global int* B,					\n"
	"				__global int* C) 					\n"
	"{													\n"
	"	// Get the work item unique id 					\n"
	"	int idx = get_global_id(0);						\n"
	"													\n"
	"	// Add the corresponding location of 			\n"
	"	// 'A' and 'B' and store the result in 'C'		\n"
	"	C[idx] = A[idx] + B[idx];						\n"
	"}													\n";

int main(int argv, char* argc[])
{
	int* A = NULL;
	int* B = NULL;
	int* C = NULL;

	const int elements = 2048;
	size_t datasize = elements * sizeof(int);

	A = (int*)malloc(datasize);
	B = (int*)malloc(datasize);
	C = (int*)malloc(datasize);

	for(int i = 0; i < elements; i++)
	{
		A[i] = i;
		B[i] = i;
	}

	// used to check the output of api call
	cl_int status;

	// retrieve the number of platforms
	cl_uint numPlatforms = 0;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if(status == CL_SUCCESS)
		printf("%i number of Platforms are detected.\n", numPlatforms);


	// allocate enough space for platforms
	cl_platform_id* platforms = NULL;
	platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
	status = clGetPlatformIDs(numPlatforms, platforms, NULL);
	if(status == CL_SUCCESS)
		printf("Successful Platform creation\n");

	// retrieve the number of devices
	cl_uint numDevices = 0;
	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);

	printf("%i Devices are detected\n", numDevices);
	if(status == CL_SUCCESS)
		printf("Successful Device num\n");

	// allocate enough space for each device
	cl_device_id* devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices, 0);
	if(status == CL_SUCCESS)
		printf("Successful Device creation\n");

	cl_context context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &status);
	if(status == CL_SUCCESS)
		printf("Successful Context creation\n");

	cl_command_queue cmdQueue = clCreateCommandQueue(context, devices[0], 0, &status);
	if(status == CL_SUCCESS)
		printf("Successful Command Queue creation\n");

	cl_mem bufA, bufB, bufC;
	bufA = clCreateBuffer(context, CL_MEM_READ_ONLY,  datasize, NULL, &status);
	bufB = clCreateBuffer(context, CL_MEM_READ_ONLY,  datasize, NULL, &status);
	bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &status);

	status = clEnqueueWriteBuffer(cmdQueue, bufA, CL_FALSE, 0, datasize, A, 0, NULL, NULL);
	status = clEnqueueWriteBuffer(cmdQueue, bufB, CL_FALSE, 0, datasize, B, 0, NULL, NULL);

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&programsource, NULL, &status);
	if(status == CL_SUCCESS)
		printf("Successful Program Creation\n");
	status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
	if(status == CL_SUCCESS)
		printf("Successful Program Built\n");

	cl_kernel kernel = clCreateKernel(program, "vecadd", &status);
	if(status == CL_SUCCESS)
		printf("Successful kernel creation\n");

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);

	// execute the kernel for execution
	size_t globalWorkSize[1] = { elements };
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, 0, globalWorkSize, NULL, 0, NULL, NULL);

	clEnqueueReadBuffer(cmdQueue, bufC, CL_TRUE, 0, datasize, C, 0, NULL, NULL);

	int result = 1;
	for(int i = 0; i < elements; i++)
	{
		// printf("The value = %i \n", C[i]);
		if(C[i] != i + i)
		{
			result = 0;
			break;
		}
	}

	if(result == 1)
		printf("output is correct.\n");
	else
		printf("output is incorrect.\n");

	// Free OpenCL resources
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(bufA);
	clReleaseMemObject(bufB);
	clReleaseMemObject(bufC);
	clReleaseContext(context);

	// Free host resources
	free(A);
	free(B);
	free(C);
	free(platforms);
	free(devices);

	return 0;
}
