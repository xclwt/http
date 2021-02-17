//
// Created by xclwt on 2021/1/30.
//

#include <mutex>
#include <condition_variable>

#include "threadpool.h"


ThreadPool::ThreadPool(int threadCount){
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
                    m_cond.wait(m_locker.get());
            }
        }).detach();
    }
}


ThreadPool::~ThreadPool(){
    m_locker.lock();
    isClosed = true;
    m_locker.unlock();
    m_cond.broadcast();
}

