#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <mpi.h>
#include <stdio.h>
#include <fstream>
#include <omp.h>
#include <CL/cl.h>
#define MASTER 0


#define PRINT 1
using namespace std;
using namespace std::chrono;
int size = 5e2;
int **A, **B, **C, **A_sub, **B_sub, **C_sub;

//data transfer to device, reside in GPU
cl_mem bufA, bufB, bufC;

//device(CPU,GPU) id
cl_device_id device_id;
//openCL context type
cl_context context;
// host program 
cl_program program;
//kernel function
cl_kernel kernel;
//OpenCL command queue for communication
cl_command_queue queue;
cl_event event = NULL;

const int mem_max = size;
const int TS = 4; //indexing pe

size_t local[2] = { (size_t)TS, (size_t)TS }; //local working group size
size_t global[2] = { (size_t) mem_max, (size_t) mem_max }; //global working gr size
int err;

//create a device in the openCL context, return the device id
cl_device_id create_device();
//set-up funcitn for kernel (execution filename (.cl), kernel name(s))
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
//Build program on device
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
//load/copy memory and argument
void setup_kernel_memory(int rows_per_proc, int** &A, int** &B, int** &C);
//Copy arguement from host to buffer
void copy_kernel_args(int rows_per_proc);
//free up memory
void free_memory();

void generateMat(int** &A, int rows, int cols, bool initialize_data);
void print( int** A, int rows, int cols);
void mulMat(int **&A, int **&B, int **&C, int rows, int cols);
void node(int rank, int numtasks);
void head (int numtasks);

int main(int argc, char **argv)
{
    //Initialize MPI
    int numtasks, rank, name_len, tag=1; 
    char name[MPI_MAX_PROCESSOR_NAME];
    srand(time(0));
    MPI_Status status; // receive routine information

    // Initialize the MPI environment
    MPI_Init(&argc,&argv); //process num and address

    // Get the number of tasks/process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    // Get the rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Find the processor name
    MPI_Get_processor_name(name, &name_len);

    printf("Hello SIT315. You get this message from %s, rank %d out of %d\n",name, rank, numtasks);

    if (rank == MASTER){
        head(numtasks);
    }
    else{
        node(rank,numtasks);
    }
    
    // free_memory();
    MPI_Finalize();
}
void head(int numtasks)
{
    int rows_per_proc =  size/numtasks;
    int size_per_procs = (size*size)/numtasks;
    int size_bcast = size*size;

    generateMat(A, size, size,true);
    generateMat(B,size,size, true);
    generateMat(C,size,size, false);
    generateMat(A_sub, rows_per_proc, size,false);
    generateMat(C_sub, rows_per_proc,size, false);
    //Print out matrix
    print(A,size,size);
    print(B,size,size);

    auto start = high_resolution_clock::now();

    MPI_Scatter(&A[0][0],size_per_procs,MPI_INT, &A_sub[0][0],size_per_procs, MPI_INT,MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&B[0][0],size_bcast, MPI_INT, MASTER,MPI_COMM_WORLD);
//Start OpenCL
    local[0] = rows_per_proc; local[1] = size;
    global[0] = rows_per_proc; global[1] = size;
    setup_openCL_device_context_queue_kernel((char *)"./opencl_mul.cl", (char *)"mul_mat");

    setup_kernel_memory(rows_per_proc,A_sub, B, C_sub);
    copy_kernel_args(rows_per_proc);


    //submit kernel to the buffer for execution
    /*
    Command queue, kernel, dimension for work, global offset, worksize,
    */
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
        //Read the buffer object to get the result from device
    clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, rows_per_proc*size * sizeof(int), &C_sub[0][0], 0, NULL, NULL);
//End OpenCL

    //Final matrix
    MPI_Gather(&C_sub[0][0],size_per_procs,MPI_INT,&C[0][0],size_per_procs,MPI_INT,MASTER,MPI_COMM_WORLD);

    print(C,size,size);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Time taken by function: "
    << duration.count() << " milliseconds"   
    <<endl;

    free_memory();

}

void node(int rank, int numtasks)
{
    int rows_per_proc =  size/numtasks;
    int size_per_procs = (size*size)/numtasks;
    int size_bcast = size*size;
    generateMat(A, size, size,false);
    generateMat(B,size,size, false);
    generateMat(C,size,size, false);
    generateMat(A_sub, rows_per_proc, size,false);

    generateMat(C_sub, rows_per_proc,size, false);

    MPI_Scatter(&A[0][0],size_per_procs,MPI_INT, &A_sub[0][0],size_per_procs, MPI_INT,MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&B[0][0],size_bcast, MPI_INT, MASTER,MPI_COMM_WORLD);
//Start OpenCL
    local[0] = rows_per_proc; local[1] = size;
    global[0] = rows_per_proc; global[1] = size;
    setup_openCL_device_context_queue_kernel((char *)"./opencl_mul.cl", (char *)"mul_mat");

    setup_kernel_memory(rows_per_proc,A_sub, B, C_sub);
    copy_kernel_args(rows_per_proc);


    //submit kernel to the buffer for execution
    /*
    Command queue, kernel, dimension for work, global offset, worksize,
    */
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
        //Read the buffer object to get the result from device
    clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, rows_per_proc*size * sizeof(int), &C_sub[0][0], 0, NULL, NULL);
//End OpenCL
    MPI_Gather(&C_sub[0][0],size_per_procs,MPI_INT,&C[0][0],size_per_procs,MPI_INT,MASTER,MPI_COMM_WORLD);
    free_memory();
}



void free_memory()
{
    //free the buffers
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    free(A);
    free(B);
    free(C);

    free(A_sub);
    free(B_sub);
    free(C_sub);

}


void copy_kernel_args(int rows_per_proc)
{
    //set args value for kernel
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&rows_per_proc);
    clSetKernelArg(kernel, 1, sizeof(int), (void *)&size);
    clSetKernelArg(kernel, 2, sizeof(int), (void *)&size);

    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufA);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&bufB);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&bufC);
    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory(int rows_per_proc, int** &A, int** &B, int** &C)
{
    //allocate buffer memory object for 3 matrices
    //The second parameter of the clCreateBuffer is cl_mem_flags flags. Check the OpenCL documention to find out what is it's purpose and read the List of supported memory flag values 
    bufA = clCreateBuffer(context, CL_MEM_READ_WRITE, rows_per_proc*size * sizeof(int), NULL, NULL);
    bufB = clCreateBuffer(context, CL_MEM_READ_WRITE, size*size * sizeof(int), NULL, NULL);
    bufC = clCreateBuffer(context, CL_MEM_READ_WRITE, rows_per_proc*size * sizeof(int), NULL, NULL);
    // Copy matrices to the GPU buffer
    clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, rows_per_proc*size * sizeof(int), &A[0][0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, size*size * sizeof(int), &B[0][0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufC, CL_TRUE, 0, rows_per_proc*size * sizeof(int), &C[0][0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    //create OpenCL context on device
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    //Create command-queue for the given device_id
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };


    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{

    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    /* Read program file and place content into buffer */
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    //Create host program from file
    //The code read file content into char array, then create a program from the source code
    program = clCreateProgramWithSource(ctx, 1,
                                        (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program 

   The fourth parameter accepts options that configure the compilation. 
   These are similar to the flags used by gcc. For example, you can 
   define a macro with the option -DMACRO=VALUE and turn off optimization 
   with -cl-opt-disable.
   */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
    //   printf("GPU not found\n");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}

void generateMat(int** &A, int rows, int cols, bool initialize_data)
{
    A = (int **) malloc(sizeof(int)*cols*rows);
    int* temp = (int *) malloc(sizeof(int)*cols*rows);

    for ( int i = 0 ; i < rows ; i ++)
    {
        A[i] = &temp[i * cols];
    }
    if (initialize_data)
    {
        for (long i = 0 ; i < rows ; i++)
        {
            for (long j = 0 ; j < cols ; j++)
            {
                A[i][j] = rand() % 10;       //any number less than 100
            }
        }
    }

}

void print( int** A, int rows, int cols) {
    if (rows > 10)
    {
        rows = 10;
        if (cols > 10)
        {
            cols = 10;
            printf("First 10*10 element: \n");
        }

    }

  for(long i = 0 ; i < rows; i++) { //rows
        for(long j = 0 ; j < cols; j++) {  //cols
            printf("%d ",  A[i][j]); // print the cell value
        }
        printf("\n"); //at the end of the row, print a new line
    }
    printf("----------------------------\n");
}