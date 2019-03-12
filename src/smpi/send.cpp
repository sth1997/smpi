#include <mpi.h>
#include <proc.h>
#include <common.h>

int MPI_Send(const void *buf, int count, MPI_Datatype datatype,
            int dest, int tag, MPI_Comm comm)
{
    int rc;
    CHECK_SMPI_SUCCESS(checkInit());
    CHECK_SMPI_SUCCESS(checkRankSupport(dest));
    CHECK_SMPI_SUCCESS(checkTagSupport(tag));
    CHECK_SMPI_SUCCESS(checkCommSupport(comm));
    if (buf == NULL)
    {
        printf("MPI_Send with NULL pointer!\n");
        return MPI_ERR_BUFFER;
    }

    if (count <= 0)
    {
        printf("MPI_Send with count <= 0!\n");
        return MPI_ERR_COUNT;
    }
    //TODO : invalid buffer(len(buf) < sizeof(datatype) * count)

    //TODO : if overflow, split the buffer and send more times
    CHECK_SMPI_SUCCESS(checkMulOverflow((unsigned long) count, getDataSize(datatype)));

    ssize_t sentBytes = mainProc.sendBytes(buf, count * getDataSize(datatype), dest);
    //TODO : this code may cause an error when count * getDataSize(datatype) > MAX_UNSIGNED_LONG / 2
    if (sentBytes !=  count * getDataSize(datatype))
    {
        printf("MPI_Send failed!  Sent %ld bytes.\n", sentBytes);
        return MPI_ERR_UNKNOWN;
    }
    return MPI_SUCCESS;
}