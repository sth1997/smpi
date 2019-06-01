#include <mpi.h>
#include <common.h>
#include <limits.h>
#include <cstring>
#include <algorithm>
#ifdef BREAKDOWN_ANALYSIS
#include <time.h>
#include <sys/time.h>
#include <omp.h>

double get_wall_time()
{
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
static double compressTime = 0.0;
static double decompressTime = 0.0;
static double addSparseTime = 0.0;
static double commTime = 0.0;
static double selectTime = 0.0;
#endif

struct CompressFormat
{
    unsigned int index;
    float value;
};


/*
 * Compress format: ((unsigned int) index, (datatype) val)
 * Example: (0,0,0,3,0,0,1,0,0,0,0,0,7,0,0) -> ((3,3),(6,1),(12,7))
 */
static MPI_RET_CODE compress(const void* src, void* dst, const float topKVal, MPI_Datatype datatype, int count, int nonzeroCount)
{
    #ifdef BREAKDOWN_ANALYSIS
    double start = get_wall_time();
    #endif
    //Now, only supports MPI_FLOAT
    if (datatype != MPI_FLOAT)
    {
        printf("SMPI compress only supports MPI_FLOAT!\n");
        return MPI_ERR_TYPE;
    }

    float* srcValue = (float*) src;
    CompressFormat* compressed = (CompressFormat*) dst;
    int compressNum = 0;
    // TODO : use multi-thread
    for (int index = 0; index < count; ++index)
        if (srcValue[index] >= topKVal)
        {
            compressed[compressNum].index = (unsigned int) index;
            compressed[compressNum].value = srcValue[index];
            if (++compressNum == nonzeroCount)
                break;
        }

    if (compressNum != nonzeroCount)
    {
        printf("nonzeroCount is not correct!\n");
        return MPI_ERR_COUNT;
    }
    #ifdef BREAKDOWN_ANALYSIS
    double end = get_wall_time();
    compressTime = end - start;
    #endif
    return MPI_SUCCESS;
}

static MPI_RET_CODE decompress(const void* src, void* dst, MPI_Datatype datatype, int count, int nonzeroCount)
{
    #ifdef BREAKDOWN_ANALYSIS
    double start = get_wall_time();
    #endif
    //Now, only supports MPI_FLOAT
    if (datatype != MPI_FLOAT)
    {
        printf("SMPI compress only supports MPI_FLOAT!\n");
        return MPI_ERR_TYPE;
    }

    // TODO : use multi-thread
    memset(dst, 0, getDataSize(datatype) * count);
    #ifdef BREAKDOWN_ANALYSIS
    printf("memsetTime = %.5f\n", get_wall_time() - start);
    #endif

    float* dstValue = (float*) dst;
    const CompressFormat* compressed = (CompressFormat*) src;
    for (int i = 0; i < nonzeroCount; ++i)
        dstValue[compressed[i].index] = compressed[i].value;
    #ifdef BREAKDOWN_ANALYSIS
    double end = get_wall_time();
    decompressTime = end - start;
    #endif
    return MPI_SUCCESS;
}

static MPI_RET_CODE addSparse(const void* src1, const void* src2, void* dst, int nonzeroCount1, int nonzeroCount2, int& totalNonzeroCount, MPI_Datatype datatype)
{
    #ifdef BREAKDOWN_ANALYSIS
    double start = get_wall_time();
    #endif
    //Now, only supports MPI_FLOAT
    if (datatype != MPI_FLOAT)
    {
        printf("SMPI addSparse only supports MPI_FLOAT!\n");
        return MPI_ERR_TYPE;
    }

    const CompressFormat* cp1 = (CompressFormat*) src1;
    const CompressFormat* cp2 = (CompressFormat*) src2;
    CompressFormat* cpAdd = (CompressFormat*) dst;
    int index1 = 0;
    int index2 = 0;
    totalNonzeroCount = nonzeroCount1 + nonzeroCount2;
    // TODO : use multi-thread
    int i;
    for (i = 0; i < nonzeroCount1 + nonzeroCount2; ++i)
        if (cp1[index1].index < cp2[index2].index)
        {
            cpAdd[i] = cp1[index1];
            if (++index1 == nonzeroCount1)
                break;
        }
        else if (cp1[index1].index > cp2[index2].index)
        {
            cpAdd[i] = cp2[index2];
            if (++index2 == nonzeroCount2)
                break;
        }
        else //srcIndex1[index1] == srcIndex2[index2]
        {
            cpAdd[i].index = cp1[index1].index;
            cpAdd[i].value = cp1[index1].value + cp2[index2].value;
            --totalNonzeroCount;
            ++index1;
            ++index2;
            if ((index1 == nonzeroCount1) || (index2 == nonzeroCount2))
                break;
        }
    
    ++i;
    /*
    while (index1 < nonzeroCount1 && index2 < nonzeroCount2)
    {
        if (cp1[index1].index != cp2[index2].index)
        {
            bool take1 = (cp1[index1].index < cp2[index2].index);
            cpAdd[i++] = take1 ? cp1[index1] : cp2[index2];
            index1 += take1;
            index2 += 1 - take1;
        }
        else
        {
            cpAdd[i].index = cp1[index1].index;
            cpAdd[i].value = cp1[index1++].value + cp2[index2++].value;
            --totalNonzeroCount;
        }
    }
    */

    if (index1 < nonzeroCount1)
        memcpy(&cpAdd[i], &cp1[index1], (sizeof(int) + getDataSize(datatype)) * (nonzeroCount1 - index1));
    else if (index2 < nonzeroCount2)
        memcpy(&cpAdd[i], &cp2[index2], (sizeof(int) + getDataSize(datatype)) * (nonzeroCount2 - index2));
    #ifdef BREAKDOWN_ANALYSIS
    double end = get_wall_time();
    addSparseTime += end - start;
    #endif
    return MPI_SUCCESS;
}

static MPI_RET_CODE sendrecv(const void *sendbuf, int sendNonzeroCount, void *recvbuf, int& recvNonzeroCount, int myRank, int peerRank,
                            int tag, MPI_Datatype datatype, MPI_Comm comm)
{
    #ifdef BREAKDOWN_ANALYSIS
    double start = get_wall_time();
    #endif
    int rc;
    if (myRank < peerRank)
    {
        CHECK_SMPI_SUCCESS(MPI_Send(&sendNonzeroCount, 1, MPI_INT, peerRank, 
                                    TAG_ALLREDUCE_SPARSE, comm));
        CHECK_SMPI_SUCCESS(MPI_Send(sendbuf, sendNonzeroCount * 2, datatype, peerRank, tag, comm));
        CHECK_SMPI_SUCCESS(MPI_Recv(&recvNonzeroCount, 1, MPI_INT, peerRank,
                                    TAG_ALLREDUCE_SPARSE, comm, MPI_STATUS_IGNORE));
        CHECK_SMPI_SUCCESS(MPI_Recv(recvbuf, recvNonzeroCount * 2, datatype, peerRank, tag, comm, MPI_STATUS_IGNORE));
    }
    else
    {
        CHECK_SMPI_SUCCESS(MPI_Recv(&recvNonzeroCount, 1, MPI_INT, peerRank,
                                    TAG_ALLREDUCE_SPARSE, comm, MPI_STATUS_IGNORE));
        CHECK_SMPI_SUCCESS(MPI_Recv(recvbuf, recvNonzeroCount * 2, datatype, peerRank, tag, comm, MPI_STATUS_IGNORE));
        CHECK_SMPI_SUCCESS(MPI_Send(&sendNonzeroCount, 1, MPI_INT, peerRank, 
                                    TAG_ALLREDUCE_SPARSE, comm));
        CHECK_SMPI_SUCCESS(MPI_Send(sendbuf, sendNonzeroCount * 2, datatype, peerRank, tag, comm));
    }
    #ifdef BREAKDOWN_ANALYSIS
    double end = get_wall_time();
    printf("commTime = %.5f %d %d\n", end - start, sendNonzeroCount, recvNonzeroCount);
    commTime += end - start;
    #endif
    return MPI_SUCCESS;
}

/*
 * The compressed data is stored in sbuf as input.
 * The result which is also compressed is stored in resultBuf as output.
 * Note, resultBuf must be sbuf or tmp_buf.
 */
MPI_RET_CODE allreduceSparse(char* sbuf, char* rbuf, char* tmp_buf, int size, int rank, int nonzeroCount, MPI_Datatype datatype, MPI_Comm comm, char* &resultBuf, int &retTotalNonzeroCount)
{
    char *tmpsend = NULL, *tmprecv = NULL, *tmpswap = NULL;
    tmpsend = (char*) sbuf;
    tmprecv = (char*) rbuf;
    char* tmpadd = tmp_buf;

    int newrank, newremote, extra_ranks, adjsize, remote, distance;
    int rc;

    adjsize = nextPowerOfTwoGT(size);
    /* Determine nearest power of two less than or equal to size */
    adjsize >>= 1;

    /* Handle non-power-of-two case:
       - Even ranks less than 2 * extra_ranks send their data to (rank + 1), and
       sets new rank to -1.
       - Odd ranks less than 2 * extra_ranks receive data from (rank - 1),
       apply appropriate operation, and set new rank to rank/2
       - Everyone else sets rank to rank - extra_ranks
    */
    extra_ranks = size - adjsize;
    int recvNonzeroCount, sendNonzeroCount = nonzeroCount;
    if (rank <  (2 * extra_ranks)) {
        if (0 == (rank & 1)) {
            // TODO : Combine these two MPI_Send.
            #ifdef BREAKDOWN_ANALYSIS
            double start = get_wall_time();
            #endif
            CHECK_SMPI_SUCCESS(MPI_Send(&sendNonzeroCount, 1, MPI_INT, (rank + 1),
                                        TAG_ALLREDUCE_SPARSE, comm));
            CHECK_SMPI_SUCCESS(MPI_Send(tmpsend, sendNonzeroCount * 2, datatype, (rank + 1),
                                        TAG_ALLREDUCE_SPARSE, comm));
            #ifdef BREAKDOWN_ANALYSIS
            double end = get_wall_time();
            commTime += end - start;
            #endif
            newrank = -1;
        } else {
            #ifdef BREAKDOWN_ANALYSIS
            double start = get_wall_time();
            #endif
            CHECK_SMPI_SUCCESS(MPI_Recv(&recvNonzeroCount, 1, MPI_INT, (rank - 1),
                                    TAG_ALLREDUCE_SPARSE, comm, MPI_STATUS_IGNORE));
            CHECK_SMPI_SUCCESS(MPI_Recv(tmprecv, recvNonzeroCount * 2, datatype, (rank - 1),
                                    TAG_ALLREDUCE_SPARSE, comm, MPI_STATUS_IGNORE));
            #ifdef BREAKDOWN_ANALYSIS
            double end = get_wall_time();
            commTime += end - start;
            #endif
            /* tmpsend = tmprecv (op) tmpsend */
            int totalNonzeroCount;
            CHECK_SMPI_SUCCESS(addSparse(tmpsend, tmprecv, tmpadd, sendNonzeroCount,
                                        recvNonzeroCount, totalNonzeroCount, datatype));
            tmpswap = tmpadd;
            tmpadd = tmpsend;
            tmpsend = tmpswap;
            sendNonzeroCount = totalNonzeroCount;
            newrank = rank >> 1;
        }
    } else {
        newrank = rank - extra_ranks;
    }

    /* Communication/Computation loop
       - Exchange message with remote node.
       - Perform appropriate operation taking in account order of operations:
       result = value (op) result
    */
    for (distance = 0x1; distance < adjsize; distance <<= 1) {
        if (newrank < 0) break;
        /* Determine remote node */
        newremote = newrank ^ distance;
        remote = (newremote < extra_ranks)?
            (newremote * 2 + 1):(newremote + extra_ranks);

        /* Exchange the data */
        CHECK_SMPI_SUCCESS(sendrecv(tmpsend, sendNonzeroCount, tmprecv, recvNonzeroCount,
                                    rank, remote, TAG_ALLREDUCE_SPARSE, datatype, comm));
        
        int totalNonzeroCount;
        CHECK_SMPI_SUCCESS(addSparse(tmpsend, tmprecv, tmpadd, sendNonzeroCount,
                                    recvNonzeroCount, totalNonzeroCount, datatype));
        tmpswap = tmpadd;
        tmpadd = tmpsend;
        tmpsend = tmpswap;
        sendNonzeroCount = totalNonzeroCount;
        resultBuf = tmpsend;
    }

    /* Handle non-power-of-two case:
       - Odd ranks less than 2 * extra_ranks send result from tmpsend to
       (rank - 1)
       - Even ranks less than 2 * extra_ranks receive result from (rank + 1)
    */
    if (rank < (2 * extra_ranks)) {
        if (0 == (rank & 1)) {
            #ifdef BREAKDOWN_ANALYSIS
            double start = get_wall_time();
            #endif
            CHECK_SMPI_SUCCESS(MPI_Recv(&recvNonzeroCount, 1, MPI_INT, (rank + 1),
                                    TAG_ALLREDUCE_SPARSE, comm, MPI_STATUS_IGNORE));
            CHECK_SMPI_SUCCESS(MPI_Recv(tmpsend, recvNonzeroCount * 2, datatype, (rank + 1),
                                    TAG_ALLREDUCE_SPARSE, comm, MPI_STATUS_IGNORE));
            #ifdef BREAKDOWN_ANALYSIS
            double end = get_wall_time();
            commTime += end - start;
            #endif
            resultBuf = tmpsend;
            retTotalNonzeroCount = recvNonzeroCount;
            return MPI_SUCCESS;
        } else {
            #ifdef BREAKDOWN_ANALYSIS
            double start = get_wall_time();
            #endif
            CHECK_SMPI_SUCCESS(MPI_Send(&sendNonzeroCount, 1, MPI_INT, (rank - 1),
                                        TAG_ALLREDUCE_SPARSE, comm));
            CHECK_SMPI_SUCCESS(MPI_Send(tmpsend, sendNonzeroCount * 2, datatype, (rank - 1),
                                        TAG_ALLREDUCE_SPARSE, comm));
            #ifdef BREAKDOWN_ANALYSIS
            double end = get_wall_time();
            commTime += end - start;
            #endif
            resultBuf = tmpsend;
            retTotalNonzeroCount = sendNonzeroCount;
            return MPI_SUCCESS;
        }
    }
    retTotalNonzeroCount = sendNonzeroCount;
    return MPI_SUCCESS;
}

void printComp(const void* tmp, int count)
{
    const CompressFormat* buf = (const CompressFormat*) tmp;
    for (int i = 0; i < count; ++i)
        printf("%d %.1f   ", buf[i].index, buf[i].value);
    printf("\n");
}

float random_select1(float* a, int l, int r, int k)
{
    while (l < r)
    {
        float tmp = a[(l + r) >> 1];
        a[(l + r) >> 1] = a[l];
        a[l] = tmp;
        float key = tmp;
        int i = l + 1, j = r;
        while (i <= j)
        {
            while (a[i] > key)
                ++i;
            while (a[j] < key)
                --j;
            if (i < j)
            {
                tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
                ++i;
                --j;
            }
            else
                ++i;
        }
        a[l] = a[j];
        a[j] = key;

        int q = j;
        int tmp_k = q - l;
        if (tmp_k == k)
            return a[q];
        else if (k < tmp_k)
            r = q - 1;
        else
        {
            l = q + 1;
            k -= tmp_k + 1;
        }
    }
    return a[l];
}

float random_select2(float* a, int l, int r, int k)
{
    k = r - k;
    while (l < r)
    {
        float tmp = a[(l + r) >> 1];
        a[(l + r) >> 1] = a[r];
        a[r] = tmp;
        float key = tmp;
        int i = l - 1;
        for (int j = l; j < r; ++j)
        {
            if (a[j] <= key)
            {
                ++i;
                tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
            }
        }
        a[r] = a[i + 1];
        a[i + 1] = key;

        int q = i + 1;
        int tmp_k = q - l;
        if (tmp_k == k)
            return a[q];
        else if (k < tmp_k)
            r = q - 1;
        else
        {
            l = q + 1;
            k -= tmp_k + 1;
        }
    }
    return a[l];
}

float random_select3(float* buf, int count, int k)
{
    std::nth_element(buf, buf + (count - k - 1), buf + count);
    return buf[count - k - 1];
}

float randomSelect(float* buf, int count, int k)
{
    //return random_select1(buf, 0, count - 1, k);
    //return random_select2(buf, 0, count - 1, k);
    return random_select3(buf, count, k);
}

float select(const float* buf, const int count)
{
    #ifdef BREAKDOWN_ANALYSIS
    double start = get_wall_time();
    #endif
    // TODO : try to merge some sample buffers into one buffer
    
    // sample buf[0~sampleCount-1]
    int sampleCount = count / 100;
    
    static int times = 0;
    ++times;
    static float* tmpBuf;
    if (times == 1)
        // NOTE sampleCount * 10 * sizeof(float). The code "tmpBuf[index++]" may cause some error if we just alloc sampleCount*sizeof(float).
        tmpBuf = (float*) mallocAlign(sampleCount * 10 * sizeof(float), 4);
    float tmpKVal;
    bool sampleFailed = true;
    float ratio = 5.0f / 1000;
    while (sampleFailed)
    {
        memcpy(tmpBuf, buf, sampleCount * sizeof(float));
        tmpKVal = randomSelect(tmpBuf, sampleCount, sampleCount * ratio - 1);
        // int index = 0;

        /*
        // TODO : use multi-thread
        for (int i = 0; i < count; ++i)
            // do NOT set a[i]=0 if a[i] < tmpKVal
            if (buf[i] >= tmpKVal)
                tmpBuf[index++] = buf[i];
        */


        //chw multi-thread
        int thread_count = 8;
	printf("thread_count is %d\n", thread_count);
        /*float** tmpbuf_thread = new float*[thread_count];
        for(int i = 0; i < thread_count; i++)
            *tmpbuf_thread = new float[count / thread_count];*/
	float** tmpbuf_thread;
	tmpbuf_thread = (float**) malloc(thread_count * sizeof(float*));
	for(int i = 0; i < thread_count; i++)
		tmpbuf_thread[i] = (float*)malloc(count / thread_count * sizeof(float));
	printf("malloc ok\n");
        #pragma omp parallel num_threads(thread_count)
        {
            int rank = omp_get_thread_num() + 1;
            int size = omp_get_num_threads();
	    printf("rank is %d, size is %d", rank, size);
            memset(tmpbuf_thread[rank - 1], 0, sizeof(float) * count / thread_count);
            int thread_index = 0;
            for(int i = (rank - 1) * count / size; i < rank * count / size; i++)
            {
                if(buf[i] >= tmpKVal)
                    tmpbuf_thread[rank - 1][thread_index++] = buf[i];
            }
        }
        //merge

        int index = 0;
        for(int i = 0; i < thread_count; i++)
        {
            for(int j = 0; tmpbuf_thread[i][j] != 0 && j < count / thread_count; j++)
                tmpBuf[index++] = tmpbuf_thread[i][j];
        }


        printf("tmpKval = %.5f index = %d  count = %d\n", tmpKVal, index, count);
        if (index > sampleCount * 10)
        {
            printf("Index is too large! May cause buffer overflow error!");
            std::abort();
        }
        if (index < count / 1000)
        {
            sampleFailed = true;
            ratio *= 2.0f;
            continue;
        }
        float ret =  randomSelect(tmpBuf, index, count / 1000 - 1);
        #ifdef BREAKDOWN_ANALYSIS
        selectTime = get_wall_time() - start;
        #endif
        return ret;
    }
}

/*
 * Adapted from OpenMPI: openmpi-4.0.0/ompi/mca/coll/base/coll_base_allreduce.c: ompi_coll_base_allreduce_intra_recursivedoubling.
 *
 *   Function:       Recursive doubling algorithm for allreduce operation
 *   Accepts:        Same as MPI_Allreduce()
 *   Returns:        MPI_SUCCESS or error code
 *
 *   Description:    Implements recursive doubling algorithm for allreduce.
 *                   Original (non-segmented) implementation is used in MPICH-2
 *                   for small and intermediate size messages.
 *                   The algorithm preserves order of operations so it can
 *                   be used both by commutative and non-commutative operations.
 *
 *         Example on 7 nodes:
 *         Initial state
 *         #      0       1      2       3      4       5      6
 *               [0]     [1]    [2]     [3]    [4]     [5]    [6]
 *         Initial adjustment step for non-power of two nodes.
 *         old rank      1              3              5      6
 *         new rank      0              1              2      3
 *                     [0+1]          [2+3]          [4+5]   [6]
 *         Step 1
 *         old rank      1              3              5      6
 *         new rank      0              1              2      3
 *                     [0+1+]         [0+1+]         [4+5+]  [4+5+]
 *                     [2+3+]         [2+3+]         [6   ]  [6   ]
 *         Step 2
 *         old rank      1              3              5      6
 *         new rank      0              1              2      3
 *                     [0+1+]         [0+1+]         [0+1+]  [0+1+]
 *                     [2+3+]         [2+3+]         [2+3+]  [2+3+]
 *                     [4+5+]         [4+5+]         [4+5+]  [4+5+]
 *                     [6   ]         [6   ]         [6   ]  [6   ]
 *         Final adjustment step for non-power of two nodes
 *         #      0       1      2       3      4       5      6
 *              [0+1+] [0+1+] [0+1+]  [0+1+] [0+1+]  [0+1+] [0+1+]
 *              [2+3+] [2+3+] [2+3+]  [2+3+] [2+3+]  [2+3+] [2+3+]
 *              [4+5+] [4+5+] [4+5+]  [4+5+] [4+5+]  [4+5+] [4+5+]
 *              [6   ] [6   ] [6   ]  [6   ] [6   ]  [6   ] [6   ]
 *
 */
int MPI_Allreduce_Sparse(const void *sbuf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int rc;
    CHECK_SMPI_SUCCESS(checkInit());
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(sbuf));
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(rbuf));
    CHECK_SMPI_SUCCESS(checkCommSupport(comm));
    if (datatype != MPI_FLOAT)
    {
        printf("SMPI MPI_Allreduce_Sparse only supports MPI_FLOAT!\n");
        return MPI_ERR_TYPE;
    }

    //Now, this function only supports MPI_SUM because of the memory alignment problem.
    if (op != MPI_SUM)
    {
        printf("SMPI MPI_Allreduce_Sparse only supports MPI_SUM!\n");
        return MPI_ERR_OP;
    }

    #ifdef BREAKDOWN_ANALYSIS
    compressTime = 0.0;
    decompressTime = 0.0;
    addSparseTime = 0.0;
    commTime = 0.0;
    selectTime = 0.0;
    #endif

    int rank, size;
    char *inplacebuf_free = NULL;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);


    /* Special case for size == 1 */
    if (1 == size) {
        if (MPI_IN_PLACE != sbuf) {
            memcpy(rbuf, sbuf, getDataSize(datatype) * (unsigned long) count);
        }
        return MPI_SUCCESS;
    }

    float topKVal;
    if (MPI_IN_PLACE == sbuf)
        topKVal = select((float*) rbuf, count);
    else
        topKVal = select((float*) sbuf, count);

    const int nonzeroCount = count / 1000;
    /* Allocate and initialize temporary send buffer */
    // Now, we assume nonzeroCount in every process is the same.
    // TODO : Use Allreduce to calculate the total number of nonzeroCount
    // TODO : use faster allocator
    inplacebuf_free = (char*) mallocAlign((sizeof(int) + getDataSize(datatype)) * nonzeroCount * size, sizeof(unsigned int));
    // Used for the addition of two buf.
    bool useSmartnic = mainProc.havePeer();
    char* tmp_buf;
    if (!useSmartnic)
        tmp_buf = (char*) mallocAlign((sizeof(int) + getDataSize(datatype)) * nonzeroCount * size, sizeof(unsigned int));
    if (NULL == inplacebuf_free || (!useSmartnic && NULL == tmp_buf))
    {
        printf("SMPI mallocAlign failed\n");
        return MPI_ERR_UNKNOWN;
    }
    if (MPI_IN_PLACE == sbuf)
        CHECK_SMPI_SUCCESS(compress(rbuf, inplacebuf_free, topKVal, datatype, count, nonzeroCount));
    else
        CHECK_SMPI_SUCCESS(compress(sbuf, inplacebuf_free, topKVal, datatype, count, nonzeroCount));

    int totalNonzeroCount;
    char* resultBuf;
    if (useSmartnic)
    {
        // TODO : Supporting for breakdown analysis when using smartnic
        CHECK_SMPI_SUCCESS(MPI_Send(&nonzeroCount, 1, MPI_INT, rank, TAG_HOST_SMARTNIC, comm));
        CHECK_SMPI_SUCCESS(MPI_Send(inplacebuf_free, nonzeroCount * 2, datatype, rank, TAG_HOST_SMARTNIC, comm));
        CHECK_SMPI_SUCCESS(MPI_Recv(&totalNonzeroCount, 1, MPI_INT, rank, TAG_HOST_SMARTNIC, comm, MPI_STATUS_IGNORE));
        CHECK_SMPI_SUCCESS(MPI_Recv(inplacebuf_free, totalNonzeroCount * 2, datatype, rank, TAG_HOST_SMARTNIC, comm, MPI_STATUS_IGNORE));
        resultBuf = inplacebuf_free;
    }
    else
    {
        CHECK_SMPI_SUCCESS(allreduceSparse(inplacebuf_free, (char*)rbuf, tmp_buf, size, rank, nonzeroCount, datatype, comm, resultBuf, totalNonzeroCount));
    }
    CHECK_SMPI_SUCCESS(decompress(resultBuf, rbuf, datatype, count, totalNonzeroCount));
    freeAlign(inplacebuf_free);
    if (!useSmartnic)
        freeAlign(tmp_buf);
    #ifdef BREAKDOWN_ANALYSIS
    printf("compressTime = %.5f\n", compressTime);
    printf("decompressTime = %.5f\n", decompressTime);
    printf("selectTime = %.5f\n", selectTime);
    printf("addSparseTime = %.5f\n", addSparseTime);
    printf("commTime = %.5f\n", commTime);
    #endif
    return MPI_SUCCESS;
}
