//
// Created by chenshouyang on 2025/12/19.
//

#ifndef F103C8_NEW_H
#define F103C8_NEW_H

#include <cstdio>

class HeapMgr;
//本头文件需要实现自己的new以实现各种数据结构否者无法实现事件队列

extern HeapMgr globalHeapMgr;

void * operator new(std::size_t size);// 普通 new

void * operator new[](std::size_t size);// 数组 new

void operator delete[](void *);// 数组 delete

void operator delete( void *);// 普通 delete

#endif //F103C8_NEW_H