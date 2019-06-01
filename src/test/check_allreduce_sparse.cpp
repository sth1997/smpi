#include <mpi.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

const float eps = 1e-8;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 2)
    {
        printf("Error! You should only start 2 processes");
        return 0;
    }
    float* a;
    int count;
    if (rank == 0)
    {
        if (!freopen("/home/chw/chw_lab/smpi/src/test/check_allreduce_sparse1.input", "r", stdin))
        {
            printf("Can't open file!!!\n");
            std::abort();
        }
        printf("begin to read\n");
        scanf("%d", &count);
        a = new float[count];
        for (int i = 0; i < count; ++i)
            scanf("%f", &a[i]);
        fclose(stdin);
    }
    else
    {
        if(!freopen("/home/chw/chw_lab/smpi/src/test/check_allreduce_sparse2.input", "r", stdin))
        {
            printf("Can't open file!!!\n");
            printf("Please change the file path to absolute path.");
            std::abort();
        }
        printf("begin to read\n");
        scanf("%d", &count);
        a = new float[count];
        for (int i = 0; i < count; ++i)
            scanf("%f", &a[i]);
        fclose(stdin);
    }
    int rc = MPI_Allreduce_Sparse(MPI_IN_PLACE, a, count, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    float* b = new float[count];
    if (!freopen("/home/chw/chw_lab/smpi/src/test/check_allreduce_sparse.output", "r", stdin))
    {
        printf("Can't open file!!!\n");
        printf("Please change the file path to absolute path.");
        std::abort();
    }
    for (int i = 0; i < count; ++i)
    {
        scanf("%f", &b[i]);
        if (fabs(b[i] - a[i]) > eps)
        {
            printf("Wrong!\n");
            delete []a;
            delete []b;
            MPI_Finalize();
            fclose(stdin);
            return 0;
        }
    }
    
    printf("Correct!\n");
    delete []a;
    delete []b;
    MPI_Finalize();
    return 0;
}
