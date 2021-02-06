//
// Created by xclwt on 2021/1/30.
//

#include "timer.h"

RotationNode::RotationNode(int rot): rotation(rot), prev(nullptr), next(nullptr), start(nullptr){}

RotationNode::~RotationNode(){
    this->deleteAll();
}

void RotationNode::addTimer(Timer *timer){
    if (this->start)
        this->start->prev = timer;

    timer->next = this->start;
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

Timer::Timer(int ts): next(nullptr), prev(nullptr), timeSlot(ts){}

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

void TimeWheel::add_timer(int timeout){
    if (timeout < 0)
        return;

    int ticks = 0;

    if (timeout < INTERVAL)
        ticks = 1;
    else
        ticks = timeout / INTERVAL;

    int idx = (cur_slot + ticks) % N;
    int rotation = ticks / N;
    auto *newTimer = new Timer(idx);

    if (!slots[idx]) {
        auto *newRotation = new RotationNode(rotation);
        slots[idx] = newRotation;
        newRotation->addTimer(newTimer);
        return;
    }

    RotationNode *tmp = slots[idx];

    while (tmp){
        if (rotation < tmp->rotation){
            auto *newRotation = new RotationNode(rotation);

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
        }
        else{
            if (tmp->next){
                tmp = tmp->next;
                continue;
            }

            tmp->next = new RotationNode(rotation);
            tmp->next->addTimer(newTimer);
            break;
        }
    }
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