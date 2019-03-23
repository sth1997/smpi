#ifndef PROC_H
#define PROC_H

#include <list>
#include <string>

class Proc
{
public:
    Proc():fd(-1), isHost(true) {}
    Proc(int tmpRank, int tmpFd, bool tmpIsHost):
        rank(tmpRank), fd(tmpFd), isHost(tmpIsHost) {}
    ~Proc();
    //Disallow copy constructor and operator=.
    Proc(const Proc&) = delete;
    Proc& operator = (const Proc&) = delete;

    /*const char* getIpAddr() const;
    void setIpAddr(const char* c);
    int getPort() const;
    void setPort(int tmpPort);*/
    int getRank() const
    {
        return this->rank;
    }
    void setRank(int tmpRank)
    {
        this->rank = tmpRank;
    }
    int getFd() const
    {
        return this->fd;
    }
    void setFd(int tmpFd)
    {
        this->fd = tmpFd;
    }
    bool getIsHost() const
    {
        return this->isHost;
    }
    void setIsHost(bool tmpIsHost)
    {
        this->isHost = tmpIsHost;
    }
    void clear();
private:
    int rank;
    //For MainProc, listen fd; for Proc, connection fd.
    int fd;
    bool isHost;
};

class MainProc: public Proc
{
public:
    MainProc() {}
    ~MainProc();
    bool havePeer() {return (peer.getFd() != -1);}
    void setup(int port);
    void connectToServer(const std::string &server, int port, int serverRank, bool serverIsHost);
    void acceptOneConnection();
    int getFdByRank(int peerRank) const;
    void clear();
    ssize_t sendBytes(const void* buf, size_t len, int peerRank) const;
    ssize_t recvBytes(void* buf, size_t len, int peerRank) const;
private:
    //if the main proc is host, it stores other hosts; if the main proc is smatNIC, it stores other smartNICs
    std::list<Proc> others;
    Proc peer;
};

extern MainProc mainProc;

#endif // PROC_H