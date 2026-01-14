//
// Created by chenshouyang on 2025/12/19.
//

#ifndef F103C8_GLOBAL_H
#define F103C8_GLOBAL_H
#include "new/new.h"
#include "new/heap.h"
#include <atomic>
#include "EventQueue/Event.h"
#include "EventQueue/GlobalEventQueue.h"


extern HeapMgr globalHeapMgr;
extern GlobalEventQueue globalEventQueue;
#endif //F103C8_GLOBAL_H