/**
 * @file SerialPortDebug.h
 * @brief Debug utilities for SerialPort initialization
 * 
 * Provides parameter validation and debug logging for SerialPort
 */

#ifndef SERIALPORT_DEBUG_H
#define SERIALPORT_DEBUG_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 调试输出超时常数（循环次数，实际时间取决于CPU频率）
#define SERIAL_DEBUG_TIMEOUT 10000

// 简单的调试输出函数（使用轮询方式发送，不依赖DMA）
static inline void serial_debug_putchar(USART_TypeDef* usart, char c) {
    if (!usart) return;
    if (!(usart->CR1 & USART_CR1_UE)) return;  // USART未启用
    
    // 等待发送缓冲区空，带超时保护
    // timeout递减到0表示超时，此时timeout--返回0使循环退出
    uint32_t timeout = SERIAL_DEBUG_TIMEOUT;
    while (!(usart->SR & USART_SR_TXE) && timeout--);
    // 只有当没有超时(timeout>0)时才写入数据，避免在硬件故障时死锁
    if (timeout > 0) {
        usart->DR = c;
    }
}

static inline void serial_debug_print(USART_TypeDef* usart, const char* str) {
    if (!usart || !str) return;
    while (*str) {
        serial_debug_putchar(usart, *str++);
    }
}

static inline void serial_debug_print_hex(USART_TypeDef* usart, uint32_t num) {
    const char hex[] = "0123456789ABCDEF";
    serial_debug_print(usart, "0x");
    for (int i = 7; i >= 0; i--) {
        serial_debug_putchar(usart, hex[(num >> (i * 4)) & 0xF]);
    }
}

static inline void serial_debug_print_dec(USART_TypeDef* usart, uint32_t num) {
    // Buffer para uint32_t decimal: max 10 dígitos + null terminator
    #define DECIMAL_BUFFER_SIZE 12
    char buf[DECIMAL_BUFFER_SIZE];
    int i = 0;
    
    if (num == 0) {
        serial_debug_putchar(usart, '0');
        return;
    }
    
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        serial_debug_putchar(usart, buf[--i]);
    }
    #undef DECIMAL_BUFFER_SIZE
}

// 参数验证函数
static inline bool validate_baudrate(uint32_t baud) {
    // STM32F103常用波特率范围：300 - 921600
    // 实际最大波特率取决于时钟频率，这里使用保守值
    return (baud >= 300 && baud <= 921600);
}

static inline bool validate_usart_instance(USART_TypeDef* usart) {
    // 验证USART实例指针是否为有效的USART外设地址
    return (usart == USART1 || usart == USART2 || usart == USART3);
}

static inline bool validate_gpio_speed(GPIOSpeed_TypeDef speed) {
    return (speed == GPIO_Speed_10MHz || 
            speed == GPIO_Speed_2MHz || 
            speed == GPIO_Speed_50MHz);
}

static inline bool validate_gpio_mode(GPIOMode_TypeDef mode) {
    return (mode == GPIO_Mode_AIN ||
            mode == GPIO_Mode_IN_FLOATING ||
            mode == GPIO_Mode_IPD ||
            mode == GPIO_Mode_IPU ||
            mode == GPIO_Mode_Out_OD ||
            mode == GPIO_Mode_Out_PP ||
            mode == GPIO_Mode_AF_OD ||
            mode == GPIO_Mode_AF_PP);
}

#ifdef __cplusplus
}
#endif

#endif // SERIALPORT_DEBUG_H
