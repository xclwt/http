//
// Created by xclwt on 2021/2/11.
//

#ifndef HTTP_EPOLLER_H
#define HTTP_EPOLLER_H

#include <sys/epoll.h>
#include <cassert>
#include <unistd.h>

#define MAX_EPOLL_EVENTS 10240

class Epoller{
public:
    static Epoller* getEpollInstance();

    bool addfd(int fd, uint32_t events);

    bool removefd(int fd);

    bool modfd(int fd, uint32_t events);

    int wait(int timeout);

    int getEpollFd(int i);

    uint32_t getEpollEvents(int i);

private:
    Epoller();

    ~Epoller();

    int m_epollFd;

    epoll_event m_events[MAX_EPOLL_EVENTS];
};

#endif //HTTP_EPOLLER_H
