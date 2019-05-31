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
    float* a = (float*) malloc(sizeof(float) * n);
    if (a == NULL)
    {
        printf("Can't malloc so much memory!\n");
        return 0;
    }
    srand(rank);
    for (int i = 0; i < n; ++i)
        a[i] = rand() * 1.0f / rand();
    MPI_Barrier(MPI_COMM_WORLD);
    double startTime = get_wall_time();
    int rc = MPI_Allreduce_Sparse(MPI_IN_PLACE, a, n, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    double endTime = get_wall_time();
    printf("Rank%d : Time = %.5f  Bytes = %luKB\n", rank, endTime - startTime, n / 1024 * sizeof(float));
    if (rc != MPI_SUCCESS)
    {
        printf("Rank%d : Allreduce failed! rc = %d\n", rank, rc);
        return 0;
    }
    
    for (int i = 0; i < n; ++i)
        a[i] = rand() * 1.0f / rand();
    MPI_Barrier(MPI_COMM_WORLD);
    startTime = get_wall_time();
    rc = MPI_Allreduce_Sparse(MPI_IN_PLACE, a, n, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    endTime = get_wall_time();
    printf("Rank%d : Time = %.5f  Bytes = %luKB\n", rank, endTime - startTime, n / 1024 * sizeof(float));
    if (rc != MPI_SUCCESS)
    {
        printf("Rank%d : Allreduce failed! rc = %d\n", rank, rc);
        return 0;
    }
    MPI_Finalize();
}
