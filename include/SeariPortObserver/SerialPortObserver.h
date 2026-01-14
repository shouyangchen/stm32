//
// Created by chenshouyang on 2026/1/12.
//

#ifndef SPLPROJECT_SEARIPORTOBSERVER_H
#define SPLPROJECT_SEARIPORTOBSERVER_H

#include "Object.h"
#include "SerialPort/SerialPort.h"
#include "singleton/Singleton.h"
#include <unordered_map>
#include <queue>
#include "Allocator/allocator.h"

// 观察者基类 - 对串口消息感兴趣的对象继承此类
class SerialObserver : public Object {
public:
    static constexpr ObjectType staticObjectType() {
        return ObjectType::Observer;
    }

    explicit SerialObserver(uint8_t priority, Object* parent = nullptr);
    
    // 处理串口数据的纯虚函数
    virtual void processSerialData(uint8_t portId, uint8_t source, const uint8_t* data, uint8_t len) = 0;
    
    void event(Event *event) override;
    [[nodiscard]] uint8_t getPriority() const;
    ~SerialObserver() override = default;

private:
    uint8_t priority_m;
};

// 观察者比较器（用于优先级队列）
struct ObserverCompare {
    bool operator()(SerialObserver* lhs, SerialObserver* rhs) const {
        if (!lhs || !rhs) return false;
        return lhs->getPriority() < rhs->getPriority();  // 数值越大优先级越高
    }
};

// 串口观察者管理器 - 单例模式
class SerialPortObserver : public Object, public Singleton<SerialPortObserver> {
public:
    static constexpr ObjectType staticObjectType() {
        return ObjectType::SerialPortObserver;
    }

    void event(Event *event) override;

    // 注册观察者到特定串口的特定消息源
    void registerObserver(SerialObserver* observer, uint8_t portId, uint8_t messageSource);
    
    // 注销观察者
    void unregisterObserver(SerialObserver* observer, uint8_t portId, uint8_t messageSource);
    
    // 分发接收到的消息给相关观察者
    void dispatchMessage(uint8_t portId, uint8_t source, const uint8_t* data, uint8_t len);

    ~SerialPortObserver() override = default;

private:
    friend class Singleton<SerialPortObserver>;
    SerialPortObserver();

    // 观察者键：(portId, messageSource)
    using ObserverKey = std::pair<uint8_t, uint8_t>;

    struct ObserverKeyHash {
        std::size_t operator()(const ObserverKey& key) const {
            return std::hash<uint8_t>()(key.first) ^ (std::hash<uint8_t>()(key.second) << 1);
        }
    };

    // 观察者映射表
    std::unordered_map<
        ObserverKey,
        std::priority_queue<SerialObserver*, std::vector<SerialObserver*, ::allocator<SerialObserver*>>, ObserverCompare>,
        ObserverKeyHash
    > m_observers;
};

#endif //SPLPROJECT_SEARIPORTOBSERVER_H