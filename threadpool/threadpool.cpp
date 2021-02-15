//
// Created by xclwt on 2021/1/30.
//

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>
#include "threadpool.h"

template<class T>
ThreadPool<T>::ThreadPool(int threadCount){
    assert(threadCount > 0);
    for(int i = 0; i < threadCount; i++){
        std::thread([=]{
            m_locker.lock();

            while(true){
                if(!taskQueue.empty()){
                    auto task = std::move(taskQueue.front());
                    taskQueue.pop();
                    m_locker.unlock();
                    task();
                    m_locker.lock();
                }else if(isClosed)
                    break;
                else
                    this->cond.wait(m_locker.get());
            }
        }).detach();
    }
}

template<class T>
ThreadPool<T>::~ThreadPool<T>(){
    this->m_locker.lock();
    this->isClosed = true;
    this->m_locker.unlock();
    this->m_cond.broadcast();
}

template<class T>
void ThreadPool<T>::AddTask(T &&task){
    this->m_locker.lock();
    this->taskQueue.emplace(std::forward<T>(task));
    this->m_locker.unlock();
    this->m_cond.broadcast();
}
