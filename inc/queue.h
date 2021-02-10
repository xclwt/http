//
// Created by xclwt on 2021/1/30.
//

#ifndef HTTP_QUEUE_H
#define HTTP_QUEUE_H

#include "sync.h"
#include <cassert>
#include <string>
#include <memory>

using namespace std;

template<class T>
class BlockQueue{
public:
    explicit BlockQueue(int max_size);

    ~BlockQueue();

    bool full();

    bool empty();

    int size();

    void push(const T &item);

    void pop(T &item);

private:
    unique_ptr<string[]> m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;

    Locker m_locker;
    CondVar m_cond_consumer;
    CondVar m_cond_producer;
};

#endif
