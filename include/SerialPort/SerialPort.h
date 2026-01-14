/**
 * @file SerialPort.h
 * @brief STM32F10x 事件驱动串口系统
 * 
 * 基于DMA的非阻塞串口收发，支持消息协议解析和观察者模式
 * 
 * 消息格式: [源ID(1字节)][长度(1字节)][数据(N字节)]
 * 
 * 使用示例:
 *   // 1. 创建观察者类
 *   class MyObserver : public SerialObserver {
 *       void processSerialData(uint8_t portId, uint8_t source, 
 *                             const uint8_t* data, uint8_t len) override {
 *           // 处理接收到的数据
 *       }
 *   };
 * 
 *   // 2. 注册观察者
 *   MyObserver* obs = new MyObserver();
 *   SerialPortObserver::getInstance().registerObserver(obs, 1, 0x01);
 * 
 *   // 3. 发送数据
 *   uint8_t msg[] = {0x01, 0x05, 'H', 'e', 'l', 'l', 'o'};
 *   serial1->sendData(msg, sizeof(msg));
 */

#ifndef SIMPLE_SERIAL_H
#define SIMPLE_SERIAL_H

#include "stm32f10x.h"
#include <cstring>
#include "Object.h"
#include "EventQueue/Event.h"

// 串口消息结构：[source_byte][length_byte][data...]
struct SerialMessage {
    uint8_t source;      // 消息源标识符
    uint8_t length;      // 数据长度
    uint8_t data[254];   // 数据内容（最大254字节）
    
    SerialMessage() : source(0), length(0) {
        memset(data, 0, sizeof(data));
    }
};

// 串口接收事件
class SerialReceiveEvent : public Event {
public:
    SerialMessage message;
    uint8_t port_id;  // 串口ID (1, 2, 3)
    
    SerialReceiveEvent(Object* receiver, uint8_t portId, const SerialMessage& msg)
        : Event(EventType_SerialProtReceive, receiver)
        , message(msg)
        , port_id(portId) {
        Priority = 2;  // 中等优先级
    }
};

// 串口发送完成事件
class SerialSendCompleteEvent : public Event {
public:
    uint8_t port_id;
    
    SerialSendCompleteEvent(Object* receiver, uint8_t portId)
        : Event(EventType_SerialProtSend, receiver)
        , port_id(portId) {
        Priority = 1;  // 低优先级
    }
};

class SimpleSerial : public Object {
public:
    enum class USART_ID {
        USART1_ID,
        USART2_ID,
        USART3_ID
    };

    static constexpr ObjectType staticObjectType() {
        return ObjectType::SerialPort;
    }

    SimpleSerial(USART_ID id, uint32_t baud);
    ~SimpleSerial();

    void sendByte(uint8_t byte);
    void sendString(const char* str);
    void sendData(const uint8_t* data, uint16_t len);

    bool received();
    uint8_t readByte();
    uint16_t readData(uint8_t* buffer, uint16_t max_len);

    // 事件处理
    void event(Event* e) override;

    // 中断服务调用的函数
    void handleIdleInterrupt();
    void handleDMATxComplete();

    // ✅ 改为 public（中断需要访问）
    USART_TypeDef* m_usart;
    USART_ID m_id;
    DMA_Channel_TypeDef* m_dma_tx;
    DMA_Channel_TypeDef* m_dma_rx;

    volatile uint8_t m_rx_buffer[256];
    volatile uint16_t m_rx_index;
    volatile uint16_t m_rx_count;

private:
    void init(uint32_t baud);
    void initGPIO();
    void initUSART(uint32_t baud);
    void initDMA();
    void initNVIC();
    
    void parseAndPostMessage();
    bool m_tx_in_progress;
};

extern SimpleSerial* g_serial1;
extern SimpleSerial* g_serial2;
extern SimpleSerial* g_serial3;

extern "C" {
    void USART1_IRQHandler(void);
    void USART2_IRQHandler(void);
    void USART3_IRQHandler(void);
    void DMA1_Channel4_IRQHandler(void);
    void DMA1_Channel7_IRQHandler(void);
}

#endif