#include "Servo/Servo.h"

Servo:: Servo(TIM_TypeDef* timer, Channel channel, GPIO_TypeDef* gpio, uint16_t pin)
    : timer_m(timer),
      gpio_m(gpio),
      channel_m(channel),
      pin_m(pin),
      currentAngle_m(90),
      isAttached_m(false)
{
    // 1. 使能时钟
    enableGPIOClock(gpio);
    enableTimerClock(timer);
    enableAFIOClock();  // ✅ 新增：在 GPIO 初始化之前启用 AFIO

    // 2. 初始化GPIO
    initGPIO(gpio, pin);

    // 3. 初始化定时器
    initTimer();

    // 4. 配置PWM通道
    configureChannel();

    // 5. 设置初始角度为90度
    setAngle(90);

    isAttached_m = true;
}

// ✅ 新增函数
void Servo::enableAFIOClock() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    // ✅ 修复：确保 TIM3 使用默认引脚映射（禁用任何重映射）
    // TIM3 默认映射: CH1=PA6, CH2=PA7, CH3=PB0, CH4=PB1
    // 这对于 PB0 上的舵机至关重要
    if (timer_m == TIM3) {
        GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, DISABLE);
        GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, DISABLE);
    }
}

void Servo::enableGPIOClock(GPIO_TypeDef* gpio) {
    if (gpio == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if (gpio == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (gpio == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    } else if (gpio == GPIOD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    } else if (gpio == GPIOE) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    }
}

void Servo::enableTimerClock(TIM_TypeDef* timer) {
    if (timer == TIM1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    } else if (timer == TIM2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    } else if (timer == TIM3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    } else if (timer == TIM4) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    } else if (timer == TIM5) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    } else if (timer == TIM8) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
    }
}

void Servo::initGPIO(GPIO_TypeDef* gpio, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // ✅ 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio, &GPIO_InitStructure);
}

void Servo::initTimer() {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    TIM_TimeBaseStructure.TIM_Period = ARR;
    TIM_TimeBaseStructure.TIM_Prescaler = PSC;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(timer_m, &TIM_TimeBaseStructure);

    TIM_ARRPreloadConfig(timer_m, ENABLE);

    if (timer_m == TIM1 || timer_m == TIM8) {
        TIM_CtrlPWMOutputs(timer_m, ENABLE);
    }

    TIM_Cmd(timer_m, ENABLE);
}

void Servo::configureChannel() {
    TIM_OCInitTypeDef TIM_OCInitStructure;

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = MID_PULSE;

    switch (channel_m) {
        case Channel::CH1:
            TIM_OC1Init(timer_m, &TIM_OCInitStructure);
            TIM_OC1PreloadConfig(timer_m, TIM_OCPreload_Enable);
            break;
        case Channel::CH2:
            TIM_OC2Init(timer_m, &TIM_OCInitStructure);
            TIM_OC2PreloadConfig(timer_m, TIM_OCPreload_Enable);
            break;
        case Channel::CH3:
            TIM_OC3Init(timer_m, &TIM_OCInitStructure);
            TIM_OC3PreloadConfig(timer_m, TIM_OCPreload_Enable);
            break;
        case Channel::CH4:
            TIM_OC4Init(timer_m, &TIM_OCInitStructure);
            TIM_OC4PreloadConfig(timer_m, TIM_OCPreload_Enable);
            break;
    }
}

void Servo:: setAngle(uint8_t angle) {
    if (angle > 180) {
        angle = 180;
    }

    currentAngle_m = angle;
    uint16_t pulseWidth = angleToPulseWidth(angle);
    setCCR(pulseWidth);
}

void Servo::setPulseWidth(uint16_t pulseWidth) {
    if (pulseWidth < MIN_PULSE) {
        pulseWidth = MIN_PULSE;
    } else if (pulseWidth > MAX_PULSE) {
        pulseWidth = MAX_PULSE;
    }

    setCCR(pulseWidth);
    currentAngle_m = (uint8_t)((pulseWidth - MIN_PULSE) * 180 / (MAX_PULSE - MIN_PULSE));
}

uint16_t Servo::angleToPulseWidth(uint8_t angle) {
    return MIN_PULSE + ((uint32_t)angle * (MAX_PULSE - MIN_PULSE) / 180);
}

void Servo::setCCR(uint16_t value) {
    if (! isAttached_m) {
        return;
    }

    switch (channel_m) {
        case Channel::CH1:
            TIM_SetCompare1(timer_m, value);
            break;
        case Channel:: CH2:
            TIM_SetCompare2(timer_m, value);
            break;
        case Channel::CH3:
            TIM_SetCompare3(timer_m, value);
            break;
        case Channel:: CH4:
            TIM_SetCompare4(timer_m, value);
            break;
    }
}

void Servo::detach() {
    if (!isAttached_m) {
        return;
    }

    setCCR(0);
    isAttached_m = false;
}

void Servo::attach() {
    if (isAttached_m) {
        return;
    }

    isAttached_m = true;
    setAngle(currentAngle_m);
}