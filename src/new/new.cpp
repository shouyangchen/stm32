//
// Created by chenshouyang on 2025/12/19.
//
#include "new/new.h"
#include "global.h"
void * operator new(std::size_t size){
    return globalHeapMgr.allocate(size);
}
void * operator new[](std::size_t size){
    return globalHeapMgr.allocate(size);
}
void operator delete[](void * ptr){
    globalHeapMgr.deallocate(ptr);
}
void operator delete( void * ptr){
    globalHeapMgr.deallocate(ptr);
}
