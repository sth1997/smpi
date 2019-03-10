#include <proc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>

MainProc mainProc;
int smpiCommSize = 0;

Proc::Proc() : fd(-1), isHost(true)
{
}

Proc::Proc(int tmpRank, int tmpFd, bool tmpIsHost) : rank(tmpRank), fd(tmpFd), isHost(tmpIsHost)
{
}

Proc::~Proc()
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

/*const char* Proc::getIpAddr() const
{
    return this->ipAddr;
}

void Proc::setIpAddr(const char* c)
{
    memcpy(this->ipAddr, c, strlen(c));
}

int Proc::getPort() const
{
    return this->port;
}
void Proc::setPort(int tmpPort)
{
    this->port = tmpPort;
}*/

int Proc::getRank() const
{
    return this->rank;
}

void Proc::setRank(int tmpRank)
{
    this->rank = tmpRank;
}

int Proc::getFd() const
{
    return fd;
}

void Proc::setFd(int tmpFd)
{
    this->fd = tmpFd;
}

bool Proc::getIsHost() const
{
    return this->isHost;
}

void Proc::setIsHost(bool tmpIsHost)
{
    this->isHost = tmpIsHost;
}

MainProc::MainProc()
{
}

MainProc::~MainProc()
{
}

void MainProc::setup(int port)
{
    int  listenfd;
    struct sockaddr_in  servaddr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
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

    if( bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1){
        printf("Rank%d : bind socket error: %s(errno: %d)\n",this->getRank(), strerror(errno),errno);
        return;
    }

    if( listen(listenfd, smpiCommSize) == -1){
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

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
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

    while (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
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
    if( send(sockfd, &myRank, sizeof(int), 0) < 0){
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
    if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
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
