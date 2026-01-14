#ifndef SIMPLE_SERIAL_H
#define SIMPLE_SERIAL_H

#include "stm32f10x.h"
#include <cstring>

class SimpleSerial {
public:
    enum class USART_ID {
        USART1_ID,
        USART2_ID,
        USART3_ID
    };

    SimpleSerial(USART_ID id, uint32_t baud);
    ~SimpleSerial();

    void sendByte(uint8_t byte);
    void sendString(const char* str);
    void sendData(const uint8_t* data, uint16_t len);

    bool received();
    uint8_t readByte();
    uint16_t readData(uint8_t* buffer, uint16_t max_len);

    // ✅ 改为 public（中断需要访问）
    USART_TypeDef* m_usart;
    USART_ID m_id;
    DMA_Channel_TypeDef* m_dma_tx;
    DMA_Channel_TypeDef* m_dma_rx;

    volatile uint8_t m_rx_buffer[256];
    volatile uint16_t m_rx_index;
    volatile uint16_t m_rx_count;

private:
    void init(uint32_t baud);
    void initGPIO();
    void initUSART(uint32_t baud);
    void initDMA();
    void initNVIC();
};

extern SimpleSerial* g_serial1;
extern SimpleSerial* g_serial2;
extern SimpleSerial* g_serial3;

extern "C" {
    void USART1_IRQHandler(void);
    void USART2_IRQHandler(void);
    void USART3_IRQHandler(void);
    void DMA1_Channel4_IRQHandler(void);
    void DMA1_Channel7_IRQHandler(void);
}

#endif