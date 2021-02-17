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
    explicit BlockQueue(int max_size){
        assert(max_size > 0);

        m_max_size = max_size;
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_array = unique_ptr<string[]>(new string[m_max_size]);
    }

    ~BlockQueue(){
    }

    bool full(){
        if (m_size >= m_max_size){
            return true;
        }

        return false;
    }

    bool empty(){
        if (m_size == 0){
            return true;
        }

        return false;
    }

    int size(){
        return m_size;
    }

    void push(const T &item){
        m_locker.lock();

        while (m_size == m_max_size){
            m_cond_producer.wait(m_locker.get());
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond_consumer.signal();
        m_locker.unlock();
    }

    void pop(T &item){
        m_locker.lock();

        while (m_size == 0){
            m_cond_consumer.wait(m_locker.get());
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_cond_producer.signal();
        m_locker.unlock();
    }

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
