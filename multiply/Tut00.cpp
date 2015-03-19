#define C_BINDING
//#define CPP_BINDING

#ifdef C_BINDING
# include <CL\cl.h>
#else
# ifdef CPP_BINDING
#define __CL_ENABLE_EXCEPTIONS
#define __NO_STD_VECTOR
#   include <CL/cl.hpp>
# endif
#endif

#include <iostream>
#include <fstream>
#include <string>


int main(void)
{
#ifdef C_BINDING
  cl_int ciErrNum;

  // Use the first platform
  cl_platform_id platform;
  ciErrNum = clGetPlatformIDs(1, &platform, NULL);

  // Use the first device
  cl_device_id device;
  ciErrNum = clGetDeviceIDs(
    platform, 
    CL_DEVICE_TYPE_ALL,
    1,
    &device,
    NULL);

  cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM,
    (cl_context_properties)platform,
    0
  };

  // Create the context
  cl_context ctx = clCreateContext( cps, 1, &device, NULL, NULL, &ciErrNum );

  // Create the command queue
  cl_command_queue myqueue = clCreateCommandQueue( 
    ctx, 
    device,
    0,
    &ciErrNum);
#else 
# ifdef CPP_BINDING
  cl::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  //create a context with first platform
  cl_context_properties cps[3] = {
    CL_CONTEXT_PLATFORM,
    (cl_context_properties)(platforms[0])(),
    0
  };

  // create the context with first platform
  cl::Context context(CL_DEVICE_TYPE_ALL, cps);

  // get the device list from the context
  cl::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

  // create command queue on the first device
  cl::CommandQueue queue = cl::CommandQueue(context, devices[0], 0);

# endif
#endif



  const int arr_size = 256;
  float A[arr_size * arr_size], B[arr_size * arr_size], C[arr_size * arr_size];

  for(int i = 0; i < arr_size; i++){
    for(int j = 0; j < arr_size; j++){
      if(i == j){
        A[i*arr_size+j] = 1;
        B[i*arr_size+j] = 1;
      }
      else{
        A[i*arr_size+j] = 0;
        B[i*arr_size+j] = 0;
      }
    }
  }

#ifdef C_BINDING
  // Allocate space for Matrix A on the device
  cl_mem bufferA = clCreateBuffer( ctx, CL_MEM_READ_ONLY, arr_size * arr_size * sizeof(float), NULL, &ciErrNum );

  // Copy Matrix A to the device
  ciErrNum = clEnqueueWriteBuffer( myqueue, bufferA, CL_TRUE, 0, arr_size * arr_size * sizeof(float), A, 0, NULL, NULL );

  // Allocate space for Matrix B on the device
  cl_mem bufferB = clCreateBuffer( ctx, CL_MEM_READ_ONLY, arr_size * arr_size * sizeof(float), NULL, &ciErrNum );

  // Copy Matrix A to the device
  ciErrNum = clEnqueueWriteBuffer( myqueue, bufferB, CL_TRUE, 0, arr_size * arr_size * sizeof(float), B, 0, NULL, NULL );

  // Allocate space for Matrix C on the device
  cl_mem bufferC = clCreateBuffer( ctx, CL_MEM_WRITE_ONLY, arr_size * arr_size * sizeof(float), NULL, &ciErrNum );

#else
# ifdef CPP_BINDING

  // Create buffers for the input and output data
  cl::Buffer bufferA = cl::Buffer(context, CL_MEM_READ_ONLY, arr_size*arr_size*sizeof(float));
  cl::Buffer bufferB = cl::Buffer(context, CL_MEM_READ_ONLY, arr_size*arr_size*sizeof(float));
  cl::Buffer bufferC = cl::Buffer(context, CL_MEM_READ_ONLY, arr_size*arr_size*sizeof(float));
  queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, arr_size*arr_size*sizeof(float), A);
  queue.enqueueWriteBuffer(bufferB, CL_TRUE, 0, arr_size*arr_size*sizeof(float), B);

# endif
#endif

  // Reading and compiling the kernel
  std::ifstream sourceFile("mat_multiply_kernel.cl");
  if(!sourceFile){
    std::cout << "Cannot read find the mat_multiply_kernel.cl file\n";
    exit(0);
  }
  std::string source_str(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
  

#ifdef C_BINDING
  const char* source = source_str.c_str();

  // Creating program from the source
  cl_program myprogram = clCreateProgramWithSource(ctx, 1, &source, NULL, &ciErrNum);

  // Compile the program
  ciErrNum = clBuildProgram(myprogram, 0, NULL, NULL, NULL, NULL);

  // Create the kernel
  cl_kernel kernel  = clCreateKernel( myprogram, "simplyMultiply", &ciErrNum );

  // Running the porgram
  clSetKernelArg( kernel, 0, sizeof(cl_mem), (void*)&bufferC );
  clSetKernelArg( kernel, 1, sizeof(int), (void*)&arr_size   );
  clSetKernelArg( kernel, 2, sizeof(int), (void*)&arr_size   );
  clSetKernelArg( kernel, 3, sizeof(int), (void*)&arr_size   );
  clSetKernelArg( kernel, 4, sizeof(int), (void*)&arr_size   );
  clSetKernelArg( kernel, 5, sizeof(cl_mem), (void*)&bufferA );
  clSetKernelArg( kernel, 6, sizeof(cl_mem), (void*)&bufferB );

  size_t localws[2] = {16, 16};
  size_t globalws[2] = {arr_size, arr_size};

  ciErrNum = clEnqueueNDRangeKernel( myqueue, kernel, 2, NULL, globalws, localws, 0, NULL, NULL );

  ciErrNum = clEnqueueReadBuffer( myqueue, bufferC, CL_TRUE, 0, arr_size * arr_size * sizeof(float), (void*)C, 0, NULL, NULL );

#else
# ifdef CPP_BINDING

  cl::Program::Sources source(1, std::make_pair(source_str.c_str(), source_str.length()+1));
  cl::Program program(context, source);
  cl::Kernel kernel(program, "simplyMultiply");

  // Running the program
  kernel.setArg(0, bufferC);
  kernel.setArg(1, arr_size);
  kernel.setArg(2, arr_size);
  kernel.setArg(3, arr_size);
  kernel.setArg(4, arr_size);
  kernel.setArg(5, bufferA);
  kernel.setArg(6, bufferB);

  cl::NDRange globalws(arr_size, arr_size);
  cl::NDRange localws(16, 16);

  queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalws, localws);

  queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, arr_size * arr_size * sizeof(float), C);

# endif

#endif

  bool result_true = true;
  for(int i = 0; i < arr_size; i++)
    for(int j = 0; j < arr_size; j++){
      if(i == j && C[i * arr_size + j] != 1){
        result_true = false;
        break;
      }
      if(i != j && C[i * arr_size + j] != 0){
        result_true = false;
        break;
      }
    }

    if(result_true)
      std::cout << "Result is correct!\n";
    else
      std::cout << "Result is incorrect!\n";
}