#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int num = 1024 * 20;
    int* buf = (int*) malloc(sizeof(int) * num);
    memset(buf, 0, sizeof(int) * num);
    if (buf == NULL)
    {
        printf("Can't malloc so much memory!\n");
        return 0;
    }
    if (rank & 1)
    {
        if (MPI_Recv((void*)buf, num, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) == MPI_SUCCESS)
            printf("Rank%d : received %d from %d\n", rank, buf[num - 1], rank ^ 1);
        else
            printf("Rank%d : recv from %d failed!\n", rank, rank ^ 1);
        bool correct = true;
        for (int i = 0; i < num; ++i)
            if (buf[i] != i)
            {
                correct = false;
                break;
            }
        if (!correct)
            printf("Rank%d : Recv failed!!!!!!!\n", rank);
    }
    else if (rank != size - 1) // to avoid size == odd number
    {
        //if (MPI_Send(&send, 1, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD) == MPI_SUCCESS)
        for (int i = 0; i < num; ++i)
            buf[i] = i;
        if (MPI_Send((void*)buf, num, MPI_INT, rank ^ 1, 0, MPI_COMM_WORLD) == MPI_SUCCESS)
            printf("Rank%d : sent %d to %d\n", rank, buf[num - 1], rank ^ 1);
        else
            printf("Rank%d : sent to %d failed!\n", rank, rank ^ 1);
    }
    MPI_Finalize();
    return 0;
}