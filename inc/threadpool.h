//
// Created by xclwt on 2021/1/30.
//



#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <cassert>
#include "sync.h"

using namespace std;


class ThreadPool{
public:
    ThreadPool(int threadCount = 8);

    ~ThreadPool();

    template<class T>
    void AddTask(T&& task){
        m_locker.lock();
        taskQueue.emplace(std::forward<T>(task));
        m_locker.unlock();
        m_cond.broadcast();
    }

private:
    bool isClosed;
    Locker m_locker;
    CondVar m_cond;
    queue<std::function<void()>> taskQueue;
};

#endif