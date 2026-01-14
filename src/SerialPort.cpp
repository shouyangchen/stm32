#include "SerialPort/SerialPort.h"
#include "SerialPort/SerialPortDebug.h"
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
    , m_usart(nullptr)
    , m_dma_tx(nullptr)
    , m_dma_rx(nullptr)
{
    // ✅ 参数验证：检查波特率有效性
    if (!validate_baudrate(baud)) {
        // 波特率无效，使用默认值
        baud = 115200;
    }
    
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
    
    // ✅ 参数验证：检查GPIO速度配置
    if (!validate_gpio_speed(GPIO_InitStructure.GPIO_Speed)) {
        // GPIO速度配置无效，但这不应该发生
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    }

    switch (m_id) {
        case USART_ID:: USART1_ID:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            m_usart = USART1;

            // TX:  PA9, RX: PA10
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
            GPIO_InitStructure. GPIO_Mode = GPIO_Mode_AF_PP;
            // ✅ 参数验证：检查GPIO模式
            if (!validate_gpio_mode(GPIO_InitStructure.GPIO_Mode)) {
                m_usart = nullptr;  // 配置无效，清除USART指针
                return;
            }
            GPIO_Init(GPIOA, &GPIO_InitStructure);

            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            if (!validate_gpio_mode(GPIO_InitStructure.GPIO_Mode)) {
                m_usart = nullptr;
                return;
            }
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;

        case USART_ID::USART2_ID:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            m_usart = USART2;

            // TX: PA2, RX:  PA3
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
            if (!validate_gpio_mode(GPIO_InitStructure.GPIO_Mode)) {
                m_usart = nullptr;
                return;
            }
            GPIO_Init(GPIOA, &GPIO_InitStructure);

            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            if (!validate_gpio_mode(GPIO_InitStructure.GPIO_Mode)) {
                m_usart = nullptr;
                return;
            }
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            break;

        case USART_ID:: USART3_ID:
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
            m_usart = USART3;

            // TX: PB10, RX: PB11
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
            if (!validate_gpio_mode(GPIO_InitStructure.GPIO_Mode)) {
                m_usart = nullptr;
                return;
            }
            GPIO_Init(GPIOB, &GPIO_InitStructure);

            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            if (!validate_gpio_mode(GPIO_InitStructure.GPIO_Mode)) {
                m_usart = nullptr;
                return;
            }
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            break;
            
        default:
            // ✅ 无效的USART_ID，不应该发生
            m_usart = nullptr;
            return;
    }
    
    // ✅ 参数验证：最终确认USART实例有效
    // 这是一个防御性检查，确保switch语句正确执行
    if (!validate_usart_instance(m_usart)) {
        m_usart = nullptr;
    }
}

void SimpleSerial::initUSART(uint32_t baud) {
    // ✅ 参数验证：确保USART实例有效
    if (!m_usart || !validate_usart_instance(m_usart)) {
        return;  // USART实例无效，无法初始化
    }
    
    // ✅ 参数验证：再次检查波特率
    if (!validate_baudrate(baud)) {
        baud = 115200;  // 使用默认安全值
    }
    
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure. USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    // ✅ 参数验证：检查USART配置参数
    // WordLength应该是8b或9b
    if (USART_InitStructure.USART_WordLength != USART_WordLength_8b &&
        USART_InitStructure.USART_WordLength != USART_WordLength_9b) {
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    }
    
    // StopBits应该是有效值
    if (USART_InitStructure.USART_StopBits != USART_StopBits_1 &&
        USART_InitStructure.USART_StopBits != USART_StopBits_0_5 &&
        USART_InitStructure.USART_StopBits != USART_StopBits_2 &&
        USART_InitStructure.USART_StopBits != USART_StopBits_1_5) {
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
    }
    
    USART_Init(m_usart, &USART_InitStructure);

    // 启用 DMA
    USART_DMACmd(m_usart, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);

    // 启用空闲中断（用于检测接收完成）
    USART_ITConfig(m_usart, USART_IT_IDLE, ENABLE);

    USART_Cmd(m_usart, ENABLE);
    
    // ✅ 调试日志：输出初始化参数（初始化完成后）
    // 延迟一小段时间确保USART完全就绪
    for (volatile int i = 0; i < 1000; i++);
    
    serial_debug_print(m_usart, "\r\n=== USART Init ===\r\n");
    serial_debug_print(m_usart, "USART: ");
    if (m_usart == USART1) serial_debug_print(m_usart, "USART1");
    else if (m_usart == USART2) serial_debug_print(m_usart, "USART2");
    else if (m_usart == USART3) serial_debug_print(m_usart, "USART3");
    serial_debug_print(m_usart, "\r\nBaudrate: ");
    serial_debug_print_dec(m_usart, baud);
    serial_debug_print(m_usart, "\r\n==================\r\n");
}

void SimpleSerial::initDMA() {
    // ✅ 参数验证：确保USART实例有效
    if (!m_usart || !validate_usart_instance(m_usart)) {
        return;  // USART实例无效，无法配置DMA
    }
    
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
    
    // ✅ 参数验证：确保DMA RX通道已正确分配
    if (!m_dma_rx) {
        return;  // DMA RX通道无效
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
    
    // ✅ 参数验证：检查DMA配置参数
    if (DMA_InitStructure.DMA_BufferSize == 0 || 
        DMA_InitStructure.DMA_BufferSize > 65535) {
        return;  // 缓冲区大小无效
    }
    
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
    
    // ✅ 参数验证：检查优先级配置（范围0-15）
    uint8_t preemption_priority = 3;
    uint8_t sub_priority = 0;
    
    if (preemption_priority > 15) preemption_priority = 3;
    if (sub_priority > 15) sub_priority = 0;
    
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = preemption_priority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = sub_priority;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    // USART 中断
    switch (m_id) {
        case USART_ID:: USART1_ID:
            NVIC_InitStructure. NVIC_IRQChannel = USART1_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            // 只在有DMA TX时启用DMA中断
            if (m_dma_tx) {
                NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
                NVIC_Init(&NVIC_InitStructure);
            }
            break;

        case USART_ID::USART2_ID:
            NVIC_InitStructure. NVIC_IRQChannel = USART2_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            // 只在有DMA TX时启用DMA中断
            if (m_dma_tx) {
                NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
                NVIC_Init(&NVIC_InitStructure);
            }
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