//
// Created by xclwt on 2021/1/30.
//

#ifndef TIMER_H
#define TIMER_H

#include <ctime>
#include <netinet/in.h>
#include <cstdio>

#define BUFFER_SIZE 64

class Timer;

struct ClientData{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    Timer* timer;
};

class RotationNode{
public:
    explicit RotationNode(int rot);

    ~RotationNode();

    void addTimer(Timer *timer);

    void decRotation();

    void deleteAll();

public:
    int rotation;
    RotationNode *prev;
    RotationNode *next;
    Timer *start;
};

class Timer{
public:
    explicit Timer(int ts);

public:
    int timeSlot;
    void (*cb_func)(ClientData*);
    ClientData* user_data;
    Timer* next;
    Timer* prev;
};

class TimeWheel{
public:
    TimeWheel();

    TimeWheel(int accuracy);

    ~TimeWheel();

    void add_timer(int timeout);

    void tick();

private:
    static const int N = 60;
    const int INTERVAL;
    RotationNode* slots[N];
    int cur_slot;
};

#endif