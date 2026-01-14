//
// Created by chenshouyang on 2025/12/19.
//

#ifndef F103C8_HEAP_CPP_H
#define F103C8_HEAP_CPP_H
#define MAXSIZEOFHEAP 6144
#include <cinttypes>
#include <atomic>
class HeapMgr {//实现一个堆
private:
    alignas(8) char heap[MAXSIZEOFHEAP];//因为Arm的内存对齐要求是8字节对齐所以让heap数组在8字节内存地址对其的地方开辟这样得到的地则址就是8字节对齐的
    void init_heap();
    std::atomic<bool> isInit{false};//是否初始化
public:
    struct BlockHeader {
        std::size_t size;
        BlockHeader* next;
        BlockHeader* prev;
        bool isFree;
        BlockHeader() {
            isFree = false;
            size = 0;
            next = nullptr;
            prev = nullptr;
        }
    };
    HeapMgr();
    void* allocate(std::size_t size);
    void deallocate(void* ptr);
    ~HeapMgr();
    friend void* operator new(std::size_t size);
    friend void* operator new[](std::size_t size);
    friend void operator delete[](void* );
    friend void operator delete( void* );

};


#endif //F103C8_HEAP_CPP_H