#include <mpi.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <time.h>
#include <sys/time.h>

double get_wall_time()
{
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int n = atoi(argv[argc - 1]);
    if (n == 0)
    {
        printf("Error! numFloats can't be zero!\n");
        printf("Usage: smpirun.py --hostfile=gorgonhosts test_allreduce_sparse numFloats");
        return 0;
    }
    int interval = 1000 * 2; //sparsity ratio = 1/1000
    int nonzeroCount = 0;
    float* a = (float*) malloc(sizeof(float) * n);
    memset(a, 0, sizeof(float) * n);
    if (a == NULL)
    {
        printf("Can't malloc so much memory!\n");
        return 0;
    }
    for (int i = 0; i < n; ++i)
        if (i % interval == 0)
            a[i] = 1.0f * i / interval + 1, ++nonzeroCount;
        else if ((i - rank - 1) % interval == 0)
            a[i] = 1.0f * i / interval + 1, ++nonzeroCount;
        else
            a[i] = 0.0f;
    MPI_Barrier(MPI_COMM_WORLD);
    printf("nonzerocount = %d  n= %d\n", nonzeroCount, n);
    double startTime = get_wall_time();
    int rc = MPI_Allreduce_Sparse(MPI_IN_PLACE, a, n, nonzeroCount, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    double endTime = get_wall_time();
    printf("Rank%d : Time = %.5f  Bytes = %luKB\n", rank, endTime - startTime, n / 1024 * sizeof(float));
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


    interval = 1000; // sparsity ratio = 1/1000, with no overlap
    nonzeroCount = 0;
    for (int i = 0; i < n; ++i)
        if ((i - rank - 1) % interval == 0)
            a[i] = 1.0f * i / interval + 1, ++nonzeroCount;
        else
            a[i] = 0.0f;
    MPI_Barrier(MPI_COMM_WORLD);
    printf("nonzerocount = %d  n= %d\n", nonzeroCount, n);
    startTime = get_wall_time();
    rc = MPI_Allreduce_Sparse(MPI_IN_PLACE, a, n, nonzeroCount, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    endTime = get_wall_time();
    printf("Rank%d : Time = %.5f  Bytes = %luKB\n", rank, endTime - startTime, n / 1024 * sizeof(float));
    if (rc != MPI_SUCCESS)
    {
        printf("Rank%d : Allreduce failed! rc = %d\n", rank, rc);
        return 0;
    }
    correct = true;
    for (int i = 0; i < n; ++i)
        if ((i % interval <= size) && (i % interval > 0))
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
