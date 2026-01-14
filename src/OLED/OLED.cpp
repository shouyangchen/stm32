#include "OLED/OLED.h"

#include "Applications.h"
#include "OLED/OLED_Font.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

// OLED地址
#define OLED_ADDRESS 0x78

extern const unsigned char PIC1[1024];
extern const unsigned char PIC2[1024];
extern const unsigned char PIC3[1024];
extern const unsigned char PIC4[1024];
extern const unsigned char PIC5[1024];
extern const unsigned char PIC6[1024];
extern const unsigned char PIC7[1024];
extern const unsigned char PIC8[1024];
extern const unsigned char PIC9[1024];
extern const unsigned char PIC10[1024];
extern const unsigned char PIC11[1024];
extern const unsigned char PIC12[1024];

const unsigned char *Animations[12]={PIC1,PIC2,PIC3,PIC4,PIC5,PIC6,PIC7,PIC8,PIC9,PIC10,PIC11,PIC12};

volatile uint8_t OLED::order_pic=0;
volatile uint32_t OLED::set_mine_num=30;
volatile uint32_t OLED::now_time=0;
volatile uint32_t OLED::drink_nums=0;
// OLED延时函数
static void OLED_Delay(volatile uint32_t count) {
    while(count--);
}


void OLED::OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[])
{
    unsigned int j = 0;
    unsigned char x, y;

    for(y = y0; y < y0 + y1; y++)  // y1 是页数 (8页)
    {
        SetPos(x0, y);
        for(x = x0; x < x1; x++)
        {
            WriteByte(BMP[j++], 1);
        }
    }
}

void OLED::OLED_DrawAnimations() {//显示动画时间间隔为100ms转为帧数为10
    Clear();
    if (order_pic==12) {
        order_pic=0;
    }
    OLED_DrawBMP(0, 0, 128, 8, const_cast< unsigned char *>(Animations[order_pic++]));  // 注意：y1=8 表示8页

}

static uint8_t OLED_CalcProgress(uint32_t totalTime, uint32_t remainTime) {
    if (totalTime == 0) return 0; // 避免除零
    // 进度 = (总时间 - 剩余时间) / 总时间 * 100
    uint32_t progress = ((totalTime - remainTime) * 100) / totalTime;
    return (progress > 100) ? 100 : static_cast<uint8_t>(progress);
}

OLED::OLED() {}

// I2C起始信号
void OLED::I2C_Start() {
    SDA_H();
    SCL_H();
    OLED_Delay(10);
    SDA_L();
    OLED_Delay(10);
    SCL_L();
}

// I2C停止信号
void OLED::I2C_Stop() {
    SDA_L();
    SCL_H();
    OLED_Delay(10);
    SDA_H();
}

// I2C等待应答
void OLED::I2C_WaitAck() {
    SCL_H();
    OLED_Delay(10);
    SCL_L();
}

// I2C发送字节
void OLED::I2C_SendByte(uint8_t byte) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (byte & 0x80) SDA_H();
        else SDA_L();
        byte <<= 1;
        OLED_Delay(5);
        SCL_H();
        OLED_Delay(10);
        SCL_L();
        OLED_Delay(5);
    }
    I2C_WaitAck();
}

// 向OLED写入字节（数据或命令）
// dat: 要写入的数据/命令
// cmd: 0表示命令,1表示数据
void OLED::WriteByte(uint8_t dat, uint8_t cmd) {
    I2C_Start();
    I2C_SendByte(OLED_ADDRESS);
    if (cmd) I2C_SendByte(0x40); // 数据
    else I2C_SendByte(0x00);     // 命令
    I2C_SendByte(dat);
    I2C_Stop();
}

// 设置显示位置
// x: 列位置(0-127)
// y: 页位置(0-7)
void OLED::SetPos(unsigned char x, unsigned char y) {
    WriteByte(0xb0 + y, 0);
    WriteByte(((x & 0xf0) >> 4) | 0x10, 0);
    WriteByte((x & 0x0f), 0);
}

// 打开OLED显示
void OLED::On() {
    WriteByte(0X8D, 0);
    WriteByte(0X14, 0);
    WriteByte(0XAF, 0);
}

// 关闭OLED显示
void OLED::Off() {
    WriteByte(0X8D, 0);
    WriteByte(0X10, 0);
    WriteByte(0XAE, 0);
}

// 清屏函数
void OLED::Clear() {
    uint8_t i, n;
    for (i = 0; i < 8; i++) {
        WriteByte(0xb0 + i, 0);  // 设置页地址
        WriteByte(0x00, 0);      // 设置列低地址
        WriteByte(0x10, 0);      // 设置列高地址
        for (n = 0; n < 128; n++) WriteByte(0, 1); // 填充0清空
    }
}

// OLED初始化函数
void OLED::Init() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能GPIOB时钟

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // PB6(SCL), PB7(SDA)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;      // 开漏输出(适合I2C)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     // 50MHz速度
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SCL_H(); // 初始拉高SCL
    SDA_H(); // 初始拉高SDA

    OLED_Delay(10000); // 等待屏幕上电稳定

    // SSD1306初始化序列
    WriteByte(0xAE, 0); // 关闭显示
    WriteByte(0x00, 0); // 设置低列地址
    WriteByte(0x10, 0); // 设置高列地址
    WriteByte(0x40, 0); // 设置起始行
    WriteByte(0x81, 0); // 设置对比度控制
    WriteByte(0xCF, 0);
    WriteByte(0xA1, 0); // 设置段重映射
    WriteByte(0xC8, 0); // 设置COM输出扫描方向
    WriteByte(0xA6, 0); // 设置正常/反显显示
    WriteByte(0xA8, 0); // 设置多路复用率
    WriteByte(0x3F, 0);
    WriteByte(0xD3, 0); // 设置显示偏移
    WriteByte(0x00, 0);
    WriteByte(0xD5, 0); // 设置显示时钟分频/振荡器频率
    WriteByte(0x80, 0);
    WriteByte(0xD9, 0); // 设置预充电周期
    WriteByte(0xF1, 0);
    WriteByte(0xDA, 0); // 设置COM引脚硬件配置
    WriteByte(0x12, 0);
    WriteByte(0xDB, 0); // 设置VCOMH取消选择电平
    WriteByte(0x40, 0);
    WriteByte(0x20, 0); // 设置内存寻址模式
    WriteByte(0x02, 0);
    WriteByte(0x8D, 0); // 设置电荷泵
    WriteByte(0x14, 0);
    WriteByte(0xA4, 0); // 整个显示打开(恢复)
    WriteByte(0xA6, 0); // 正常显示
    WriteByte(0xAF, 0); // 打开显示

    Clear(); // 清屏
}

// 显示单个字符
// x: 列位置(0-127)
// y: 页位置(0-7)
// chr: 要显示的字符
// Char_Size: 字符大小(目前仅支持16)
void OLED::ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size) {
    unsigned char c = 0, i = 0;
    c = chr - ' '; // 计算偏移量
    if (x > 128 - 1) { x = 0; y = y + 2; } // 超出边界换行
    if (Char_Size == 16) {
        // 映射字符到字库索引
        int index = -1;
        if (chr >= '0' && chr <= '9') index = chr - '0'; // 数字0-9
        else if (chr == ':') index = 10; // 冒号
        else if (chr == ' ') index = 11; // 空格

        if (index != -1) {
             SetPos(x, y);
             for (i = 0; i < 8; i++) WriteByte(F8X16[index][i], 1); // 上半部分
             SetPos(x, y + 1);
             for (i = 0; i < 8; i++) WriteByte(F8X16[index][i + 8], 1); // 下半部分
        }
    }
}

// 计算m的n次方(用于数字显示)
uint32_t OLED::oled_pow(uint8_t m, uint8_t n) {
    uint32_t result = 1;
    while (n--) result *= m;
    return result;
}

// 显示数字
// x: 列位置
// y: 页位置
// num: 要显示的数字
// len: 数字位数
// size2: 字符大小
void OLED::ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size2) {
    uint8_t t, temp;
    uint8_t enshow = 0; // 是否开始显示(用于跳过前导零)
    for (t = 0; t < len; t++) {
        temp = (num / oled_pow(10, len - t - 1)) % 10; // 提取每一位
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                ShowChar(x + (size2 / 2) * t, y, ' ', size2); // 前导零显示为空格
                continue;
            } else enshow = 1; // 遇到非零数字开始显示
        }
        ShowChar(x + (size2 / 2) * t, y, temp + '0', size2); // 显示数字
    }
}

// 显示字符串
// x: 列位置
// y: 页位置
// chr: 字符串指针
// Char_Size: 字符大小
void OLED::ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size) {
    unsigned char j = 0;
    while (chr[j] != '\0') { // 遍历字符串
        ShowChar(x, y, chr[j], Char_Size);
        x += 8; // 移动到下一个字符位置
        if (x > 120) { x = 0; y += 2; } // 超出边界换行
        j++;
    }
}

// 显示汉字
// x: 列位置
// y: 页位置
// no: 汉字在字库中的索引
// font_array: 汉字字库数组
void OLED::ShowChinese(uint8_t x, uint8_t y, uint8_t no, const uint8_t* font_array) {
    uint8_t t, adder = 0;
    SetPos(x, y);
    for (t = 0; t < 16; t++) {
        WriteByte(font_array[2 * no * 16 + t], 1); // 上半部分
        adder++;
    }
    SetPos(x, y + 1);
    for (t = 0; t < 16; t++) {
        WriteByte(font_array[2 * no * 16 + t + 16], 1); // 下半部分
        adder++;
    }
}

// 绘制进度条
// x: 左上角x坐标
// y: 左上角y坐标(页)
// width: 进度条宽度
// height: 进度条高度
// progress: 进度(0-100)
void OLED::DrawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress) {
    // 简易进度条: 边框 + 填充
    // 假设高度为8像素(1页)
    if (progress > 100) progress = 100; // 限制进度最大值
    uint8_t fill_width = (width * progress) / 100; // 计算填充宽度

    SetPos(x, y);
    for (uint8_t i = 0; i < width; i++) {
        uint8_t data = 0;
        // 上下边框
        data |= 0x01; // 上边
        data |= 0x80; // 下边

        // 左右边框
        if (i == 0 || i == width - 1) data = 0xFF; // 左右边框填充

        // 填充部分
        if (i < fill_width && i > 0) {
            data |= 0x7E; // 填充中间部分
        }

        WriteByte(data, 1);
    }
}

// 更新状态显示
// currentValue: 当前值
// maxValue: 最大值
void OLED::UpdateStatus(uint32_t currentValue, uint32_t maxValue) {
    if (set_mine_num!=now_time) {
        for(int i=0; i<6; i++) {
            ShowChinese(i*16, 0, i, (const uint8_t*)HZK_Row1);
        }

        uint8_t percent = OLED_CalcProgress(OLED::set_mine_num,OLED::now_time);   // 计算百分比

        ShowNum(106,0,(set_mine_num-now_time>0?set_mine_num-now_time:0) , 2, 16);//显示剩余时间

        ShowNum(35,2 , percent, 3, 16);//显示百分比

        for(int i=0; i<6; i++) {
            ShowChinese(i*16, 6, i, (const uint8_t*)HZK_Row2);
        }
        ShowNum(106,6,drink_nums,2,16);
        DrawProgressBar(0, 4, 100, 8, percent);
    }
        //如果剩余时间为0则直接触发软中断显示BMP图然后蜂鸣器鸣叫
        //需要外部中断来修改值或者按下复位键


}


