//
// Created by xclwt on 2021/1/30.
//

#include "threadpool.h"


ThreadPool::ThreadPool(int threadCount){
    assert(threadCount > 0);
    for(int i = 0; i < threadCount; i++){
        std::thread([=]{
            unique_lock<mutex> locker(m_locker);

            while(true){
                if(!taskQueue.empty()){
                    auto task = std::move(taskQueue.front());
                    taskQueue.pop();
                    locker.unlock();
                    task();
                    locker.lock();
                }else if(m_isClosed)
                    break;
                else
                    m_cond.wait(locker);
            }
        }).detach();
    }
}


ThreadPool::~ThreadPool(){
    unique_lock<mutex> locker(m_locker);
    m_isClosed = true;
    locker.unlock();
    m_cond.notify_all();
}

