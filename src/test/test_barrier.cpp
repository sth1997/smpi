#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

double get_wall_time() 
{ 
  struct timeval time ; 
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
    double startTime;
    MPI_Barrier(MPI_COMM_WORLD);
    startTime = get_wall_time();
    if (rank == 1)
        sleep(3);
    MPI_Barrier(MPI_COMM_WORLD);
    double endTime = get_wall_time();
    printf("Rank%d : Barrier time = %.6f\n", rank, endTime - startTime);
    MPI_Finalize();
    return 0;
}