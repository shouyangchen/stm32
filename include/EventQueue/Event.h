//
// Created by chenshouyang on 2025/12/19.
//

#ifndef F103C8_EVENT_H
#define F103C8_EVENT_H
#include <cstdint>

class Object;//前向声明
//全局事件抽象类
class Event {
public:
    enum EventType {
        EventType_None = 0,
        EventType_KeyDown,
        EventType_TemperatureChange,
        EventType_LedFlash,
        EventType_TimerOver,
        EventType_SerialProtReceive,//串口接收到新的数据
        EventType_SerialProtSend,//通过串口发送新的数据
        EventType_BlueToothReceive,//通过蓝牙接收到数据
        EventType_BlueToothSend,//通过蓝牙发送数据
        EventType_ASRGoForward,
        EventType_ASRGoBack,
        EventType_ASRGoLeft,
        EventType_ASRGoRight,
        EventType_DeleteObject, // 延迟删除对象事件
    };
    uint8_t Priority{};
    Event()=default;
    Event(EventType type, Object *object);
    virtual ~Event()=default;
    [[nodiscard]] EventType getEventType()const;
    [[nodiscard]] Object* getReciver()const;
    private:
    EventType type_m;
    Object* reciver_m;//事件接收者
};

#endif //F103C8_EVENT_H