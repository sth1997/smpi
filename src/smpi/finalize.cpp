#include <mpi.h>
#include <proc.h>
#include <common.h>

int MPI_Finalize(void)
{
    int rc;
    CHECK_SMPI_SUCCESS(checkInit());
    CHECK_SMPI_SUCCESS(MPI_Barrier(MPI_COMM_WORLD));
    mainProc.clear();
    smpiCommSize = 0;
    return MPI_SUCCESS;
}