//
// Created by xclwt on 2021/1/30.
//

#include "queue.h"

template<class T>
BlockQueue<T>::BlockQueue(int max_size){
    assert(max_size > 0);

    m_max_size = max_size;
    m_size = 0;
    m_front = -1;
    m_back = -1;
    m_array = unique_ptr<string[]>(new string[m_max_size]);
}

template<class T>
BlockQueue<T>::~BlockQueue<T>(){

}

template<class T>
bool BlockQueue<T>::full(){
    if (m_size >= m_max_size){
        return true;
    }

    return false;
}

template<class T>
bool BlockQueue<T>::empty(){
    if (m_size == 0){
        return true;
    }

    return false;
}

template<class T>
int BlockQueue<T>::size(){
    return m_size;
}

template<class T>
void BlockQueue<T>::push(const T &item){
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

template<class T>
void BlockQueue<T>::pop(T &item){
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