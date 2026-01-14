//
// Created by chenshouyang on 2026/1/12.
//

#include "SeariPortObserver/SerialPortObserver.h"
#include "Applications.h"

// SerialObserver 实现
SerialObserver::SerialObserver(uint8_t priority, Object* parent)
    : Object(parent, ObjectType::Observer)
    , priority_m(priority) {
}

uint8_t SerialObserver::getPriority() const {
    return this->priority_m;
}

void SerialObserver::event(Event *event) {
    if (event->getEventType() == Event::EventType_SerialProtReceive) {
        // 处理串口接收事件
        SerialReceiveEvent* serialEvt = static_cast<SerialReceiveEvent*>(event);
        processSerialData(serialEvt->port_id, 
                         serialEvt->message.source,
                         serialEvt->message.data,
                         serialEvt->message.length);
    }
    Object::event(event);
}

// SerialPortObserver 实现
SerialPortObserver::SerialPortObserver()
    : Object(nullptr, ObjectType::SerialPortObserver) {
}

void SerialPortObserver::event(Event *event) {
    if (event->getEventType() == Event::EventType_SerialProtReceive) {
        // 接收到串口消息事件，分发给相关观察者
        SerialReceiveEvent* serialEvt = static_cast<SerialReceiveEvent*>(event);
        dispatchMessage(serialEvt->port_id,
                       serialEvt->message.source,
                       serialEvt->message.data,
                       serialEvt->message.length);
    }
    Object::event(event);
}

void SerialPortObserver::registerObserver(SerialObserver* observer, uint8_t portId, uint8_t messageSource) {
    if (!observer) return;

    ObserverKey key{portId, messageSource};
    auto it = m_observers.find(key);

    if (it != m_observers.end()) {
        it->second.push(observer);
    } else {
        std::priority_queue<SerialObserver*, std::vector<SerialObserver*, ::allocator<SerialObserver*>>, ObserverCompare> newQueue;
        newQueue.push(observer);
        m_observers.insert({key, std::move(newQueue)});
    }
}

void SerialPortObserver::unregisterObserver(SerialObserver* observer, uint8_t portId, uint8_t messageSource) {
    if (!observer) return;

    ObserverKey key{portId, messageSource};
    auto it = m_observers.find(key);

    if (it != m_observers.end()) {
        // 重建队列，排除要删除的观察者
        std::priority_queue<SerialObserver*, std::vector<SerialObserver*, ::allocator<SerialObserver*>>, ObserverCompare> tempQueue;

        while (!it->second.empty()) {
            auto obs = it->second.top();
            if (obs != observer) {
                tempQueue.push(obs);
            }
            it->second.pop();
        }

        it->second = std::move(tempQueue);
    }
}

void SerialPortObserver::dispatchMessage(uint8_t portId, uint8_t source, const uint8_t* data, uint8_t len) {
    ObserverKey key{portId, source};
    auto it = m_observers.find(key);

    if (it != m_observers.end() && !it->second.empty()) {
        // 创建临时队列的副本，这样可以遍历所有观察者
        auto tempQueue = it->second;

        while (!tempQueue.empty()) {
            auto observer = tempQueue.top();
            if (observer) {
                observer->processSerialData(portId, source, data, len);
            }
            tempQueue.pop();
        }
    }
}