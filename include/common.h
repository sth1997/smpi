#ifndef COMMON_H
#define COMMON_H

#include <mpi.h>
#include <proc.h>
#include <cstdio>

#define CHECK_SMPI_SUCCESS(fn) \
do \
{ \
    if ((rc = (fn)) != MPI_SUCCESS) \
        return rc; \
} while(0)

MPI_RET_CODE checkCommSupport(MPI_Comm comm);

MPI_RET_CODE checkRankSupport(int rank);

MPI_RET_CODE checkTagSupport(int tag);

MPI_RET_CODE checkStatusSupport(MPI_Status* status);

MPI_RET_CODE checkMulOverflow(unsigned long count, size_t size);

MPI_RET_CODE checkInit();

size_t getDataSize(MPI_Datatype datatype);

int nextPowerOfTwoInclusive(int value);

// Comm tag
#define TAG_BARRIER 0

#endif //COMMON_H