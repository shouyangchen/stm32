//
// Created by chenshouyang on 2025/12/19.
//

#include "EventQueue/GlobalEventQueue.h"
#include "new/new.h"

void GlobalEventQueue::enqueueEvent(Event *event) {
    if (event) {
        this->queue.push(event);
    }
}

Event* GlobalEventQueue::dequeueEvent() {
    if (!this->queue.empty()) {
        const auto event=this->queue.top();
        this->queue.pop();
        return event;
    }
    return nullptr;
}

bool GlobalEventQueue::isEmpty() const {
    return this->queue.empty();
}

GlobalEventQueue globalEventQueue;