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

    //TODO : if overflow, split the buffer and recv more times
    CHECK_SMPI_SUCCESS(checkMulOverflow((unsigned long) count, getDataSize(datatype)));

    ssize_t recvedBytes = mainProc.recvBytes(buf, count * getDataSize(datatype), source);
    //TODO : this code may cause an error when count * getDataSize(datatype) > MAX_UNSIGNED_LONG / 2
    if (recvedBytes !=  count * getDataSize(datatype))
    {
        printf("MPI_Recv failed!  Received %ld bytes.\n", recvedBytes);
        return MPI_ERR_UNKNOWN;
    }
    return MPI_SUCCESS;
}