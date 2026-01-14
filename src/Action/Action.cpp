//
// Created by chenshouyang on 2026/1/11.
//

#include "Action/Action.h"
#include "Applications.h"

void GoForward(void*data) {
  auto action= Action::getInstance();

}

Action::Action():go_back_state(StateGoBack::INIT),go_forward_state(StateGoForward::INIT),
    go_left_state(StateGoLeft::INIT),go_right_right(StateGoRight::INIT),time_space(500) {
    this->legs_m.emplace_back(TIM3,Servo::Channel::CH4,GPIOB,GPIO_Pin_1);//前左
    this->legs_m.emplace_back(TIM3, Servo::Channel::CH3, GPIOB, GPIO_Pin_0);//前右
    this->legs_m.emplace_back(TIM3,Servo::Channel::CH2,GPIOA,GPIO_Pin_7);//后左
    this->legs_m.emplace_back(TIM3,Servo::Channel::CH1,GPIOA,GPIO_Pin_6);//后右
}





void Action::event(Event *event) {
    switch (event->getEventType()) {
        case Event::EventType_ASRGoForward:
            this->goForward();
            //TODO:可通过于OLED通信进行表情播放或者是通过串口发送数据给ASR让其播放特定的音频
            break;
        case Event::EventType_ASRGoBack:
            this->goBack();
            break;
        case Event::EventType_ASRGoLeft:
            this->goLeft();
            break;
        case Event::EventType_ASRGoRight:
            this->goRight();
            break;
         default:
            break;
    }
}

void Action::goBack() {

}

void Action::goForward() {

}

void Action::goLeft() {

}

void Action::goRight() {

}

void Action::go_Back() {

}

void Action::go_Forward() {
    //TODO:前进通过调用Application的添加计时器来实现我们需要添加4个总计500ms+600ms+700ms+800ms=2.6s
    //TODO:2.6往前移动一步向前移动四步需要近9秒

}

void Action::go_Left() {

}

void Action::go_Right() {

}