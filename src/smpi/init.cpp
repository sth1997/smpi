#include <mpi.h>
#include <proc.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <common.h>

static const int LISTEN_PORT = 23333;
static const int MAX_HOST_NAME_OR_IP_LEN = 32;

int smpiCommSize = 0;

static int calcListenPort(bool isHost, int rank)
{
    if (isHost)
        return (LISTEN_PORT + rank);
    else
        return (LISTEN_PORT + rank + smpiCommSize);
}

int MPI_Init(int *argc, char ***argv)
{
    //TODO : use getopt for arg
    int rc;
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(argc));
    CHECK_SMPI_SUCCESS(checkPointerNotNULL(argv));
    bool useSmartnic = false;
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
        else if (strcmp((*argv)[i], "--use-smartnic") == 0)
        {
            //Host
            mainProc.setIsHost(true);
            useSmartnic = true;
        }
        else if (strcmp((*argv)[i], "--is-smartnic") == 0)
        {
            //Smartnic
            mainProc.setIsHost(false);
            useSmartnic = true;
        }

    // Begin to listen connections
    mainProc.setup(calcListenPort(mainProc.getIsHost(), mainProc.getRank()));

    std::ifstream fin("/tmp/hosts");
    std::string str;
    char host[MAX_HOST_NAME_OR_IP_LEN];
    char smartnic[MAX_HOST_NAME_OR_IP_LEN];
    char tmpBuffer[128];
    for (int i = 0; getline(fin, str); ++i)
    {
        strcpy(tmpBuffer, str.c_str());
        char *tokenPtr=strtok(tmpBuffer," ="); 
        strcpy(host, tokenPtr);

        while(tokenPtr!=NULL)
        { 
            if (strcmp(tokenPtr, "smartnic") == 0)
            {
                tokenPtr=strtok(NULL," =");
                strcpy(smartnic, tokenPtr);
            }
            tokenPtr=strtok(NULL," ="); 
        } 

        // Begin to build connections.
        if (i < mainProc.getRank())
        {
            //Connection between two hosts or two smartnics
            if (mainProc.getIsHost())
                mainProc.connectToServer(std::string(host), calcListenPort(true, i), i, true);
            else
                mainProc.connectToServer(std::string(smartnic), calcListenPort(false, i), i, false);
        }
        else if (i == mainProc.getRank())
        {
            //Smartnic connect to its host
            if (!mainProc.getIsHost())
                mainProc.connectToServer(std::string(host), calcListenPort(true, i), i, true);
            break;
        }
    }
    fin.close();

    for (int i = mainProc.getRank() + 1; i < smpiCommSize; ++i)
    {
        mainProc.acceptOneConnection();
    }
    // Accept one connection from its smartnic
    if (useSmartnic && mainProc.getIsHost())
    {
        mainProc.acceptOneConnection();
    }
    return MPI_SUCCESS;
}
