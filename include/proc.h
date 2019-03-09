#ifndef PROC_H
#define PROC_H

#include <list>
#include <string>

class Proc
{
public:
    Proc();
    Proc(int tmpRank, int tmpFd, bool tmpIsHost);
    ~Proc();
    //Disallow copy constructor and operator=.
    Proc(const Proc&) = delete;
    Proc& operator = (const Proc&) = delete;

    /*const char* getIpAddr() const;
    void setIpAddr(const char* c);
    int getPort() const;
    void setPort(int tmpPort);*/
    int getRank() const;
    void setRank(int tmpRank);
    int getFd() const;
    void setFd(int tmpFd);
    bool getIsHost() const;
    void setIsHost(bool tmpIsHost);
private:
    int rank;
    int fd;
    bool isHost;
};

class MainProc: public Proc
{
public:
    MainProc();
    ~MainProc();
    void setup(int port);
    void connectToServer(const std::string &server, int port, int serverRank, bool serverIsHost);
    void acceptOneConnection();
private:
    //if the main proc is host, it stores other hosts; if the main proc is smatNIC, it stores other smartNICs
    std::list<Proc> others;
    Proc peer;
};

extern MainProc mainProc;
extern int smpiCommSize;

#endif // PROC_H