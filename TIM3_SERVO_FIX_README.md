# TIM3 舵机修复说明 (TIM3 Servo Fix Documentation)

## 问题描述 (Problem Description)

**中文**: 一旦开启串口之后，PB0 的 TIM3 就不能驱动舵机了。

**English**: Once UART is enabled, TIM3 on PB0 cannot drive the servo anymore.

## 根本原因 (Root Cause)

当串口初始化时，代码会启用 AFIO（Alternative Function I/O）时钟：
```cpp
RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
```

启用 AFIO 后，GPIO 重映射功能变为可用状态。如果没有明确配置，TIM3 的引脚可能被意外重映射到其他位置，导致 PB0 上的 TIM3_CH3 失效。

**In English**: 
When UART is initialized, the code enables the AFIO (Alternative Function I/O) clock. Once AFIO is enabled, GPIO remapping functionality becomes active. Without explicit configuration, TIM3 pins might get accidentally remapped to different locations, causing TIM3_CH3 on PB0 to stop working.

## TIM3 引脚映射 (TIM3 Pin Mapping)

STM32F103 的 TIM3 有三种映射模式：

### 1. 无重映射 (No Remap) - 默认
- CH1 = PA6
- CH2 = PA7
- CH3 = **PB0** ← 舵机使用此引脚
- CH4 = PB1

### 2. 部分重映射 (Partial Remap)
- CH1 = PB4
- CH2 = PB5
- CH3 = PB0
- CH4 = PB1

### 3. 完全重映射 (Full Remap)
- CH1 = PC6
- CH2 = PC7
- CH3 = PC8
- CH4 = PC9

## 解决方案 (Solution)

在启用 AFIO 后，明确禁用 TIM3 的所有重映射模式，确保使用默认引脚：

```cpp
// 禁用 TIM3 部分重映射
GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, DISABLE);

// 禁用 TIM3 完全重映射
GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, DISABLE);
```

## 修改的文件 (Modified Files)

### 1. `src/Servo/Servo.cpp`

在 `enableAFIOClock()` 函数中添加：
```cpp
void Servo::enableAFIOClock() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    // 确保 TIM3 使用默认引脚映射
    if (timer_m == TIM3) {
        GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, DISABLE);
        GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, DISABLE);
    }
}
```

### 2. `src/SerialPort.cpp`

在 `initGPIO()` 函数中，AFIO 启用后添加：
```cpp
void SimpleSerial::initGPIO() {
    // ... GPIO 初始化代码 ...
    
    // 启用 AFIO
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    // 禁用 TIM3 重映射，确保 PB0 上的舵机能正常工作
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, DISABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, DISABLE);
}
```

## 测试步骤 (Testing Steps)

### 1. 硬件连接
- 将舵机信号线连接到 **PB0** (TIM3_CH3)
- 连接串口工具到 USART1 (PA9/PA10) 或其他串口

### 2. 编译并烧录
1. 使用 Keil/IAR/STM32CubeIDE 编译项目
2. 将固件烧录到 STM32 开发板

### 3. 验证舵机功能
观察舵机是否能正常转动：
- 应该在 160° 和 180° 之间摆动
- 每 500ms 切换一次角度

### 4. 验证串口功能
使用串口调试助手：
- 波特率：115200
- 应该能收到 "USART1 Ready!" 和心跳消息
- 可以发送测试数据

## 预期结果 (Expected Results)

✅ **修复前**: 开启串口后，舵机停止工作  
✅ **修复后**: 串口和舵机可以同时正常工作

## 技术细节 (Technical Details)

### 为什么 USART3 不会冲突？
USART3 使用的引脚是：
- TX: PB10
- RX: PB11

这些引脚与 TIM3 的默认引脚（PB0, PB1）不冲突。

### DMA 冲突已解决
根据 `IMPLEMENTATION_SUMMARY.md`，USART3 的 DMA 冲突已通过以下方式解决：
- USART3 TX 使用中断模式（不使用 DMA）
- USART3 RX 使用 DMA1_Channel3
- TIM3 相关的 DMA 不受影响

## 兼容性 (Compatibility)

此修复完全兼容：
- ✅ 舵机 PWM 控制 (TIM3)
- ✅ USART1/2/3 串口通信
- ✅ OLED 显示
- ✅ 事件循环框架
- ✅ 定时器系统

## 故障排查 (Troubleshooting)

### 如果舵机仍然不工作：

1. **检查硬件连接**
   - 确认舵机电源正常（通常 5V）
   - 信号线确实连接到 PB0
   - 地线已正确连接

2. **检查引脚配置**
   ```cpp
   // 在 main.cpp 中，确认使用的是：
   Servo* servo1_ptr = new Servo(TIM3, Servo::Channel::CH3, GPIOB, GPIO_Pin_0);
   ```

3. **检查时钟配置**
   - 确认 GPIOB 时钟已启用
   - 确认 TIM3 时钟已启用
   - 确认 AFIO 时钟已启用

4. **使用示波器测试**
   - PB0 应该输出 50Hz 的 PWM 信号
   - 脉宽应该在 0.5ms - 2.5ms 之间

## 参考资料 (References)

- STM32F10x Reference Manual (RM0008)
- STM32F10x Standard Peripheral Library Documentation
- GPIO Remapping: Section 9.3.7 in RM0008

## 更新历史 (Change History)

- **2026-01-14**: 初始修复 - 添加 TIM3 重映射禁用代码
