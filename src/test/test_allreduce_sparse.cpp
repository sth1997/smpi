#include <mpi.h>
#include <cstdio>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int interval = 200;
    int n = 4000;
    int nonzeroCount = 0;
    float a[n];
    for (int i = 0; i < n; ++i)
        if (i % interval == 0)
            a[i] = 1.0f * i / interval + 1, ++nonzeroCount;
        else if ((i - rank - 1) % interval == 0)
            a[i] = 1.0f * i / interval + 1, ++nonzeroCount;
        else
            a[i] = 0.0f;
    int rc = MPI_Allreduce_Sparse(MPI_IN_PLACE, a, n, nonzeroCount, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS)
    {
        printf("Rank%d : Allreduce failed! rc = %d\n", rank, rc);
        return 0;
    }
    bool correct = true;
    for (int i = 0; i < n; ++i)
        if (i % interval == 0)
        {
            if (a[i] != (1.0f * i / interval + 1) * size)
                correct = false;
        }
        else if (i % interval <= size)
        {
            if (a[i] != (1.0f * i / interval + 1))
                correct = false;
        }
        else
            if (a[i] != 0.0f)
                correct = false;
    if (correct)
        printf("Rank%d : Correct!\n", rank);
    else
        printf("Rank%d : Wrong!\n", rank);
    MPI_Finalize();
}