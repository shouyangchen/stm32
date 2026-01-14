#include "sleep/sleep.h"
#include "stm32f10x.h"

static volatile uint32_t msTicks = 0;

void sleep_ms(uint32_t ms) {
    msTicks = ms;
    while (msTicks != 0) {
        __NOP();
    }
}

