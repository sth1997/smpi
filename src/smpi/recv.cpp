#include <mpi.h>
#include <proc.h>
#include <common.h>

int MPI_Recv(void *buf, int count, MPI_Datatype datatype,
            int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    int rc;
    CHECK_SMPI_SUCCESS(checkInit());
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(buf));
    CHECK_SMPI_SUCCESS(checkRankSupport(source));
    CHECK_SMPI_SUCCESS(checkTagSupport(tag));
    CHECK_SMPI_SUCCESS(checkCommSupport(comm));
    CHECK_SMPI_SUCCESS(checkStatusSupport(status));

    if (count <= 0)
    {
        printf("MPI_Recv with count <= 0!\n");
        return MPI_ERR_COUNT;
    }
    //TODO : invalid buffer(len(buf) < sizeof(datatype) * count)

    ssize_t recvedBytes = mainProc.recvBytes(buf, count * getDataSize(datatype), source);
 
    if (recvedBytes !=  count * getDataSize(datatype))
    {
        printf("MPI_Recv failed!  Received %ld bytes.\n", recvedBytes);
        return MPI_ERR_UNKNOWN;
    }
    return MPI_SUCCESS;
}