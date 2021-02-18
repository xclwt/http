//
// Created by xclwt on 2021/1/30.
//



#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
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
        unique_lock<mutex> locker(m_locker);
        taskQueue.emplace(std::forward<T>(task));
        locker.unlock();
        m_cond.notify_all();
    }

private:
    bool m_isClosed;
    mutex m_locker;
    condition_variable m_cond;
    queue<std::function<void()>> taskQueue;
};

#endif