#define _CRT_SECURE_NO_WARNINGS
#define PROGRAM_FILE "matvec.cl"
#define KERNEL_FUNC "matvec_mult"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef MAC
#include <OpenCL/cl.h>
#else  
#include <CL/cl.h>
#endif

#include <math.h>
#include <float.h>
#include <CMat.h>

#define M 4
#define N 4

inline bool iszero(double x){ return fabs(x)<DBL_EPSILON*10;}
inline bool iszero(float x){ return fabs(x)<FLT_EPSILON*10;}
inline bool iszero(int x) {return x==0;}

template<typename T>
inline T roundzero(T x) { return iszero(x)?0.0:x;}

template<typename T>
inline T roundzero(T x, double eps) { return fabs(x)<FLT_EPSILON?0.0:x;}

template <typename T>
inline T TMAX(const T& a, const T& b) {
    return a>b? a:b;
}

int main() {

   /* Host/device data structures */
   cl_platform_id platform;
   cl_device_id device;
   cl_context context;
   cl_command_queue queue;
   cl_int i,j, err;

   /* Program/kernel data structures */
   cl_program program;
   FILE *program_handle;
   char *program_buffer, *program_log;
   size_t program_size, log_size;
   cl_kernel kernel;
   
   /* Data and buffers */
   float mat_[M*N], vec_[M], result_[M];
   float correct_[M];
   CMat<float> mat(mat_, M, N), vec(vec_, M), result(result_, M), correct(correct_, M);
   cl_mem mat_buff, vec_buff, res_buff;
   size_t work_units_per_kernel;

   /* Initialize data to be processed by the kernel */
   for(i=0; i<M; i++) {
       for(j=0; j<N; ++j) {
           mat(i,j) = (i +j)* 2.0f;
       }
       vec(i) = i*3.0;
   } 
   for(i=0; i<M; i++) {
       correct(i) = 0;
       for(int j=0; j<N; ++j) {
           correct(i) += mat(i,j) * vec(j);
       }
   }

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't find any platforms");
      exit(1);
   }

   /* Access a device */
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, 
         &device, NULL);
   if(err < 0) {
      perror("Couldn't find any devices");
      exit(1);
   }

   /* Create the context */
   context = clCreateContext(NULL, 1, &device, NULL, 
         NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);   
   }

   /* Read program file and place content into buffer */
   program_handle = fopen(PROGRAM_FILE, "r");
   if(program_handle == NULL) {
      perror("Couldn't find the program file");
      exit(1);   
   }
   fseek(program_handle, 0, SEEK_END);
   program_size = ftell(program_handle);
   rewind(program_handle);
   program_buffer = (char*)malloc(program_size + 1);
   program_buffer[program_size] = '\0';
   fread(program_buffer, sizeof(char), program_size, program_handle);
   fclose(program_handle);

   /* Create program from file */
   program = clCreateProgramWithSource(context, 1, 
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      exit(1);   
   }
   free(program_buffer);

   /* Build program */
   err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   if(err < 0) {

      /* Find size of log and print to std output */
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
      program_log = (char*) malloc(log_size + 1);
      program_log[log_size] = '\0';
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
      printf("%s\n", program_log);
      free(program_log);
      exit(1);
   }

   /* Create kernel for the mat_vec_mult function */
   kernel = clCreateKernel(program, KERNEL_FUNC, &err);
   if(err < 0) {
      perror("Couldn't create the kernel");
      exit(1);   
   }

   /* Create CL buffers to hold input and output data */
   mat_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | 
                             CL_MEM_COPY_HOST_PTR, sizeof(float)*M*N, &mat[0], &err);
   if(err < 0) {
      perror("Couldn't create a buffer object");
      exit(1);   
   }      
   vec_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | 
      CL_MEM_COPY_HOST_PTR, sizeof(float)*M, &vec[0], NULL);
   res_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
      sizeof(float)*M, NULL, NULL);

   /* Create kernel arguments from the CL buffers */
   err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mat_buff);
   if(err < 0) {
      perror("Couldn't set the kernel argument");
      exit(1);   
   }         
   clSetKernelArg(kernel, 1, sizeof(cl_mem), &vec_buff);
   clSetKernelArg(kernel, 2, sizeof(cl_mem), &res_buff);

   /* Create a CL command queue for the device*/
   queue = clCreateCommandQueue(context, device, 0, &err);
   if(err < 0) {
      perror("Couldn't create the command queue");
      exit(1);   
   }

   /* Enqueue the command queue to the device */
   work_units_per_kernel = M; /* 4 work-units per kernel */
   err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_units_per_kernel, 
      NULL, 0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't enqueue the kernel execution command");
      exit(1);   
   }

   /* Read the result */
   err = clEnqueueReadBuffer(queue, res_buff, CL_TRUE, 0, sizeof(float)*4, 
                             &result[0], 0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't enqueue the read buffer command");
      exit(1);   
   }
   /* Test the result */
   for(int i=0; i<M; ++i) {
       if( ! iszero(result[i] - correct[i]) ) {
           printf("%d matrix multip error %f != %f \n",i, result[i], correct[i]);
       }
   }
   printf("end.\n");


   /* Deallocate resources */
   clReleaseMemObject(mat_buff);
   clReleaseMemObject(vec_buff);
   clReleaseMemObject(res_buff);
   clReleaseKernel(kernel);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);

   return 0;
}

