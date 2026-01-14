#ifndef SERVO_H
#define SERVO_H

#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

class Servo {
public:
    enum class Channel :  uint8_t {
        CH1 = 1,
        CH2 = 2,
        CH3 = 3,
        CH4 = 4
    };

    Servo(TIM_TypeDef* timer, Channel channel, GPIO_TypeDef* gpio, uint16_t pin);

    void setAngle(uint8_t angle);
    void setPulseWidth(uint16_t pulseWidth);
    uint8_t getAngle() const { return currentAngle_m; }
    void detach();
    void attach();

private:
    void initGPIO(GPIO_TypeDef* gpio, uint16_t pin);
    void initTimer();
    void enableTimerClock(TIM_TypeDef* timer);
    void enableGPIOClock(GPIO_TypeDef* gpio);
    void enableAFIOClock();  // ✅ 新增
    void configureChannel();
    void setCCR(uint16_t value);
    uint16_t angleToPulseWidth(uint8_t angle);

    TIM_TypeDef* timer_m;
    GPIO_TypeDef* gpio_m;
    Channel channel_m;
    uint16_t pin_m;
    uint8_t currentAngle_m;
    bool isAttached_m;

    static constexpr uint16_t PSC = 71;
    static constexpr uint16_t ARR = 19999;
    static constexpr uint16_t MIN_PULSE = 500;
    static constexpr uint16_t MAX_PULSE = 2500;
    static constexpr uint16_t MID_PULSE = 1500;
};

#endif // SERVO_H