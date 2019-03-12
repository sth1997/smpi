#include <mpi.h>
#include <common.h>
#include <proc.h>

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
    int rc;
    CHECK_SMPI_SUCCESS(checkInit());
    CHECK_SMPI_SUCCESS(checkCommSupport(comm));
    if (rank == NULL)
    {
        printf("MPI_Comm_Rank with NULL pointer!\n");
        return MPI_ERR_RANK;
    }
    *rank = mainProc.getRank();
    return MPI_SUCCESS;
}