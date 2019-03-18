#include <proc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <mpi.h>

#define MAX_RECV_LEN 4096
#define MAX_SEND_LEN 4096

MainProc mainProc;

Proc::~Proc()
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

void Proc::clear()
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

void MainProc::setup(int port)
{
    int  listenfd;
    struct sockaddr_in  servaddr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    int on=1;
	if((setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
	{
		perror("setsockopt failed\n");
		return;
	}

    if( bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
    {
        printf("Rank%d : bind socket error: %s(errno: %d)\n",this->getRank(), strerror(errno),errno);
        return;
    }

    if( listen(listenfd, smpiCommSize) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return;
    }

    this->setFd(listenfd);
    printf("Rank%d : listenfd = %d\n", this->getRank(), listenfd);
}

void MainProc::connectToServer(const std::string& server, int port, int serverRank, bool serverIsHost)
{
    int sockfd;
    struct sockaddr_in  servaddr;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return;
    }
    printf("Rank%d : sockfd = %d\n", this->getRank(), sockfd);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_addr(server.c_str()) == (unsigned)-1)
  	{
    	struct hostent *he;
    	struct in_addr **addr_list;
    	if ( (he = gethostbyname( server.c_str() ) ) == NULL)
    	{
		    herror("gethostbyname");
      		printf("Failed to resolve hostname\n");
			return;
    	}
	   	addr_list = (struct in_addr **) he->h_addr_list;
    	for(int i = 0; addr_list[i] != NULL; i++)
    	{
			servaddr.sin_addr = *addr_list[i];
		    break;
    	}
  	}
  	else
    	servaddr.sin_addr.s_addr = inet_addr(server.c_str());

    while (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("Rank%d: connect error: %s(errno: %d)\n",this->getRank(), strerror(errno),errno);
        // Connection refused, maybe the server has not listened on the port.
        // Sleep for a while and try again.
        if (errno == ECONNREFUSED)
        {
            sleep(1);
            continue;
        }
        else
            return;
    }
    int myRank = this->getRank();
    if( send(sockfd, &myRank, sizeof(int), 0) < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        return;
    }

    //Use emplace_back to avoid copy constructor.
    this->others.emplace_back(serverRank, sockfd, serverIsHost);
    printf("%d connect with %d\n", myRank, serverRank);
}

void MainProc::acceptOneConnection()
{
    int  connfd, listenfd = this->getFd();
    if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1)
    {
        printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
        return;
    }
    printf("Rank%d : connfd = %d\n", this->getRank(), connfd);

    int clientRank;
	recv(connfd, &clientRank, sizeof(int), 0);
    int myRank = this->getRank();
    bool clientIsHost = this->getIsHost();

    if (clientRank == myRank)
    {
        clientIsHost = !clientIsHost;
        this->peer.setRank(clientRank);
        this->peer.setFd(connfd);
        this->peer.setIsHost(clientIsHost);
    }
    else
    {
        //Use emplace_back to avoid copy constructor.
        this->others.emplace_back(clientRank, connfd, clientIsHost);
    }
    printf("%d connect with %d\n", myRank, clientRank);
}

//Return -1 for invalid peerRank.
int MainProc::getFdByRank(int peerRank) const
{
    for (auto &proc : others)
        if (proc.getRank() == peerRank)
            return proc.getFd();
    if (peerRank == peer.getRank())
        return peer.getFd();
    printf("Can not find any fd by this rank. This rank may be invalid!\n");
    return -1;
}

//Return sent bytes. Return -1 for error.
ssize_t MainProc::sendBytes(const void* buf, size_t len, int peerRank) const
{
    //TODO : invalid TCP buffer(len(buf) > TCP buffer), split the buffer and send more times
    int fd, sentBytes;
    if ((fd = this->getFdByRank(peerRank)) == -1)
        return -1;
    if ((sentBytes = send(fd , buf, len, 0)) < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    else
    {
        return sentBytes;
    }
}

//Return received bytes. Return -1 for error.
ssize_t MainProc::recvBytes(void* buf, size_t len, int peerRank) const
{
    //TODO : invalid TCP buffer(len(buf) > TCP buffer), split the buffer and send more times
    int fd, recvedBytes;
    if ((fd = this->getFdByRank(peerRank)) == -1)
        return -1;
    size_t restLen = len;
    while (restLen)
    {
        if ((recvedBytes = recv(fd, buf, (restLen < MAX_RECV_LEN) ? restLen : MAX_RECV_LEN, 0)) < 0)
        {
            printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
            return -1;
        }
        else
        {
            restLen -= recvedBytes;
            buf = (char*) buf + recvedBytes;
        }
    }
    return len;
}

void MainProc::clear()
{
    Proc::clear();
    this->peer.clear();
    others.clear();
}