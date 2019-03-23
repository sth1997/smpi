//No matter which executable smpirun runs, smartnic runs this program.

#include <mpi.h>
#include <common.h>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int recvNonzeroCount;
    int rc;
    // TODO : support other type
    MPI_Datatype datatype = MPI_FLOAT;
    while (1)
    {
        CHECK_SMPI_SUCCESS(MPI_Recv(&recvNonzeroCount, 1, MPI_INT, rank, TAG_HOST_SMARTNIC, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        if (recvNonzeroCount == -1) // means termination
            break;
        // TODO : use faster allocator
        char* buf0 = (char*) mallocAlign((sizeof(int) + getDataSize(datatype)) * recvNonzeroCount * size, sizeof(unsigned int));
        char* buf1 = (char*) mallocAlign((sizeof(int) + getDataSize(datatype)) * recvNonzeroCount * size, sizeof(unsigned int));
        char* buf2 = (char*) mallocAlign((sizeof(int) + getDataSize(datatype)) * recvNonzeroCount * size, sizeof(unsigned int));
        CHECK_SMPI_SUCCESS(MPI_Recv(buf0, recvNonzeroCount * 2, datatype, rank, TAG_HOST_SMARTNIC, MPI_COMM_WORLD, MPI_STATUS_IGNORE));

        char* resultBuf;
        int totalNonzeroCount;
        CHECK_SMPI_SUCCESS(allreduceSparse(buf0, buf1, buf2, size, rank, recvNonzeroCount, datatype, MPI_COMM_WORLD, resultBuf, totalNonzeroCount));

        CHECK_SMPI_SUCCESS(MPI_Send(&totalNonzeroCount, 1, MPI_INT, rank, TAG_HOST_SMARTNIC, MPI_COMM_WORLD));
        CHECK_SMPI_SUCCESS(MPI_Send(resultBuf, totalNonzeroCount * 2, datatype, rank, TAG_HOST_SMARTNIC, MPI_COMM_WORLD));

        freeAlign(buf0);
        freeAlign(buf1);
        freeAlign(buf2);
        //CHECK_SMPI_SUCCESS(MPI_Send());
    }
    MPI_Finalize();
    return 0;
}