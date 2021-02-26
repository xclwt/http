//
// Created by xclwt on 2021/1/30.
//

#ifndef TIMER_H
#define TIMER_H

#include <ctime>
#include <unistd.h>
#include <netinet/in.h>
#include <cstdio>
#include "httpConn.h"
#include "epoller.h"

#define BUFFER_SIZE 64

class Timer;

struct ClientData{
    sockaddr_in address;
    int sockfd;
    Timer* timer;
};

class RotationNode{
public:
    RotationNode(int rot, int ts);

    ~RotationNode();

    void addTimer(Timer *timer);

    void decRotation();

    void deleteAll();

public:
    int rotation;
    int timeSlot;
    RotationNode *prev;
    RotationNode *next;
    Timer *start;
};

class Timer{
public:
    Timer();

public:
    RotationNode *rot;
    void (*cb_func)(ClientData*);
    ClientData* user_data;
    Timer* next;
    Timer* prev;
};

class TimeWheel{
public:
    TimeWheel();

    explicit TimeWheel(int accuracy);

    ~TimeWheel();

    Timer* add_timer(int timeout);

    void del_timer(Timer *timer);

    void adjust_timer(Timer *timer, int timeout);

    void tick();

private:
    static const int N = 60;
    const int INTERVAL;
    RotationNode* slots[N];
    int cur_slot;
};

void cb_func(ClientData *user_data);

#endif