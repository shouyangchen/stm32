#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

class OLED {
public:
    volatile static uint32_t set_mine_num;//设置喝水的时间间隔
    volatile static uint32_t now_time;
    volatile static uint32_t drink_nums;
    volatile static uint8_t order_pic;
    OLED();
    void Init();
    void Clear();
    void On();
    void Off();
    void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
    void OLED_DrawAnimations();
    void ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);
    void ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size2);
    void ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size);
    void ShowChinese(uint8_t x, uint8_t y, uint8_t no, const uint8_t* font_array);

    // 在(x,y)位置绘制进度条，参数包括宽高和进度(0-100)
    void DrawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress);


    // 根据当前值和最大值更新显示
    // 会在屏幕上更新数字和进度条
    // 可在实现中自定义位置
    void UpdateStatus(uint32_t currentValue, uint32_t maxValue);

private:
    void WriteByte(uint8_t dat, uint8_t cmd);
    void SetPos(unsigned char x, unsigned char y);
    uint32_t oled_pow(uint8_t m, uint8_t n);

    // I2C底层函数
    void I2C_Start();
    void I2C_Stop();
    void I2C_WaitAck();
    void I2C_SendByte(uint8_t byte);
    // 引脚定义(软件I2C)
    // SCL: PB6, SDA: PB7
    void SCL_H() { GPIOB->BSRR = GPIO_Pin_6; }
    void SCL_L() { GPIOB->BRR = GPIO_Pin_6; }
    void SDA_H() { GPIOB->BSRR = GPIO_Pin_7; }
    void SDA_L() { GPIOB->BRR = GPIO_Pin_7; }
};

#endif