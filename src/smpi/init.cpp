#include <mpi.h>
#include <proc.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <common.h>

static const int LISTEN_PORT = 23333;
static const int MAX_HOST_NAME_OR_IP_LEN = 20;

int smpiCommSize = 0;

int MPI_Init(int *argc, char ***argv)
{
    //TODO : use getopt for arg
    int rc;
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(argc));
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(argv));
    for (int i = 0; i < *argc; ++i)
        if (strcmp((*argv)[i], "--smpirank") == 0)
        {
            mainProc.setRank(atoi((*argv)[i+1]));
            printf("rank = %d\n", atoi((*argv)[i+1]));
        }
        else if (strcmp((*argv)[i], "--smpisize") == 0)
        {
            smpiCommSize = atoi((*argv)[i+1]);
            printf("size = %d\n", atoi((*argv)[i+1]));
        }

    // Begin to listen connections
    mainProc.setup(LISTEN_PORT + mainProc.getRank());

    std::ifstream fin("/tmp/hosts");
    std::string str;
    char host[MAX_HOST_NAME_OR_IP_LEN];
    for (int i = 0; getline(fin, str); ++i)
    {
        sscanf(str.c_str(), "%s", host);

        // Begin to build connections.
        if (i < mainProc.getRank())
        {
            mainProc.connectToServer(std::string(host), LISTEN_PORT + i, i, true);
        }
        else if (i == mainProc.getRank())
        {
            break;
        }
    }
    fin.close();

    for (int i = mainProc.getRank() + 1; i < smpiCommSize; ++i)
    {
        mainProc.acceptOneConnection();
    }
    return MPI_SUCCESS;
}
