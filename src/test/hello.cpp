#include <mpi.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    /*int num = 1024 * 2;
    void* buf = malloc(sizeof(int) * num);
    if (buf == NULL)
    {
        printf("Can't malloc so much memory!\n");
        return 0;
    }*/
    if (rank & 1)
    {
        int recv;
        if (MPI_Recv(&recv, 1, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) == MPI_SUCCESS)
        //if (MPI_Recv(buf, num, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) == MPI_SUCCESS)
            printf("Rank%d : received %d from %d\n", rank, recv, rank ^ 1);
        else
            printf("Rank%d : recv from %d failed!\n", rank, rank ^ 1);
    }
    else if (rank != size - 1) // to avoid size == odd number
    {
        int send = rank + 233;
        if (MPI_Send(&send, 1, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD) == MPI_SUCCESS)
        //if (MPI_Send(buf, num, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD) == MPI_SUCCESS)
            printf("Rank%d : sent %d to %d\n", rank, send, rank ^ 1);
        else
            printf("Rank%d : sent to %d failed!\n", rank, rank ^ 1);
    }
    MPI_Finalize();
    return 0;
}