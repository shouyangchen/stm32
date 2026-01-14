// c
// 文件：`src/assert_failed.c`
// 提供最小实现：`assert_failed`（已存在）及兼容的 `assert_param` 函数，满足不同 SPL 版本链接需求。
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void assert_failed(uint8_t* file, uint32_t line)
{
    (void)file;
    (void)line;
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

