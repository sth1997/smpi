#include <mpi.h>
#include <proc.h>
#include <common.h>

/*
 * Adapted from OpenMPI: openmpi-4.0.0/ompi/mca/coll/base/coll_base_barrier.c: ompi_coll_base_barrier_intra_tree.
 * Another recursive doubling type algorithm, but in this case
 * we go up the tree and back down the tree.
 */
int MPI_Barrier(MPI_Comm comm)
{
    int rank, size, depth, jump, partner;
    int rc;
    CHECK_SMPI_SUCCESS(MPI_Comm_rank(comm, &rank));
    CHECK_SMPI_SUCCESS(MPI_Comm_size(comm, &size));

    /* Find the nearest power of 2 of the communicator size. */
    depth = nextPowerOfTwoInclusive(size);

    for (jump=1; jump<depth; jump<<=1)
    {
        partner = rank ^ jump;
        if (!(partner & (jump-1)) && partner < size)
        {
            int tmpBuf = 0;
            if (partner > rank)
                CHECK_SMPI_SUCCESS(MPI_Recv(&tmpBuf, 1, MPI_INT, partner,
                                         TAG_BARRIER, comm,
                                         MPI_STATUS_IGNORE));

            else if (partner < rank)
                CHECK_SMPI_SUCCESS(MPI_Send(&tmpBuf, 1, MPI_INT, partner,
                                         TAG_BARRIER, comm));
        }
    }

    depth >>= 1;
    for (jump = depth; jump>0; jump>>=1)
    {
        partner = rank ^ jump;
        if (!(partner & (jump-1)) && partner < size)
        {
            int tmpBuf = 0;
            if (partner > rank)
                CHECK_SMPI_SUCCESS(MPI_Send(&tmpBuf, 1, MPI_INT, partner,
                                         TAG_BARRIER, comm));
            else if (partner < rank)
                CHECK_SMPI_SUCCESS(MPI_Recv(&tmpBuf, 1, MPI_INT, partner,
                                         TAG_BARRIER, comm,
                                         MPI_STATUS_IGNORE));
        }
    }

    return MPI_SUCCESS;
}