//
// Created by chenshouyang on 2025/12/19.
//

#ifndef F103C8_GLOBALEVENTQUEUE_H
#define F103C8_GLOBALEVENTQUEUE_H
#include <atomic>
#include <queue>

#include "Event.h"
#include <vector>
#include <utility>

#include "Allocator/allocator.h"

struct EventPriorityCompare {
    bool operator()(Event* lhs,Event*rhs){
        if (!lhs||!rhs)
            return false;
        return lhs->Priority>rhs->Priority;
    }
};

class GlobalEventQueue {
public:
    GlobalEventQueue()=default;
    using EventVector=std::vector<Event*,::allocator<Event*>>;
    void enqueueEvent(Event*event);
    Event* dequeueEvent();
    [[nodiscard]] bool isEmpty() const;
    ~GlobalEventQueue()=default;
private:
    std::priority_queue<Event*,EventVector,EventPriorityCompare> queue;
};


#endif //F103C8_GLOBALEVENTQUEUE_H