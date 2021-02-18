//
// Created by xclwt on 2021/1/30.
//

#include "timer.h"

RotationNode::RotationNode(int rot, int ts): rotation(rot), timeSlot(ts), prev(nullptr), next(nullptr), start(nullptr){}

RotationNode::~RotationNode(){
    this->deleteAll();
}

void RotationNode::addTimer(Timer *timer){
    if (this->start)
        this->start->prev = timer;

    timer->next = this->start;
    timer->rot = this;
    this->start = timer;
}

void RotationNode::decRotation(){
    --rotation;
}

void RotationNode::deleteAll(){
    Timer *tmp = this->start;

    while (tmp){
        this->start = tmp->next;
        tmp->cb_func(tmp->user_data);
        delete tmp;
        tmp = this->start;
    }
}

Timer::Timer(): next(nullptr), prev(nullptr){}

TimeWheel::TimeWheel(): TimeWheel(1){}

TimeWheel::TimeWheel(int accuracy): INTERVAL(accuracy), cur_slot(0){
    for (auto & slot : slots) {
        slot = nullptr;
    }
}

TimeWheel::~TimeWheel(){
    for (auto & slot : slots) {
        RotationNode *tmp = slot;

        while (tmp){
            slot = tmp->next;
            delete tmp;
            tmp = slot;
        }
    }
}

Timer* TimeWheel::add_timer(int timeout){
    if (timeout < 0)
        return nullptr;

    int ticks = 0;

    if (timeout < INTERVAL)
        ticks = 1;
    else
        ticks = timeout / INTERVAL;

    int idx = (cur_slot + ticks) % N;
    int rotation = ticks / N;
    auto *newTimer = new Timer();

    if (!slots[idx]) {
        auto *newRotation = new RotationNode(rotation, idx);
        slots[idx] = newRotation;
        newRotation->addTimer(newTimer);
        return newTimer;
    }

    RotationNode *tmp = slots[idx];

    while (tmp){
        if (rotation < tmp->rotation){
            auto *newRotation = new RotationNode(rotation, idx);

            newRotation->addTimer(newTimer);

            if (tmp == slots[idx]){
                slots[idx] = newRotation;
            }else{
                newRotation->prev = tmp->prev;
                tmp->prev->next = newRotation;
            }

            newRotation->next = tmp;
            tmp->prev = newRotation;
        }else if (rotation == tmp->rotation){
            tmp->addTimer(newTimer);
            break;
        }
        else{
            if (tmp->next){
                tmp = tmp->next;
                continue;
            }

            tmp->next = new RotationNode(rotation, idx);
            tmp->next->addTimer(newTimer);
            break;
        }
    }

    return newTimer;
}

void TimeWheel::del_timer(Timer *timer){
    if (timer->prev)
        timer->prev->next = timer->next;
    else
        timer->rot->start = timer->next;

    delete timer;
}

void TimeWheel::adjust_timer(Timer *timer, int timeout){
    Timer *newtimer = add_timer(timeout);
    newtimer->user_data = timer->user_data;
    newtimer->cb_func = timer->cb_func;
    newtimer->user_data->timer = newtimer;
    del_timer(timer);
}

void TimeWheel::tick(){
    RotationNode *tmp = slots[cur_slot];

    while (tmp){
        if (tmp->rotation == 0){
            tmp->deleteAll();
            slots[cur_slot] = tmp->next;
            delete tmp;
            tmp = slots[cur_slot];
            continue;
        }

        tmp->decRotation();
        tmp = tmp->next;
    }

    cur_slot = ++cur_slot % N;
}

void cb_func(ClientData *user_data)
{
    Epoller::getEpollInstance()->removefd(user_data->sockfd);
    //assert(user_data);
    close(user_data->sockfd);
    HttpConn::m_user_cnt--;
}