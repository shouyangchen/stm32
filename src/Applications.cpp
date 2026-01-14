//
// Created by chenshouyang on 2025/12/19.
//

#include "Applications.h"
#include "SerialPort/SerialPort.h"
#include "stm32f10x_pwr.h"
#include "SeariPortObserver/SerialPortObserver.h"

extern "C" {
    extern volatile uint32_t uwTick;
}

static void changeState(void*arg) {
    *(static_cast<bool*>(arg))=true;
}

SimpleSerial* serial1 = nullptr;
SimpleSerial* serial2 = nullptr;
SimpleSerial* serial3 = nullptr;
SerialPortObserver* serialPortObserver = nullptr;

void setupSerial() {
    serial1 = new SimpleSerial(SimpleSerial::USART_ID::USART1_ID, 115200);
    serial2 = new SimpleSerial(SimpleSerial::USART_ID::USART2_ID, 115200);
    serial3 = new SimpleSerial(SimpleSerial::USART_ID::USART3_ID, 115200);

    // 初始化串口观察者管理器
    serialPortObserver = &SerialPortObserver::getInstance();

    if (serial1) serial1->sendString("USART1 Ready!\r\n");
    if (serial2) serial2->sendString("USART2 Ready!\r\n");
    if (serial3) serial3->sendString("USART3 Ready!\r\n");
}

void sendHeartbeat(void*) {
    if (serial1) serial1->sendString("Heartbeat from STM32!\r\n");
    Applications::addTimer(2000, 0, sendHeartbeat, nullptr);
}


Applications* Applications::instance=nullptr;

std::atomic<bool> Applications::isQuit{false};

GlobalEventQueue* Applications::eventQueue=nullptr;
TimerQueue* Applications::timerQueue=nullptr;


void initApplicationsInstance(Applications*app) {
   if (Applications::instance==nullptr) {
        Applications::instance=app;
   }
}

void initEventQueue() {
    if (Applications::eventQueue==nullptr) {
        Applications::eventQueue=&globalEventQueue;
    }
}

void initTimerQueue() {
    if (Applications::timerQueue == nullptr) {
        // 使用 placement new 或者直接 new，这里假设已经有全局分配器支持
        Applications::timerQueue = new TimerQueue(Applications::getTick);
    }
}

Applications::Applications() {
    initApplicationsInstance(this);
    initEventQueue();
    initTimerQueue();
    // 配置 SysTick 为 1ms 中断
    // SystemCoreClock 是系统时钟频率，/1000 表示每秒 1000 次中断
    if (SysTick_Config(SystemCoreClock / 1000)) {
        // Capture error
        while (1);
    }
    setupSerial();
    sendHeartbeat(nullptr);
    // initInterputer();
    // Initialize Serial Port ONCE
}

int Applications::exec() {
    while (! isQuit. load()) {
        if (timerQueue) {
            timerQueue->tick();
        }
        processEvent();
    }
    return 0;
}

Applications* Applications::getInstance() {
    return Applications::instance;
}

void Applications::processEvent() {
    while (!eventQueue->isEmpty()) {
        if (Event* event_m = eventQueue->dequeueEvent()) {
            if (event_m->getEventType() == Event::EventType_DeleteObject) {
                 delete event_m->getReciver();
            } else if (event_m->getEventType() == Event::EventType_SerialProtReceive) {
                // 串口接收事件，转发给SerialPortObserver处理
                if (serialPortObserver) {
                    serialPortObserver->event(event_m);
                }
            } else {
                if (const auto receiver = event_m->getReciver())
                    receiver->event(event_m);
            }
            delete event_m;
        }
    }
}

void Applications::postEvent(Event *event) {
    if (event) {
        globalEventQueue.enqueueEvent(event);
    }
}

void Applications::sendEvent(Event *event, Object *reciver) {
    reciver->event(event);
}

uint32_t Applications::getTick() {
    return uwTick;
}

void Applications::addTimer(uint32_t ms_delay, uint32_t interval, TimerCallback cb, void* data) {
    if (timerQueue) {
        timerQueue->add_timer(ms_delay, interval, cb, data);
    }
}


void Applications::sleep(uint32_t delay) {
    bool finished = false;
    uint32_t start_tick = getTick();
    Applications::addTimer(delay, 0, changeState, static_cast<void*>(&finished));
    while (!finished) {
        // 优先检查定时器状态
        if (timerQueue) {
            timerQueue->tick();
        }
        // 非阻塞式处理事件
        int processed_events = 0;
        constexpr int MAX_EVENTS_PER_CYCLE = 2; // 限制每次处理的事件数量
        while (!eventQueue->isEmpty() && processed_events < MAX_EVENTS_PER_CYCLE) {
            if (Event* event_m = eventQueue->dequeueEvent()) {
                if (event_m->getEventType() == Event::EventType_DeleteObject) {
                    delete event_m->getReciver();
                } else if (event_m->getEventType() == Event::EventType_SerialProtReceive) {
                    // 串口接收事件，转发给SerialPortObserver处理
                    if (serialPortObserver) {
                        serialPortObserver->event(event_m);
                    }
                } else {
                    if (const auto receiver = event_m->getReciver())
                        receiver->event(event_m);
                }
                delete event_m;
                processed_events++;
            }
        }
        // 检查是否已达到睡眠时间
        if ((getTick() - start_tick) >= delay) {
            break;
        }
    }
}


Applications::~Applications() {
    if (timerQueue) {
        delete timerQueue;
        timerQueue = nullptr;
    }
}