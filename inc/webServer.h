//
// Created by xclwt on 2021/2/8.
//

#ifndef HTTP_WEBSERVER_H
#define HTTP_WEBSERVER_H

#include <stdint.h>
#include <memory>
#include <cstring>
#include "httpconn.h"
#include "timer.h"
#include "threadpool.h"
#include "epoller.h"
#include "log.h"

using namespace std;

#define MAX_FD 65536
#define PATH_LEN 256

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

    void dealWrite(HttpConn* client);

    void dealRead(HttpConn* client);

    void sendError(int fd, const char*info);

    void extentTime(HttpConn* client);

    void closeConn(HttpConn* client);

    void onRead(HttpConn* client);

    void onWrite(HttpConn* client);

    void onProcess(HttpConn* client);

    static int setNonblock(int fd);

    static int addsig(int sig);

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
