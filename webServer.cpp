//
// Created by xclwt on 2021/2/8.
//

#include "webServer.h"

WebServer::WebServer(int port, int trigMode, int timeout, bool optLinger,
                     int threadNum, bool openLog, int logLevel, int logQueueSize):
                     m_port(port), m_timeout(timeout), m_optLinger(optLinger),
                     m_trigMode(trigMode), m_timeWheel(new TimeWheel()),
                     m_usersTimer(new ClientData[MAX_FD]),
                     m_threadpool(new ThreadPool(threadNum)),
                     m_epoller(new Epoller()), m_users(new HttpConn[MAX_FD]){

    getcwd(m_rootDir, PATH_LEN);
    strncat(m_rootDir, "/resources/", 12);
    HttpConn::m_rootDir = m_rootDir;
    HttpConn::m_user_cnt = 0;

    initTrigMode();

    if (openLog){
        Log::init(logLevel, DEFAULT_MAX_LINE, logQueueSize, DEFAULT_LOG_BUF, "./log", "log");
    }

    if(!eventListen()) m_isClose = true;

    if(m_isClose){
        LOG_ERROR("Server init error!");
    }else{
        LOG_INFO("Server init successfully.");
    }
}

WebServer::~WebServer(){
    close(m_listenFd);
    m_isClose = true;
}

void WebServer::eventLoop(){
    if (!m_isClose){
        LOG_INFO("Server Start");
    }
    while (!m_isClose){

    }
}

bool WebServer::eventListen(){
    if (m_port < 1024 || m_port > 65535){
        LOG_ERROR("Invalid Port %d", m_port);
        return false;
    };

    m_listenFd = socket(PF_INET, SOCK_STREAM, 0);

    if (m_listenFd < 0){
        LOG_ERROR("Invalid Listen Fd %d", m_listenFd);
        return false;
    }

    linger optLinger = {0 ,0};
    if (m_optLinger){
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    /* gracefully closed */
    int ret = setsockopt(m_listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0){
        LOG_ERROR("Fail to set linger");
        return false;
    }

    /* reuse addr */
    int optval = 1;
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0){
        LOG_ERROR("fail to set reuseaddr");
        return false;
    }

    ret = bind(m_listenFd, (sockaddr*)&addr, sizeof(addr));
    if (ret < 0){
        LOG_ERROR("fail to bind port: %d", m_port);
        return false;
    }

    ret = listen(m_listenFd, SOMAXCONN);
    if (ret < 0){
        LOG_ERROR("fail to listen port: %d", m_port);
        return false;
    }

    ret = m_epoller->addfd(m_listenFd, m_listenEvent);
    if (ret < 0){
        LOG_ERROR("fail to add listen event to epoll");
        return false;
    }

    setNonblock(m_listenFd);
    LOG_INFO("Server Listen Port: %d", m_port);
    return true;
}

void WebServer::initTrigMode(){
    m_listenEvent = EPOLLRDHUP | EPOLLIN;

    if (m_trigMode == 0);
    else{
        m_listenEvent |= EPOLLET;
    }
}



