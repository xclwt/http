//
// Created by xclwt on 2021/2/8.
//

#include "webServer.h"

WebServer::WebServer(int port, int trigMode, int timeout, bool optLinger,
                     int threadNum, bool openLog, int logLevel, int logQueueSize):
                     m_port(port), m_timeout(timeout), m_optLinger(optLinger),
                     m_trigMode(trigMode), m_timeWheel(new TimeWheel(TIMESLOT)),
                     m_usersTimer(new ClientData[MAX_FD]),
                     m_threadpool(new ThreadPool(threadNum)),
                     m_epoller(Epoller::getEpollInstance()), m_users(new HttpConn[MAX_FD]){

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
    bool timeout = false;

    if (!m_isClose){
        LOG_INFO("Server Start");
    }
    while (!m_isClose){
        int num = m_epoller->wait(-1);

        for (int i = 0; i < num; ++i){
            int sockFd = m_epoller->getEpollFd(i);
            uint32_t events = m_epoller->getEpollEvents(i);

            if (sockFd == m_listenFd){
                dealListen();
            }else if (events & (EPOLLRDHUP  | EPOLLHUP | EPOLLERR)){
                dealTimer(sockFd);
                /* ? */
            }else if (sockFd == m_pipeFd[0] && (events & EPOLLIN)){
                dealSig(timeout);
            }else if (events & EPOLLIN){
                dealRead(sockFd);
            }else if (events & EPOLLOUT){
                dealWrite(sockFd);
            }
        }

        if (timeout){
            timeHandler();
            LOG_INFO("timer tick");
            timeout = false;
        }
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

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipeFd);
    if (ret < 0){
        LOG_ERROR("fail to init signal pipe");
        return false;
    }

    setNonblock(m_pipeFd[1]);

    ret = m_epoller->addfd(m_pipeFd[0], EPOLLIN | EPOLLET);
    if (ret < 0){
        LOG_ERROR("fail to add sig pipe event to epoll");
        return false;
    }

    addsig(SIGPIPE, SIG_IGN);
    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);

    alarm(TIMESLOT);

    return true;
}

void WebServer::initTrigMode(){
    m_listenEvent = EPOLLRDHUP | EPOLLIN;

    if (m_trigMode == 0);
    else{
        m_listenEvent |= EPOLLET;
    }
}

void WebServer::addClient(int fd, sockaddr_in addr){

}

void WebServer::dealListen(){
    sockaddr_in addr;
    int addrlen = sizeof(addr);

    do {
        int fd = accept(m_listenFd, (sockaddr *)&addr, (socklen_t *)&addrlen);

        if (fd < 0)
            return;
        else if (HttpConn::m_user_cnt >= MAX_FD){
            sendError(fd, "Server is busy!");
            LOG_WARN("Http connections is full!");
            return;
        }

        addTimer(fd, addr);
        addClient(fd, addr);
        LOG_INFO("connection %d established", fd);
    } while (m_listenEvent & EPOLLET);
}

bool WebServer::dealSig(bool &timeout){
    char sigBuf[1024];
    int ret = recv(m_pipeFd[0], sigBuf, sizeof(sigBuf), 0);

    if(ret <= 0)
        return false;
    else{
        for (int i = 0; i < ret; ++i){
            switch (sigBuf[i]) {
                case SIGALRM:
                    timeout = true;
                    break;
                case SIGTERM:
                    m_isClose = true;
                    break;
            }
        }
    }
}

void WebServer::dealRead(int sockFd){
    adjustTimer(sockFd);
    m_threadpool->AddTask(std::bind(&WebServer::readTask, this, m_users[sockFd]));
}

void WebServer::dealWrite(int sockFd){
    adjustTimer(sockFd);
    m_threadpool->AddTask(std::bind(&WebServer::writeTask, this, m_users[sockFd]));
}

void WebServer::dealTimer(int sockFd){
    Timer *timer = m_usersTimer[sockFd].timer;

    if (timer){
        timer->cb_func(&m_usersTimer[sockFd]);
        m_timeWheel->del_timer(timer);
    }
    LOG_INFO("close sockFd: %d", sockFd);
}

void WebServer::addTimer(int sockFd, sockaddr_in addr){
    m_usersTimer[sockFd].address = addr;
    m_usersTimer[sockFd].sockfd = sockFd;

    Timer *timer = m_timeWheel->add_timer(120 * TIMESLOT);
    timer->user_data = &m_usersTimer[sockFd];
    timer->cb_func = cb_func;

    m_usersTimer[sockFd].timer = timer;
}

void WebServer::adjustTimer(int sockFd){
    Timer *timer = m_usersTimer[sockFd].timer;
    m_timeWheel->adjust_timer(timer, 12 * TIMESLOT);
}

void WebServer::closeConn(HttpConn *client){

}

void WebServer::readTask(HttpConn *client){
    int readErrno = 0;
    int ret = client->read(readErrno);

    if (ret <= 0 && readErrno != EAGAIN){
        closeConn(client);
        return;
    }

    process(client);
}

void WebServer::writeTask(HttpConn *client){

}

void WebServer::process(HttpConn *client){

}

void WebServer::timeHandler(){
    m_timeWheel->tick();
    alarm(TIMESLOT);
}

void WebServer::sendError(int fd, const char *info){
    int ret = send(fd, info, strlen(info), 0);

    if (ret < 0){
        LOG_INFO("fail to send error to client[%d]", fd);
    }

    close(fd);
}

int WebServer::setNonblock(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void WebServer::addsig(int sig, void (*handler)(int), bool restart){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    if (restart)
        sa.sa_flags |= SA_RESTART;

    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void WebServer::sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(m_pipeFd[1], (char*)&msg, 1, 0 );
    errno = save_errno;
}
