// c
// 文件：`src/assert_failed.c`
// 提供最小实现：`assert_failed`（已存在）及兼容的 `assert_param` 函数，满足不同 SPL 版本链接需求。
#include <stdint.h>
#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

// 简单的调试输出函数（使用轮询方式发送，不依赖DMA）
static void debug_putchar(char c) {
    // 尝试使用USART1输出（如果已初始化）
    if (USART1->CR1 & USART_CR1_UE) {  // 检查USART1是否启用
        while (!(USART1->SR & USART_SR_TXE));  // 等待发送缓冲区空
        USART1->DR = c;
    }
}

static void debug_print(const char* str) {
    while (*str) {
        debug_putchar(*str++);
    }
}

static void debug_print_hex(uint32_t num) {
    const char hex[] = "0123456789ABCDEF";
    debug_print("0x");
    for (int i = 7; i >= 0; i--) {
        debug_putchar(hex[(num >> (i * 4)) & 0xF]);
    }
}

void assert_failed(uint8_t* file, uint32_t line)
{
    // 尝试输出错误信息
    debug_print("\r\n*** ASSERT FAILED ***\r\n");
    
    if (file) {
        debug_print("File: ");
        debug_print((const char*)file);
        debug_print("\r\n");
    }
    
    debug_print("Line: ");
    debug_print_hex(line);
    debug_print("\r\n");
    debug_print("System halted.\r\n");
    
    /* 可在此添加日志/LED/断点；当前为安全死循环 */
    while (1)
    {
        __asm__ volatile ("nop");
    }
}

/* 有些 SPL 编译单元会生成对函数名 `assert_param` 的引用（而非宏）。
   提供一个简单实现：表达式为假时调用 assert_failed。 */
void assert_param(int expr)
{
    if (!expr)
    {
        assert_failed((uint8_t*)0, 0);
    }
}

#ifdef __cplusplus
}
#endif

