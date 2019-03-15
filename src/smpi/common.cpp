#include <common.h>

MPI_RET_CODE checkCommSupport(MPI_Comm comm)
{
    if (comm != MPI_COMM_WORLD)
    {
        printf("SMPI doesn't support such COMM!\n");
        return MPI_ERR_COMM;
    }
    return MPI_SUCCESS;
}

MPI_RET_CODE checkRankSupport(int rank)
{
    if (rank < 0 || rank >= smpiCommSize)
    {
        printf("SMPI doesn't support such rank!\n");
        return MPI_ERR_RANK;
    }
    return MPI_SUCCESS;
}

MPI_RET_CODE checkTagSupport(int tag)
{
    if (tag != 0)
    {
        printf("SMPI doesn't support such tag!\n");
        return MPI_ERR_TAG;
    }
    return MPI_SUCCESS;
}

MPI_RET_CODE checkStatusSupport(MPI_Status* status)
{
    if (status != MPI_STATUS_IGNORE)
    {
        printf("SMPI doesn't support such tag!\n");
        return MPI_ERR_IN_STATUS;
    }
    return MPI_SUCCESS;
}

MPI_RET_CODE checkMulOverflow(unsigned long count, size_t size)
{
    unsigned long val = count * size;
    if (count && val / count != size)
    {
        printf("SMPI count * size_t overflow!\n");
        return MPI_ERR_OVERFLOW;
    }
    return MPI_SUCCESS;
}

MPI_RET_CODE checkInit()
{
    if (smpiCommSize <= 0 || mainProc.getFd() == -1)
    {
        printf("SMPI have not initialized!\n");
        return MPI_ERR_NOT_INIT;
    }
    return MPI_SUCCESS;
}

MPI_RET_CODE checkPointerNotNULL(const void* p)
{
    if (p == NULL)
    {
        printf("Pointer should not be NULL!\n");
        return MPI_ERR_NULL_POINTER;
    }
    return MPI_SUCCESS;
}

size_t getDataSize(MPI_Datatype datatype)
{
    switch (datatype)
    {
        case MPI_INT:
            return sizeof(int);
        case MPI_FLOAT:
            return sizeof(float);
        default:
        {
            printf("Datatype with unknown size!\n");
            return 0;
        }
    }
}

/* Find the nearest power of 2 greater than or equal to this value*/
int nextPowerOfTwoGE(int value)
{
    int power2;
    for (power2 = 1 ; power2 < value; power2 <<= 1) /* empty */;
    return power2;
}

/* Find the nearest power of 2 greater than this value*/
int nextPowerOfTwoGT(int value)
{
    int power2;
    for (power2 = 1; power2 <= value; power2 <<= 1) /* empty */;
    return power2;
}

// Return a buf with the start address aligned.
void* mallocAlign(size_t numBytes, int align)
{
    char* p = (char*) malloc(numBytes + align);
    if (p == NULL)
        return NULL;
    char* retp = p + align - ((unsigned long)p % align);
    *(retp - 1) = retp - p;
    return retp;
}

void freeAlign(char* p)
{
    void* realp = p - *(p - 1);
    free(realp);
}