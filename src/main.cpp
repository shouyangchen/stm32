#include "stm32f10x.h"
#include "Applications.h"
#include "OLED/OLED.h"
#include <Servo/Servo.h>
#include "SeariPortObserver/SerialPortObserver.h"

extern SimpleSerial* serial1;

OLED oled;
uint32_t progress = 0;
const uint32_t max_progress = 100;

void oled_update_callback(void* arg) {
    if (OLED::now_time!=OLED::set_mine_num) {
        OLED::now_time+=1;
        oled.UpdateStatus(OLED::now_time, max_progress);
        // 重复计时器
        Applications::addTimer(60000, 0, oled_update_callback, nullptr);
    }
    else {
         oled.OLED_DrawAnimations();
        Applications::addTimer(500, 0, oled_update_callback, nullptr);
    }

}

Servo* servo1_ptr=nullptr;
Servo* servo2_ptr=nullptr;
uint8_t i=0;
void servoCallBack(void*data) {
     if (i==INT8_MAX)
         i=0;
     if (i&0x1) {
         servo1_ptr->setAngle(180);
         // servo2_ptr->setAngle(30);
         i+=1;
         Applications::addTimer(500, 0,servoCallBack,nullptr);
     }
     else {
         servo1_ptr->setAngle(160);
         // servo2_ptr->setAngle(60);
         i+=1;
         Applications::addTimer(500, 0,servoCallBack,nullptr);
     }
}

// 示例：GPS消息观察者（消息源ID = 0x01）
class GPSObserver : public SerialObserver {
public:
    GPSObserver() : SerialObserver(10) {}  // 优先级10
    
    void processSerialData(uint8_t portId, uint8_t source, const uint8_t* data, uint8_t len) override {
        // 处理GPS数据
        if (serial1) {
            serial1->sendString("GPS Data: ");
            serial1->sendData(data, len);
            serial1->sendString("\r\n");
        }
    }
};

// 示例：蓝牙消息观察者（消息源ID = 0x02）
class BluetoothObserver : public SerialObserver {
public:
    BluetoothObserver() : SerialObserver(5) {}  // 优先级5
    
    void processSerialData(uint8_t portId, uint8_t source, const uint8_t* data, uint8_t len) override {
        // 处理蓝牙数据
        if (serial1) {
            serial1->sendString("BT Data: ");
            serial1->sendData(data, len);
            serial1->sendString("\r\n");
        }
    }
};

int main() {
    servo1_ptr=new Servo(TIM3, Servo::Channel::CH3, GPIOB, GPIO_Pin_0);
    servo2_ptr=new Servo(TIM3,Servo::Channel::CH2,GPIOA,GPIO_Pin_7);
    Applications a;
    
    // 注册串口观察者示例
    // 假设USART1用于接收GPS数据（消息源0x01）和蓝牙数据（消息源0x02）
    GPSObserver* gpsObs = new GPSObserver();
    BluetoothObserver* btObs = new BluetoothObserver();
    
    SerialPortObserver& spObserver = SerialPortObserver::getInstance();
    spObserver.registerObserver(gpsObs, 1, 0x01);  // 串口1, 消息源0x01 (GPS)
    spObserver.registerObserver(btObs, 1, 0x02);   // 串口1, 消息源0x02 (蓝牙)
    
    Applications::addTimer(1000, 0,servoCallBack,nullptr);
    return Applications::exec();
}


