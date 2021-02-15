//
// Created by xclwt on 2021/1/30.
//



#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <pthread.h>
#include <queue>
#include "sync.h"

using namespace std;
template<class T>
class ThreadPool{
public:
    ThreadPool(int threadCount = 8);

    ~ThreadPool();

    void AddTask(T&& task);

private:
    bool isClosed;
    Locker m_locker;
    CondVar m_cond;
    queue<std::function<void()>> taskQueue;
};

#endif