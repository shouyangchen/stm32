# STM32F10x 串口事件驱动系统使用文档

## 概述

本系统实现了基于事件循环的串口数据处理框架，支持DMA非阻塞传输，并采用观察者模式处理不同消息源的数据。

## 核心特性

1. **事件驱动架构**：所有串口接收通过事件队列异步处理
2. **DMA非阻塞传输**：使用DMA进行数据收发，不影响主循环
3. **消息协议解析**：自动解析消息格式 `[源ID][长度][数据...]`
4. **观察者模式**：支持多个观察者监听不同消息源
5. **优先级管理**：观察者按优先级顺序处理消息
6. **多串口支持**：支持USART1/2/3同时工作互不干扰

## 消息协议格式

所有通过串口发送的消息必须遵循以下格式：

```
[源ID(1字节)][长度(1字节)][数据(N字节)]
```

### 字段说明

- **源ID (Source Byte)**：标识消息来源（0x00-0xFF）
  - 0x01: GPS数据
  - 0x02: 蓝牙数据
  - 0x03: 传感器数据
  - 0x04-0xFF: 自定义消息源

- **长度 (Length Byte)**：数据部分的字节数（0-254）

- **数据 (Data)**：实际的消息内容

### 示例

发送GPS数据 "Hello":
```
0x01 0x05 'H' 'e' 'l' 'l' 'o'
```

## 使用方法

### 1. 创建观察者类

继承 `SerialObserver` 并实现 `processSerialData` 方法：

```cpp
class GPSObserver : public SerialObserver {
public:
    GPSObserver() : SerialObserver(10) {}  // 优先级10
    
    void processSerialData(uint8_t portId, uint8_t source, 
                          const uint8_t* data, uint8_t len) override {
        // 处理GPS数据
        // portId: 串口号 (1, 2, 3)
        // source: 消息源ID
        // data: 数据指针
        // len: 数据长度
    }
};
```

### 2. 注册观察者

在 `main()` 函数中注册观察者到特定串口和消息源：

```cpp
int main() {
    Applications a;  // 初始化应用程序（自动初始化串口）
    
    // 创建观察者
    GPSObserver* gpsObs = new GPSObserver();
    BluetoothObserver* btObs = new BluetoothObserver();
    
    // 获取SerialPortObserver单例
    SerialPortObserver& spObserver = SerialPortObserver::getInstance();
    
    // 注册观察者
    // 参数：观察者对象, 串口号(1/2/3), 消息源ID
    spObserver.registerObserver(gpsObs, 1, 0x01);  // USART1, GPS消息
    spObserver.registerObserver(btObs, 1, 0x02);   // USART1, 蓝牙消息
    
    return Applications::exec();
}
```

### 3. 注销观察者

```cpp
spObserver.unregisterObserver(gpsObs, 1, 0x01);
```

## 工作流程

### 接收流程

1. **硬件中断**：UART空闲中断触发
2. **解析消息**：`handleIdleInterrupt()` 解析消息协议
3. **投递事件**：创建 `SerialReceiveEvent` 并加入事件队列
4. **事件处理**：主循环中 `processEvent()` 处理事件
5. **分发消息**：`SerialPortObserver` 将消息分发给注册的观察者
6. **观察者处理**：按优先级调用各观察者的 `processSerialData()`

### 发送流程

1. **调用发送**：`serial->sendData(data, len)`
2. **DMA传输**：启动DMA非阻塞传输
3. **中断触发**：DMA传输完成中断
4. **投递事件**：创建 `SerialSendCompleteEvent`
5. **事件处理**：通知发送完成（可选）

## DMA配置

### USART1
- **TX DMA**: DMA1_Channel4
- **RX DMA**: DMA1_Channel5
- **引脚**: TX=PA9, RX=PA10
- **支持DMA发送和接收**

### USART2
- **TX DMA**: DMA1_Channel7
- **RX DMA**: DMA1_Channel6
- **引脚**: TX=PA2, RX=PA3
- **支持DMA发送和接收**

### USART3
- **TX DMA**: 不使用（避免与TIM3冲突）
- **RX DMA**: DMA1_Channel3
- **引脚**: TX=PB10, RX=PB11
- **使用中断发送，DMA接收**

## 中断优先级

所有UART和DMA中断的优先级配置为：
- **抢占优先级**: 3
- **子优先级**: 0

这确保串口处理不会干扰更高优先级的任务（如舵机PWM）。

## 注意事项

1. **消息长度限制**：单条消息数据部分最大254字节
2. **缓冲区大小**：接收缓冲区256字节，需及时处理避免溢出
3. **DMA循环模式**：接收使用循环DMA，持续接收数据
4. **线程安全**：在中断和主循环之间通过事件队列通信
5. **内存管理**：事件对象使用new创建，处理后自动delete
6. **USART3限制**：为避免DMA冲突，USART3使用中断发送

## 示例代码

### 完整的GPS接收示例

```cpp
class GPSDataHandler : public SerialObserver {
private:
    char gpsBuffer[128];
    uint8_t bufIndex;
    
public:
    GPSDataHandler() : SerialObserver(10), bufIndex(0) {}
    
    void processSerialData(uint8_t portId, uint8_t source, 
                          const uint8_t* data, uint8_t len) override {
        // 累积GPS数据直到换行符
        for (uint8_t i = 0; i < len; i++) {
            if (data[i] == '\n') {
                gpsBuffer[bufIndex] = '\0';
                parseGPSData(gpsBuffer);
                bufIndex = 0;
            } else if (bufIndex < sizeof(gpsBuffer) - 1) {
                gpsBuffer[bufIndex++] = data[i];
            }
        }
    }
    
    void parseGPSData(const char* nmeaSentence) {
        // 解析NMEA格式的GPS数据
        // ...
    }
};
```

### 多观察者示例

```cpp
// 日志观察者 - 低优先级
class LogObserver : public SerialObserver {
public:
    LogObserver() : SerialObserver(1) {}
    void processSerialData(uint8_t portId, uint8_t source, 
                          const uint8_t* data, uint8_t len) override {
        // 记录所有数据到日志
    }
};

// 协议解析观察者 - 高优先级
class ProtocolObserver : public SerialObserver {
public:
    ProtocolObserver() : SerialObserver(10) {}
    void processSerialData(uint8_t portId, uint8_t source, 
                          const uint8_t* data, uint8_t len) override {
        // 优先解析协议数据
    }
};

// 两个观察者会按优先级顺序处理同一消息
spObserver.registerObserver(protocolObs, 1, 0x01);  // 先处理
spObserver.registerObserver(logObs, 1, 0x01);       // 后处理
```

## 兼容性

本系统完全兼容现有的：
- 事件循环框架
- 定时器系统
- 舵机PWM控制
- OLED显示

不会产生任何冲突。

## 调试建议

1. **启用串口回显**：在观察者中回显接收的数据
2. **检查消息格式**：确保发送端遵循协议格式
3. **监控队列状态**：检查事件队列是否溢出
4. **DMA状态检查**：确认DMA传输计数器正常工作
5. **中断触发验证**：使用示波器或逻辑分析仪检查IDLE中断

## 性能优化

- 消息解析在中断中完成，减少主循环负担
- DMA传输不占用CPU，完全后台运行
- 优先级队列确保重要消息优先处理
- 事件队列避免中断中的复杂处理

## 未来扩展

可以基于此框架扩展：
- CAN总线事件处理
- SPI/I2C事件驱动
- 文件系统异步IO
- 网络数据包处理
