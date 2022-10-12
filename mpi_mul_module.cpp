
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <mpi.h>
#include <stdio.h>
#include <fstream>
#include <omp.h>
#define MASTER 0
using namespace std;
using namespace std::chrono;
int size =5e2;
int **A, **B, **C, **A_sub, **B_sub, **C_sub;
#define NUM_THREAD 8

void print( int** A, int rows, int cols) ;
void generateMat(int** &A, int rows, int cols, bool initialize_data);
void head(int numtasks);
void node(int rank, int numtasks);
void mulMat(int **&A, int **&B, int **&C, int rows, int cols);

int main(int argc, char** argv){
    int numtasks, rank, name_len, tag=1; 
    char name[MPI_MAX_PROCESSOR_NAME];

    MPI_Status status; // receive routine information

    // Initialize the MPI environment
    MPI_Init(&argc,&argv); //process num and address

    // Get the number of tasks/process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    // Get the rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Find the processor name
    MPI_Get_processor_name(name, &name_len);

    int row_per_procs =  size/numtasks;
    int size_per_procs = (size*size)/numtasks;
    int size_bcast = size*size;





    // print(A, size,size);
    // print(B, size, size);
    printf("Hello SIT315. You get this message from %s, rank %d out of %d\n",name, rank, numtasks);
    if (rank == MASTER)
    {
        head(numtasks);

    }
    else
    {
        node(rank, numtasks);
    }
    MPI_Finalize();

}

void head( int numtasks)
{
    int row_per_procs =  size/numtasks;
    int size_per_procs = (size*size)/numtasks;
    int size_bcast = size*size;

    generateMat(A, size, size,true);
    generateMat(B,size,size, true);
    generateMat(C,size,size, false);
    generateMat(A_sub, row_per_procs, size,false);
    generateMat(C_sub, row_per_procs,size, false);
    print(A,size,size);
    print(B,size,size);


    auto start = high_resolution_clock::now();
    MPI_Scatter(&A[0][0],size_per_procs,MPI_INT, &A_sub[0][0],size_per_procs, MPI_INT,MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&B[0][0],size_bcast, MPI_INT, MASTER,MPI_COMM_WORLD);



    mulMat(A_sub,B,C_sub,row_per_procs,size);
    MPI_Gather(&C_sub[0][0],size_per_procs,MPI_INT,&C[0][0],size_per_procs,MPI_INT,MASTER,MPI_COMM_WORLD);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
        cout << "Time taken by function: "
    << duration.count() << " milliseconds"  <<endl;
    print(C,size,size);
    free(A); free(B); free(C);
}

void node(int rank, int numtasks)
{
    int row_per_procs =  size/numtasks;
    int size_per_procs = (size*size)/numtasks;
    int size_bcast = size*size;
    generateMat(A, size, size,false);
    generateMat(B,size,size, false);
    generateMat(C,size,size, false);
    generateMat(A_sub, row_per_procs, size,false);

    generateMat(C_sub, row_per_procs,size, false);

    MPI_Scatter(&A[0][0],size_per_procs,MPI_INT, &A_sub[0][0],size_per_procs, MPI_INT,MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&B[0][0],size_bcast, MPI_INT, MASTER,MPI_COMM_WORLD);



    mulMat(A_sub,B,C_sub,row_per_procs,size);
    MPI_Gather(&C_sub[0][0],size_per_procs,MPI_INT,&C[0][0],size_per_procs,MPI_INT,MASTER,MPI_COMM_WORLD);
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

void mulMat(int **&A, int **&B, int **&C, int rows, int cols)
{
    #pragma omp parallel num_threads(NUM_THREAD)
    {
    #pragma omp for nowait
    for (int i = 0;i<rows;i++){
        for (int j = 0; j< cols; j++){
            int elem = 0;
            #pragma omp parallel for reduction(+:elem)
            for (int k=0;k<cols; k++){
                #pragma omp critical
                elem += (A[i][k] * B[k][j]);
            }
            C[i][j] = elem;
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

