//
// Created by xclwt on 2021/2/8.
//

#ifndef HTTP_WEBSERVER_H
#define HTTP_WEBSERVER_H

#include <cstdint>
#include <memory>
#include <cstring>
#include <csignal>
#include <functional>
#include "httpconn.h"
#include "timer.h"
#include "threadpool.h"
#include "epoller.h"
#include "log.h"
#include "utils.h"

using namespace std;

#define MAX_FD 65536    /* theoretically the value can be set up to 2^63 - 1 */
#define PATH_LEN 256
#define TIMESLOT 5

class WebServer {
public:
    WebServer(
            int port, int trigMode, int timeout, bool OptLinger,
            int threadNum, bool openLog, int logLevel, int logQueSize);

    ~WebServer();

    void eventLoop();

private:
    bool eventListen();

    void initTrigMode();

    void addClient(int fd, sockaddr_in addr);

    void dealListen();

    void dealWrite(int sockFd);

    void dealRead(int sockFd);

    bool dealSig(bool &timeout);

    void dealTimer(int sockFd);

    void addTimer(int sockFd, sockaddr_in addr);

    void adjustTimer(int sockFd);

    void closeConn(HttpConn* client);

    void readTask(HttpConn* client);

    void writeTask(HttpConn* client);

    void process(HttpConn* client);

    void timeHandler();

    static void sendError(int fd, const char*info);

    static int setNonblock(int fd);

    static void addsig(int sig, void(handler)(int), bool restart = true);

    static void sig_handler(int sig);

    static int  m_pipeFd[2];

    int m_port;
    bool m_optLinger;
    int m_timeout;
    bool m_isClose;
    int m_listenFd;

    char m_rootDir[PATH_LEN];

    int m_trigMode;
    uint32_t m_listenEvent;

    unique_ptr<TimeWheel> m_timeWheel;
    unique_ptr<ClientData[]> m_usersTimer;
    unique_ptr<ThreadPool> m_threadpool;
    unique_ptr<Epoller> m_epoller;
    unique_ptr<HttpConn[]> m_users;
};

#endif //HTTP_WEBSERVER_H
