
__kernel void mul_mat(  const int M, //row size
                        const int N, //col size
                        const int K,
                        const __global int* A,const __global int* B,__global int* C) {
    
    // Thread identifiers
    const int globalRow = get_global_id(0);    //selected row A
    const int globalCol = get_global_id(1);    //selected col B

    //uncomment to see the index each PE works on

    // printf("Local buffer size %d %d %d\n", M, N, K);
    // printf("Kernel process index :(%d) (%d) (%d) ", globalRow, globalCol,K);
    int tmp = 0;
    for (int  k = 0; k<K; k++) //A-row and B-col
    {
        // printf("%d %d \n",A[globalRow*N+k] , B[k*K+globalCol]);
        tmp += A[globalRow*N+k] * B[k*K+globalCol];
        tmp = tmp%10;
    }
        // printf("%d \n", tmp);

    C[globalRow * N + globalCol] = tmp;
    }
