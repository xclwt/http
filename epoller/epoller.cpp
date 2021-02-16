//
// Created by xclwt on 2021/2/11.
//

#include "epoller.h"

Epoller::Epoller(): m_epollFd(epoll_create(1)){
    assert(m_epollFd > 0);
}

Epoller::~Epoller(){
    close(m_epollFd);
}

bool Epoller::addfd(int fd, uint32_t events){
    if(fd < 0)
        return false;

    epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    return epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::removefd(int fd){
    if(fd < 0)
        return false;

    return epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, 0) == 0;
}

bool Epoller::modfd(int fd, uint32_t events){
    if(fd < 0)
        return false;

    epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    return epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

int Epoller::wait(int timeout){
    return epoll_wait(m_epollFd, m_events, MAX_EPOLL_EVENTS, timeout);
}

int Epoller::getEpollFd(int i){
    assert(i >= 0 && i < MAX_EPOLL_EVENTS);
    return m_events[i].data.fd;
}

uint32_t Epoller::getEpollEvents(int i){
    assert(i >= 0 && i < MAX_EPOLL_EVENTS);
    return m_events[i].events;
}
