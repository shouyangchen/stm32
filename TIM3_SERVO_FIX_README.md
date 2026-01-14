# TIM3 舵机修复说明 (TIM3 Servo Fix Documentation)

## 问题描述 (Problem Description)

**中文**: 一旦开启串口之后，PB0 的 TIM3 就不能驱动舵机了。

**English**: Once UART is enabled, TIM3 on PB0 cannot drive the servo anymore.

## 根本原因 (Root Cause)

问题的根源是 **AFIO 时钟启用的时序错误**，而不是 GPIO 重映射问题。

在 STM32 中，使用复用功能（如 USART、TIM PWM）的 GPIO 引脚必须遵循特定的初始化顺序：

1. ✅ 先启用 AFIO 时钟
2. ✅ 再启用 GPIO 时钟
3. ✅ 然后配置 GPIO 引脚
4. ✅ 最后配置外设

原代码在 `SerialPort::initGPIO()` 中的错误顺序：
```cpp
// ❌ 错误的顺序
void SimpleSerial::initGPIO() {
    // 1. 启用 GPIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 2. 配置 GPIO 引脚 (此时 AFIO 还未启用！)
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 最后才启用 AFIO (太晚了！)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}
```

**In English**: 
The root cause is **incorrect AFIO clock enable timing**, not GPIO remapping.

In STM32, GPIO pins using alternate functions (like USART, TIM PWM) must follow a specific initialization sequence:

1. ✅ Enable AFIO clock FIRST
2. ✅ Enable GPIO clock
3. ✅ Configure GPIO pins
4. ✅ Configure peripheral

The original code in `SerialPort::initGPIO()` had the wrong order, enabling AFIO after GPIO configuration, which interfered with the already-initialized servo on PB0.

## 解决方案 (Solution)

将 AFIO 时钟启用移到 GPIO 配置**之前**：

```cpp
// ✅ 正确的顺序
void SimpleSerial::initGPIO() {
    // 1. 首先启用 AFIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. 启用 GPIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 3. 配置 GPIO 引脚
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}
```

这样可以确保：
- 舵机在 PB0 上的 TIM3_CH3 不会被干扰
- 串口初始化不会影响已配置的 GPIO
- 两个外设可以同时正常工作

## 引脚配置 (Pin Configuration)

### TIM3 (舵机 Servo)
- 使用默认映射（无需重映射）
- CH1 = PA6
- CH2 = PA7
- **CH3 = PB0** ← 舵机使用此引脚
- CH4 = PB1

### USART3 (串口)
- 使用默认映射（无需重映射）
- TX = PB10
- RX = PB11

**结论**: 两个外设的引脚没有冲突，使用默认配置即可。

## 修改的文件 (Modified Files)

### `src/SerialPort.cpp`

**改动**: 将 AFIO 时钟启用从函数末尾移到开头

```cpp
void SimpleSerial::initGPIO() {
    // ✅ 修复：必须在配置 GPIO 之前启用 AFIO 时钟
    // AFIO 时钟必须先于任何复用功能 GPIO 配置启用
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    // ... 其余的 GPIO 配置代码 ...
}
```

## 之前的错误尝试 (Previous Failed Attempt)

**错误方法**: 尝试使用 `GPIO_PinRemapConfig(..., DISABLE)` 来禁用重映射

```cpp
// ❌ 这会导致系统冻结！
GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, DISABLE);
GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, DISABLE);
```

**问题**: 
- `GPIO_PinRemapConfig` 的第二个参数应该是 `ENABLE`，不是 `DISABLE`
- 在 STM32 标准外设库中，使用 `DISABLE` 会导致未定义行为
- 这导致了串口冻结和系统卡死

**正确做法**: 
- 不需要调用任何重映射函数
- 默认情况下，外设使用默认引脚
- 只需要确保 AFIO 时钟在正确的时机启用

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
- **不应该出现卡死或冻结**

## 预期结果 (Expected Results)

✅ **修复后**: 
- 串口正常发送和接收数据，无冻结
- 舵机能正常转动
- 两个功能可以同时工作

## 技术要点 (Technical Points)

### 为什么 AFIO 必须先启用？

AFIO（Alternate Function I/O）控制器管理引脚的复用功能映射。在配置 GPIO 为复用模式（`GPIO_Mode_AF_PP`）之前，必须先启用 AFIO 时钟，否则：

1. GPIO 配置可能无法正确应用
2. 已配置的复用功能 GPIO 可能被重置
3. 引脚功能可能不正确

### 初始化顺序很重要

在 `main()` 中：
```cpp
int main() {
    // 1. 先创建舵机（启用 TIM3 和 AFIO）
    servo1_ptr = new Servo(TIM3, Servo::Channel::CH3, GPIOB, GPIO_Pin_0);
    
    // 2. 后创建应用（初始化串口）
    Applications a;  // 调用 setupSerial()
}
```

关键：
- 舵机初始化时已经正确启用了 AFIO
- 串口初始化时**不能**改变 AFIO 的状态或时序
- 解决方法：串口也在配置 GPIO 前启用 AFIO

## 故障排查 (Troubleshooting)

### 如果串口仍然卡死：

1. **检查 AFIO 启用位置**
   ```cpp
   // 必须在这里！
   void SimpleSerial::initGPIO() {
       RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // ← 第一行
       // ...
   }
   ```

2. **检查是否有重复的 AFIO 启用**
   - 多次启用 AFIO 是安全的
   - 但不要尝试禁用或重置 AFIO

3. **检查编译优化级别**
   - 某些优化可能影响初始化顺序
   - 尝试降低优化级别测试

### 如果舵机仍然不工作：

1. **检查引脚冲突**
   - 确认没有其他功能使用 PB0
   - 检查 TIM3 时钟已启用

2. **检查初始化顺序**
   - 舵机应该在串口之前初始化（当前是正确的）

3. **使用示波器测试**
   - PB0 应该输出 50Hz 的 PWM 信号
   - 脉宽应该在 0.5ms - 2.5ms 之间

## 兼容性 (Compatibility)

此修复完全兼容：
- ✅ 舵机 PWM 控制 (TIM3)
- ✅ USART1/2/3 串口通信（无冻结）
- ✅ OLED 显示
- ✅ 事件循环框架
- ✅ 定时器系统

## 参考资料 (References)

- STM32F10x Reference Manual (RM0008) - Section 9.3.7 (GPIO Alternate Functions)
- STM32F10x Standard Peripheral Library Documentation
- Application Note: STM32 GPIO Configuration

## 更新历史 (Change History)

- **2026-01-14 (初次尝试)**: 添加 GPIO_PinRemapConfig 禁用代码 - ❌ 导致系统冻结
- **2026-01-14 (修复)**: 修改 AFIO 启用时序 - ✅ 正确的解决方案


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

**注意**: 本项目使用默认映射，不需要任何重映射配置。

## USART3 引脚映射 (USART3 Pin Mapping)

### 1. 无重映射 (No Remap) - 默认
- TX = **PB10** ← 当前使用
- RX = **PB11** ← 当前使用

### 2. 部分重映射 (Partial Remap)
- TX = PC10
- RX = PC11

### 3. 完全重映射 (Full Remap)
- TX = PD8
- RX = PD9

**注意**: 本项目使用默认映射，不需要任何重映射配置。

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

这些引脚与 TIM3 的默认引脚（PA6, PA7, PB0, PB1）不冲突。

### DMA 冲突已解决
根据 `IMPLEMENTATION_SUMMARY.md`，USART3 的 DMA 冲突已通过以下方式解决：
- USART3 TX 使用中断模式（不使用 DMA）
- USART3 RX 使用 DMA1_Channel3
- TIM3 相关的 DMA 不受影响

### AFIO 时钟启用时机的重要性
- AFIO 必须在配置复用功能 GPIO 之前启用
- 在已有复用功能 GPIO 运行时启用 AFIO 不会干扰它们
- 重要的是不要尝试禁用或重映射已配置的外设

## 兼容性 (Compatibility)

此修复完全兼容：
- ✅ 舵机 PWM 控制 (TIM3)
- ✅ USART1/2/3 串口通信（无冻结）
- ✅ OLED 显示
- ✅ 事件循环框架
- ✅ 定时器系统

## 参考资料 (References)

- STM32F10x Reference Manual (RM0008) - Section 9.3.7 (GPIO Alternate Functions)
- STM32F10x Standard Peripheral Library Documentation
- Application Note: STM32 GPIO Configuration

## 更新历史 (Change History)

- **2026-01-14 (初次尝试)**: 添加 GPIO_PinRemapConfig 禁用代码 - ❌ 导致系统冻结
- **2026-01-14 (修复)**: 修改 AFIO 启用时序 - ✅ 正确的解决方案
