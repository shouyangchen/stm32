#include "stm32f10x.h"
#include "Applications.h"
#include "OLED/OLED.h"
#include <Servo/Servo.h>

#include "Servo/Servo.h"
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

int main() {
    servo1_ptr=new Servo(TIM3, Servo::Channel::CH3, GPIOB, GPIO_Pin_0);
    servo2_ptr=new Servo(TIM3,Servo::Channel::CH2,GPIOA,GPIO_Pin_7);
    Applications a;
    Applications::addTimer(1000, 0,servoCallBack,nullptr);
    return Applications::exec();
}


