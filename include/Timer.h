//
// Created by chenshouyang on 2025/12/25.
//

#ifndef SPLPROJECT_TIMER_H
#define SPLPROJECT_TIMER_H
#include <Allocator/allocator.h>
#include <vector>
#include <queue>
#include <functional>

// 定义回调函数类型: void func(void* arg)
typedef void (*TimerCallback)(void*);

// 运用小根堆实现通用计时器API
struct Timer {
    uint32_t expire_time; // 下一次触发的绝对时间戳 (ticks)
    uint32_t interval;    // 重复间隔 (ticks)，0 表示一次性
    TimerCallback func;   // 回调函数
    void* arg;            // 用户参数

    // 优先队列默认是大顶堆，我们需要时间最小的在顶部。
    // 因此，当 this->expire_time > other.expire_time 时，
    // 我们认为 this 的优先级“更低”（即排在后面），返回 true。
    bool operator<(const Timer& other) const {
        return expire_time > other.expire_time;
    }
};

class TimerQueue {
public:
    // 定义获取时间戳的函数指针类型
    typedef uint32_t (*TickProvider)();

    // 构造函数，可选传入时间提供者
    explicit TimerQueue(TickProvider provider = nullptr) : tick_provider(provider) {}

    // 设置时间提供者
    void setTickProvider(TickProvider provider) {
        tick_provider = provider;
    }

    // 添加定时器
    // ms_delay: 首次触发延迟
    // interval: 重复周期 (0 = 一次性)
    // cb: 回调函数
    // data: 回调参数
    void add_timer(uint32_t ms_delay, uint32_t interval, TimerCallback cb, void* data) {
        if (!tick_provider) return;
        uint32_t current_tick = tick_provider();
        Timer t = {current_tick + ms_delay, interval, cb, data};
        pq.push(t);
    }

    // 调度函数，需在主循环或定时器中断中调用
    void tick() {
        if (!tick_provider) return;
        uint32_t current_tick = tick_provider();

        while (!pq.empty()) {
            // 获取堆顶（最近过期的定时器）
            Timer t = pq.top();

            // 检查是否过期
            // 使用 (int32_t) 强转处理 uint32_t 溢出回绕问题
            // 如果 (current - expire) >= 0，说明时间已到
            if ((int32_t)(current_tick - t.expire_time) >= 0) {
                pq.pop();

                // 执行回调
                if (t.func) {
                    t.func(t.arg);
                }

                // 如果是重复定时器，更新时间并重新入队
                if (t.interval > 0) {
                    t.expire_time += t.interval; // 保持时间相位，避免漂移
                    pq.push(t);
                }
            } else {
                // 堆顶未过期，说明后面的也没过期，直接退出
                break;
            }
        }
    }

    [[nodiscard]] bool empty() const {
        return pq.empty();
    }

private:
    TickProvider tick_provider;
    // 使用自定义分配器的 vector 作为底层容器
    using TimerContainer = std::vector<Timer, allocator<Timer>>;
    // 标准优先队列
    std::priority_queue<Timer, TimerContainer> pq;
};

#endif //SPLPROJECT_TIMER_H