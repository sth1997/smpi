#include <mpi.h>
#include <common.h>

int MPI_Comm_size(MPI_Comm comm, int *size)
{
    int rc;
    CHECK_SMPI_SUCCESS(checkInit());
    CHECK_SMPI_SUCCESS(checkCommSupport(comm));
    if (size == NULL)
    {
        printf("MPI_Comm_Size with NULL pointer!\n");
        return MPI_ERR_SIZE;
    }
    *size = smpiCommSize;
    return MPI_SUCCESS;
}