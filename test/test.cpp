//
// Created by xclwt on 2021/2/16.
//


#include "log.h"
#include "threadpool.h"
#include "timer.h"
#include "epoller.h"
#include "httpConn.h"
#include <cstdio>
#include <csignal>
#include <cstring>
#include <memory>
#include <features.h>
#include <functional>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

int m_pipeFd[2];
int cnt = 0;

int setNonblock(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addsig(int sig, void (*handler)(int), bool restart){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    if (restart)
        sa.sa_flags |= SA_RESTART;

    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(m_pipeFd[1], (char*)&msg, 1, 0 );
    errno = save_errno;
}

void cb_func1(ClientData *user_data){
    printf("%d\n", cnt);
    LOG_INFO("%d\n", ++cnt);
}

void TestLog() {
    int cnt = 0, level = 0;
    Log::getInstance()->init(level, DEFAULT_MAX_LINE, 0, DEFAULT_LOG_BUF, "./synclog", ".log");
    for(level = 3; level >= 0; level--) {
        Log::getInstance()->setLevel(level);
        for(int j = 0; j < 10010; j++ ){
            for(int i = 0; i < 4; i++) {
                LOG_BASE(i,"============= %s No.1 %d ============= ", "Test", cnt++);
            }
        }
    }
    cnt = 0;
    Log::getInstance()->init(level, DEFAULT_MAX_LINE, 5000, DEFAULT_LOG_BUF, "./asynclog", ".log");
    for(level = 0; level < 4; level++) {
        Log::getInstance()->setLevel(level);
        for(int j = 0; j < 10010; j++ ){
            for(int i = 0; i < 4; i++) {
                LOG_BASE(i,"============= %s No.2 %d ============= ", "Test", cnt++);
            }
        }
    }
}

void ThreadLogTask(int i, int cnt) {
    for(int j = 0; j < 1000; j++ ){
        LOG_BASE(i,"PID:[%04d]======= %05d ========= ", gettid(), cnt++);
    }
}

[[noreturn]] void TestThreadPool() {
    Log::getInstance()->init(0, DEFAULT_MAX_LINE, 5000, DEFAULT_LOG_BUF, "./testThreadPool", ".log");
    ThreadPool threadpool(6);
    for(int i = 0; i < 18; i++) {
        threadpool.AddTask(std::bind(ThreadLogTask, i % 4, i * 10000));
    }
    while (true);
}

[[noreturn]] void TestTimer(){
    socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipeFd);
    HttpConn::m_ET = true;
    HttpConn::m_rootDir = nullptr;
    HttpConn::m_user_cnt = 0;
    Log::getInstance()->init(0, DEFAULT_MAX_LINE, 5000, DEFAULT_LOG_BUF, "./testTimer", ".log");
    unique_ptr<TimeWheel> m_timeWheel = unique_ptr<TimeWheel>(new TimeWheel(1));
    Epoller *epoller = Epoller::getEpollInstance();
    //unique_ptr<ClientData[]> m_usersTimer = unique_ptr<ClientData[]>(new ClientData[65536]);
    for (int i = 0; i < 65536; ++i){
        Timer *timer = m_timeWheel->add_timer(i % 180);
        timer->cb_func = cb_func1;
        timer->user_data = nullptr;
    }
    setNonblock(m_pipeFd[0]);
    epoller->addfd(m_pipeFd[0], EPOLLIN | EPOLLET);

    addsig(SIGALRM, sig_handler, false);
    alarm(1);
    while (true){
        int num = epoller->wait(-1);
        printf("wait\n");
        for (int i = 0; i < num; ++i) {
            int sockFd = epoller->getEpollFd(i);
            uint32_t events = epoller->getEpollEvents(i);
            if (sockFd == m_pipeFd[0] && (events & EPOLLIN)){
                m_timeWheel->tick();
                alarm(1);
            }
        }
    }
}

int main() {
    //TestLog();
    //TestThreadPool();
    TestTimer();
}


