//
// Created by xclwt on 2021/1/30.
//

#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class Semaphore{
public:
    Semaphore();

    ~Semaphore();

    bool wait();

    bool post();

private:
    sem_t sem;
};

class Locker{
public:
    Locker();

    ~Locker();

    bool lock();

    bool unlock();

    pthread_mutex_t *get();

private:
    pthread_mutex_t mutex;
};

class CondVar{
public:
    CondVar();

    ~CondVar();

    bool wait(pthread_mutex_t *mutex);

    bool signal();

    bool broadcast();

private:
    pthread_cond_t cond;
};

#endif