//
// Created by chenshouyang on 2025/12/19.
//
#include "new/heap.h"
#include "global.h"
#include <cstdint>

//定义中断宏

#define LOCK()
#define UNLOCK()

HeapMgr::HeapMgr() {
    this->isInit.store(false);
}

void HeapMgr::init_heap() {
    // 双重检查 (Double Check) 避免重复初始化
    if (this->isInit.load(std::memory_order::memory_order_acquire)) return;

    LOCK();//锁住中断
    if (!this->isInit.load(std::memory_order::memory_order_relaxed)) {
        auto initialBlock = static_cast<void*>(&heap);
        auto* header = static_cast<BlockHeader*>(initialBlock);

        header->size = MAXSIZEOFHEAP - sizeof(BlockHeader);
        header->isFree = true;
        header->next = nullptr;
        header->prev = nullptr;

        this->isInit.store(true, std::memory_order::memory_order_release);
    }
    UNLOCK();
}

void *HeapMgr::allocate(std::size_t size) {
    if(!this->isInit.load(std::memory_order::memory_order_acquire)){
        this->init_heap();
    }

    // 1. 用户数据大小对齐到 8 字节
    size = (size + 7) & ~0x07;
    if (size == 0) return nullptr;

    // 确保 Header 占用的空间也是 8 的倍数，这样 (Header + Data) 之后的地址依然是对齐的
    constexpr std::size_t headerSize = (sizeof(BlockHeader) + 7) & ~0x07;

    LOCK();

    auto* current = reinterpret_cast<BlockHeader*>(&heap);
    while (current) {
        if (current->isFree && current->size >= size) {

            // 检查剩余空间是否足够分割：需要容纳一个新的 Header (对齐后) + 至少 8 字节数据
            if (current->size >= size + headerSize + 8) {
                // --- 分割块 (Split) ---

                // 计算新块的位置：当前地址 + 对齐后的Header大小 + 对齐后的数据大小
                auto* newBlock = reinterpret_cast<BlockHeader*>(
                        reinterpret_cast<char*>(current) + headerSize + size);

                // 新块的大小 = 原大小 - 数据大小 - 新块的Header大小
                // 注意：这里减去的是 headerSize，意味着我们在内存中留出了对齐填充的空间
                newBlock->size = current->size - size - headerSize;
                newBlock->isFree = true;
                newBlock->next = current->next;
                newBlock->prev = current;

                if (newBlock->next) {
                    newBlock->next->prev = newBlock;
                }

                current->size = size;
                current->isFree = false;
                current->next = newBlock;
            } else {
                // --- 不分割 (No Split) ---
                current->isFree = false;
            }

            UNLOCK();
            // 返回给用户的指针跳过对齐后的 headerSize
            return reinterpret_cast<void*>(reinterpret_cast<char*>(current) + headerSize);
        }
        current = current->next;
    }

    UNLOCK();
    return nullptr;
}


void HeapMgr::deallocate(void *ptr) {
    if (!ptr) return;

    LOCK(); // 进入临界区

    auto* header = reinterpret_cast<BlockHeader*>(
            reinterpret_cast<char*>(ptr) - sizeof(BlockHeader));
    header->isFree = true;

    // --- 向后合并 (Coalesce Next) ---
    if (header->next && header->next->isFree) {
        header->size += sizeof(BlockHeader) + header->next->size;
        header->next = header->next->next;
        if (header->next) {
            header->next->prev = header;
        }
    }

    // --- 向前合并 (Coalesce Prev) ---
    if (header->prev && header->prev->isFree) {
        header->prev->size += sizeof(BlockHeader) + header->size;
        header->prev->next = header->next;
        if (header->next) {
            header->next->prev = header->prev;
        }
    }

    UNLOCK(); // 退出临界区
}

HeapMgr::~HeapMgr() {
    // 无需特殊处理
}

HeapMgr globalHeapMgr;
