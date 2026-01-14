#include "SerialPort/SerialPort.h"
#include "Applications.h"

// 全局实例
SimpleSerial* g_serial1 = nullptr;
SimpleSerial* g_serial2 = nullptr;
SimpleSerial* g_serial3 = nullptr;

SimpleSerial::SimpleSerial(USART_ID id, uint32_t baud)
    : Object(nullptr, ObjectType::SerialPort)
    , m_id(id)
    , m_rx_index(0)
    , m_rx_count(0)
    , m_tx_in_progress(false)
{
    init(baud);

    // 注册全局指针
    if (id == USART_ID::USART1_ID) {
        g_serial1 = this;
    } else if (id == USART_ID::USART2_ID) {
        g_serial2 = this;
    } else if (id == USART_ID::USART3_ID) {
        g_serial3 = this;
    }
}

SimpleSerial::~SimpleSerial() {
    if (m_usart) {
        USART_Cmd(m_usart, DISABLE);
    }
    if (m_dma_tx) {
        DMA_Cmd(m_dma_tx, DISABLE);
    }
    if (m_dma_rx) {
        DMA_Cmd(m_dma_rx, DISABLE);
    }
}

void SimpleSerial:: init(uint32_t baud) {
    initGPIO();
    initUSART(baud);
    initDMA();
    initNVIC();
}

void SimpleSerial::initGPIO() {
    // ✅ 修复：必须在配置 GPIO 之前启用 AFIO 时钟
    // AFIO 时钟必须先于任何复用功能 GPIO 配置启用
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    switch (m_id) {
        case USART_ID:: USART1_ID:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            m_usart = USART1;

            // TX:  PA9, RX: PA10
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
            GPIO_InitStructure. GPIO_Mode = GPIO_Mode_AF_PP;
            GPIO_Init(GPIOA, &GPIO_InitStructure);

            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;

        case USART_ID::USART2_ID:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            m_usart = USART2;

            // TX: PA2, RX:  PA3
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
            GPIO_Init(GPIOA, &GPIO_InitStructure);

            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;

        case USART_ID:: USART3_ID:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
            m_usart = USART3;

            // TX: PB10, RX: PB11
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
            GPIO_Init(GPIOB, &GPIO_InitStructure);

            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            break;
    }
}

void SimpleSerial::initUSART(uint32_t baud) {
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure. USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(m_usart, &USART_InitStructure);

    // 启用 DMA
    USART_DMACmd(m_usart, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);

    // 启用空闲中断（用于检测接收完成）
    USART_ITConfig(m_usart, USART_IT_IDLE, ENABLE);

    USART_Cmd(m_usart, ENABLE);
}

void SimpleSerial::initDMA() {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitTypeDef DMA_InitStructure;

    switch (m_id) {
        case USART_ID:: USART1_ID:
            // TX:  DMA1_Channel4, RX: DMA1_Channel5
            m_dma_tx = DMA1_Channel4;
            m_dma_rx = DMA1_Channel5;
            break;

        case USART_ID::USART2_ID:
            // TX: DMA1_Channel7, RX:  DMA1_Channel6
            m_dma_tx = DMA1_Channel7;
            m_dma_rx = DMA1_Channel6;
            break;

        case USART_ID::USART3_ID:
            // ⚠️ 注意：USART3 TX 使用 DMA1_Channel2，这可能与 TIM3_CH3 冲突！
            // 改用中断发送，DMA 只用于接收
            m_dma_tx = nullptr;  // 不用 DMA 发送
            m_dma_rx = DMA1_Channel3;
            break;
    }

    // 配置 RX DMA（循环接收）
    DMA_DeInit(m_dma_rx);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(m_usart->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)m_rx_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = sizeof(m_rx_buffer);
    DMA_InitStructure. DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;  // ✅ 循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;  // ✅ 低优先级，不影响舵机
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(m_dma_rx, &DMA_InitStructure);
    DMA_Cmd(m_dma_rx, ENABLE);

    // 只在 USART1/2 配置 TX DMA（USART3 用中断发送）
    if (m_dma_tx && m_id != USART_ID::USART3_ID) {
        DMA_DeInit(m_dma_tx);
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(m_usart->DR);
        DMA_InitStructure.DMA_MemoryBaseAddr = 0;  // 动态设置
        DMA_InitStructure. DMA_DIR = DMA_DIR_PeripheralDST;
        DMA_InitStructure.DMA_BufferSize = 0;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure. DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority = DMA_Priority_Low;  // ✅ 低优先级
        DMA_InitStructure. DMA_M2M = DMA_M2M_Disable;
        DMA_Init(m_dma_tx, &DMA_InitStructure);

        DMA_ITConfig(m_dma_tx, DMA_IT_TC, ENABLE);
    }
}

void SimpleSerial::initNVIC() {
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    // USART 中断
    switch (m_id) {
        case USART_ID:: USART1_ID:
            NVIC_InitStructure. NVIC_IRQChannel = USART1_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            break;

        case USART_ID::USART2_ID:
            NVIC_InitStructure. NVIC_IRQChannel = USART2_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            break;

        case USART_ID::USART3_ID:
            NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            // USART3 只用中断，不用 DMA TX
            break;
    }
}

void SimpleSerial::sendByte(uint8_t byte) {
    while (USART_GetFlagStatus(m_usart, USART_FLAG_TXE) == RESET);
    USART_SendData(m_usart, byte);
}

void SimpleSerial::sendString(const char* str) {
    while (*str) {
        sendByte((uint8_t)*str++);
    }
}

void SimpleSerial::sendData(const uint8_t* data, uint16_t len) {
    if (! m_dma_tx || m_id == USART_ID:: USART3_ID) {
        // USART3 或没有 DMA TX，用轮询发送
        for (uint16_t i = 0; i < len; i++) {
            sendByte(data[i]);
        }
        return;
    }

    // 使用 DMA 发送（非阻塞，带超时保护）
    uint32_t timeout = 10000;  // 超时计数
    while (m_tx_in_progress && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        // 超时，强制复位DMA状态
        m_tx_in_progress = false;
        DMA_Cmd(m_dma_tx, DISABLE);
    }
    
    m_tx_in_progress = true;
    DMA_Cmd(m_dma_tx, DISABLE);
    DMA_SetCurrDataCounter(m_dma_tx, len);
    m_dma_tx->CMAR = (uint32_t)data;
    DMA_Cmd(m_dma_tx, ENABLE);
}

bool SimpleSerial::received() {
    return m_rx_count > 0;
}

uint8_t SimpleSerial::readByte() {
    if (m_rx_count == 0) return 0;

    uint8_t byte = (uint8_t)m_rx_buffer[m_rx_index];
    m_rx_index++;
    m_rx_count--;

    return byte;
}

uint16_t SimpleSerial::readData(uint8_t* buffer, uint16_t max_len) {
    if (m_rx_count == 0) return 0;

    uint16_t len = (m_rx_count < max_len) ? m_rx_count : max_len;
    for (uint16_t i = 0; i < len; i++) {
        buffer[i] = (uint8_t)m_rx_buffer[m_rx_index + i];
    }

    m_rx_index += len;
    m_rx_count -= len;

    return len;
}

// 事件处理
void SimpleSerial::event(Event* e) {
    Object::event(e);
    // 可以在这里处理特定的串口事件
}

// 解析并投递消息事件
void SimpleSerial::parseAndPostMessage() {
    if (m_rx_count < 2) return;  // 至少需要source和length两个字节
    
    SerialMessage msg;
    msg.source = (uint8_t)m_rx_buffer[0];
    msg.length = (uint8_t)m_rx_buffer[1];
    
    // 检查消息长度是否有效（最大254字节，因为data数组是254）
    if (msg.length > 254 || m_rx_count < (2 + msg.length)) {
        // 消息不完整或长度无效，等待更多数据
        return;
    }
    
    // 复制数据
    for (uint8_t i = 0; i < msg.length; i++) {
        msg.data[i] = (uint8_t)m_rx_buffer[2 + i];
    }
    
    // 投递事件到全局事件队列
    uint8_t port_id = 0;
    if (m_id == USART_ID::USART1_ID) port_id = 1;
    else if (m_id == USART_ID::USART2_ID) port_id = 2;
    else if (m_id == USART_ID::USART3_ID) port_id = 3;
    
    SerialReceiveEvent* evt = new SerialReceiveEvent(this, port_id, msg);
    Applications::postEvent(evt);
    
    // 移除已处理的消息
    uint16_t msg_total_len = 2 + msg.length;
    m_rx_index += msg_total_len;
    m_rx_count -= msg_total_len;
}

// 处理IDLE中断
void SimpleSerial::handleIdleInterrupt() {
    // 计算接收的数据长度
    uint16_t received = sizeof(m_rx_buffer) - DMA_GetCurrDataCounter(m_dma_rx);
    m_rx_count = received - m_rx_index;
    
    // 解析并投递消息
    parseAndPostMessage();
}

// 处理DMA发送完成
void SimpleSerial::handleDMATxComplete() {
    m_tx_in_progress = false;
    
    // 投递发送完成事件
    uint8_t port_id = 0;
    if (m_id == USART_ID::USART1_ID) port_id = 1;
    else if (m_id == USART_ID::USART2_ID) port_id = 2;
    else if (m_id == USART_ID::USART3_ID) port_id = 3;
    
    SerialSendCompleteEvent* evt = new SerialSendCompleteEvent(this, port_id);
    Applications::postEvent(evt);
}

// ====== 中断处理 ======

extern "C" {
    void USART1_IRQHandler(void) {
        if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
            USART_ReceiveData(USART1);  // 清除标志
            if (g_serial1) {
                g_serial1->handleIdleInterrupt();
            }
        }
    }
    
    void USART2_IRQHandler(void) {
        if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
            USART_ReceiveData(USART2);
            if (g_serial2) {
                g_serial2->handleIdleInterrupt();
            }
        }
    }

    void USART3_IRQHandler(void) {
        if (USART_GetITStatus(USART3, USART_IT_IDLE) != RESET) {
            USART_ReceiveData(USART3);
            if (g_serial3) {
                g_serial3->handleIdleInterrupt();
            }
        }
    }

    void DMA1_Channel4_IRQHandler(void) {  // USART1 TX
        if (DMA_GetFlagStatus(DMA1_FLAG_TC4) == SET) {
            DMA_ClearFlag(DMA1_FLAG_TC4);
            if (g_serial1) {
                g_serial1->handleDMATxComplete();
            }
        }
    }

    void DMA1_Channel7_IRQHandler(void) {  // USART2 TX
        if (DMA_GetFlagStatus(DMA1_FLAG_TC7) == SET) {
            DMA_ClearFlag(DMA1_FLAG_TC7);
            if (g_serial2) {
                g_serial2->handleDMATxComplete();
            }
        }
    }
}