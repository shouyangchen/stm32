//
// Created by chenshouyang on 2025/12/19.
//
//该类主要是实现该系列的事件循环
//要创建一个全局的静态事件队列
#ifndef F103C8_APPLICATIONS_H
#define F103C8_APPLICATIONS_H
#include <bits/ios_base.h>

#include "global.h"
#include "EventQueue/GlobalEventQueue.h"
#include "Object.h"
#include "FuncHelper/FuncHelper.h"
#include "Timer.h"


class Applications;

void initApplicationsInstance(Applications*app);

void initEventQueue();

class Applications {
private:
    friend class Object;
    friend class Event;
    static std::atomic<bool> isQuit;
    static Applications* instance;
    static GlobalEventQueue* eventQueue;
    static TimerQueue* timerQueue;
    friend void initApplicationsInstance(Applications*app);
    friend void initEventQueue();
    friend void initTimerQueue();
public:
    Applications();
    int static exec();
    static void postEvent(Event *event) ;
    static void sendEvent(Event *event,Object*reciver) ;
    static Applications* getInstance();
    static void processEvent();//主动消费事件防止在耗时业务导致队列堆积
    static uint32_t getTick();
    static void addTimer(uint32_t ms_delay, uint32_t interval, TimerCallback cb, void* data);
    static void sleep(uint32_t delay);//休眠并且阻塞线程
    ~Applications();
};



#endif //F103C8_APPLICATIONS_H
