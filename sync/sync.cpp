//
// Created by xclwt on 2021/1/30.
//

#include "sync.h"

Semaphore::Semaphore(){
    if(sem_init(&sem, 0, 0) != 0){
        throw std::exception();
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&sem);
}

bool Semaphore::wait(){
    return sem_wait(&sem) == 0;
}

bool Semaphore::post(){
    return sem_post(&sem) == 0;
}

Locker::Locker(){
    if(pthread_mutex_init(&mutex, nullptr) != 0){
        throw std::exception();
    }
}

Locker::~Locker(){
    pthread_mutex_destroy(&mutex);
}

bool Locker::lock(){
    return pthread_mutex_lock(&mutex) == 0;
}

bool Locker::unlock(){
    return pthread_mutex_unlock(&mutex) == 0;
}

pthread_mutex_t* Locker::get(){
    return &mutex
}

CondVar::CondVar(){
    if(pthread_cond_init(&cond, nullptr) != 0){
        throw std::exception();
    }
}

CondVar::~CondVar(){
    pthread_cond_destroy(&cond);
}

bool CondVar::wait(pthread_mutex_t *mutex){
    int ret;

    ret = pthread_cond_wait( &cond, mutex );

    return ret == 0;
}

bool CondVar::signal(){
    return pthread_cond_signal(&cond) == 0;
}

bool CondVar::broadcast(){
    return pthread_cond_broadcast(&cond) == 0;
}