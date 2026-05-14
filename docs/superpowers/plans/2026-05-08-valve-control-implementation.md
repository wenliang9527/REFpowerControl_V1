# 注水阀控制功能实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 实现基于SER_IN_1信号的注水阀自动控制功能，包括上电初始化阶段（1秒高电平开阀、5秒低电平关阀）和系统运行阶段（10秒高电平开阀、5秒低电平关阀）

**架构：** 新建独立的 valve_control 模块，复用现有TMR4定时器（1ms周期中断），实现6状态有限状态机处理阀门开关逻辑，与现有 FluidInterlock 模块完全独立运行

**技术栈：** AT32F403A407微控制器、C语言、GPIO控制、定时器中断、状态机设计

---

## 文件结构

### 新建文件

- **ucode/valve_control.h** - 阀门控制模块头文件，定义数据类型、枚举、函数接口
- **ucode/valve_control.c** - 阀门控制模块实现，包含状态机和中断处理逻辑

### 修改文件

- **project/src/at32f403a_407_int.c** - 在TMR4中断中调用 ValveControl_Tmr4Tick()
- **project/src/main.c** - 在初始化阶段调用 ValveControl_Init()
- **ucode/ucode.uvprojx** - 将 valve_control.c 添加到工程

---

## 任务清单

### 任务 1：创建 valve_control.h 头文件

**文件：**
- 创建：`ucode/valve_control.h`

- [ ] **步骤 1：编写头文件基础结构**

```c
/**
  ******************************************************************************
  * @file    valve_control.h
  * @brief   注水阀控制模块头文件
  * @version V1.0.0
  * @date    2026-05-08
  * @note    基于SER_IN_1信号控制注水阀SW_VALVE_1的开关
  *          上电初始化: 高电平持续1s开阀，低电平持续5s关阀
  *          系统运行: 高电平持续10s开阀，低电平持续5s关阀
  ******************************************************************************
  */

#ifndef __VALVE_CONTROL_H
#define __VALVE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"
```

- [ ] **步骤 2：定义时间阈值配置参数**

```c
/** 定时器配置 - 与TMR4保持一致 */
#define VALVE_TMR4_DIV                 23u
#define VALVE_TMR4_PR                 9999u

/** 时间阈值配置 (单位: ms) */
#define VALVE_INIT_HIGH_TIME_MS       1000u
#define VALVE_OPEN_LOW_TIME_MS         5000u
#define VALVE_RUNNING_HIGH_TIME_MS    10000u

/** 备用定时周期 (fallback) */
#define VALVE_FALLBACK_TICKS_PER_SEC   1000u
```

- [ ] **步骤 3：定义阀门GPIO和SER_IN_1 GPIO**

```c
/** 阀门GPIO定义 - 与 at32f403a_407_wk_config.h 保持一致 */
#define SW_VALVE_1_PIN                GPIO_PINS_15
#define SW_VALVE_1_GPIO_PORT          GPIOD
#define SER_IN_1_PIN                  GPIO_PINS_12
#define SER_IN_1_GPIO_PORT            GPIOD
```

- [ ] **步骤 4：定义阀门状态枚举**

```c
/** 阀门状态枚举 */
typedef enum {
    VALVE_STATE_INIT = 0,
    VALVE_STATE_INIT_HIGH_WAIT,
    VALVE_STATE_OPEN,
    VALVE_STATE_OPEN_LOW_WAIT,
    VALVE_STATE_RUNNING,
    VALVE_STATE_RUNNING_HIGH_WAIT
} valve_state_t;
```

- [ ] **步骤 5：定义阀门状态结构体**

```c
/** 阀门状态结构体 */
typedef struct {
    valve_state_t state;
    uint32_t high_streak_counter;
    uint32_t low_streak_counter;
    uint32_t ticks_per_sec;
    uint8_t valve_open;
} valve_control_status_t;
```

- [ ] **步骤 6：定义全局变量和函数接口**

```c
/** 全局变量声明 */
extern valve_control_status_t g_valve_control_status;

/** 公共函数接口 */
void ValveControl_Init(void);
void ValveControl_Tmr4Tick(void);
valve_state_t ValveControl_GetState(void);
uint8_t ValveControl_IsValveOpen(void);
void ValveControl_ForceClose(void);
valve_control_status_t* ValveControl_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __VALVE_CONTROL_H */
```

- [ ] **步骤 7：保存文件并验证语法**

验证头文件格式正确，包含所有必要的声明

---

### 任务 2：创建 valve_control.c 基础框架

**文件：**
- 创建：`ucode/valve_control.c`

- [ ] **步骤 1：编写文件头部注释和includes**

```c
/**
  ******************************************************************************
  * @file    valve_control.c
  * @brief   注水阀控制模块实现文件
  * @version V1.0.0
  * @date    2026-05-08
  * @note    功能说明:
  *          1. 基于SER_IN_1信号控制SW_VALVE_1阀门开关
  *          2. 上电初始化: 高电平持续1s开阀，低电平持续5s关阀
  *          3. 系统运行: 高电平持续10s开阀，低电平持续5s关阀
  *          4. 使用TMR4中断(1ms周期)进行精确定时
  ******************************************************************************
  */

#include "valve_control.h"
#include "at32f403a_407_wk_config.h"
#include "at32f403a_407_crm.h"
```

- [ ] **步骤 2：定义全局状态变量和静态常量**

```c
/** 全局阀门控制状态变量 */
valve_control_status_t g_valve_control_status = {0};

/** 静态常量定义 */
static const uint32_t VALVE_INIT_HIGH_TIME_TICKS = VALVE_INIT_HIGH_TIME_MS;
static const uint32_t VALVE_OPEN_LOW_TIME_TICKS = VALVE_OPEN_LOW_TIME_MS;
static const uint32_t VALVE_RUNNING_HIGH_TIME_TICKS = VALVE_RUNNING_HIGH_TIME_MS;
```

- [ ] **步骤 3：实现辅助函数 - TMR4时钟频率获取**

```c
/**
  * @brief  获取TMR4的APB1时钟频率
  * @retval 时钟频率 (Hz)
  */
static uint32_t valve_tmr4_apb1_timer_clock_hz(void)
{
    crm_clocks_freq_type clk;
    crm_clocks_freq_get(&clk);
    if (CRM->cfg_bit.apb1div == CRM_APB1_DIV_1) {
        return clk.apb1_freq;
    }
    return clk.apb1_freq * 2u;
}
```

- [ ] **步骤 4：实现辅助函数 - 读取SER_IN_1电平**

```c
/**
  * @brief  读取SER_IN_1 GPIO电平状态
  * @retval 0=低电平, 1=高电平
  * @note   高电平表示无液位/需要开阀，低电平表示有液位/可以关阀
  */
static uint8_t valve_read_ser_in1(void)
{
    return (gpio_input_data_bit_read(SER_IN_1_GPIO_PORT, SER_IN_1_PIN) == SET) ? 1u : 0u;
}
```

- [ ] **步骤 5：实现辅助函数 - 设置阀门输出**

```c
/**
  * @brief  设置SW_VALVE_1输出状态
  * @param  state: 0=关闭(高电平), 1=打开(低电平)
  * @note   GPIO极性: 低电平开启阀门，高电平关闭阀门
  */
static void valve_set_output(uint8_t state)
{
    if (state == 0u) {
        gpio_bits_set(SW_VALVE_1_GPIO_PORT, SW_VALVE_1_PIN);
    } else {
        gpio_bits_reset(SW_VALVE_1_GPIO_PORT, SW_VALVE_1_PIN);
    }
}
```

- [ ] **步骤 6：实现辅助函数 - 打开阀门**

```c
/**
  * @brief  打开注水阀
  * @note   设置SW_VALVE_1为低电平，开启阀门
  */
static void valve_open(void)
{
    valve_set_output(1u);
    g_valve_control_status.valve_open = 1u;
}
```

- [ ] **步骤 7：实现辅助函数 - 关闭阀门**

```c
/**
  * @brief  关闭注水阀
  * @note   设置SW_VALVE_1为高电平，关闭阀门
  */
static void valve_close(void)
{
    valve_set_output(0u);
    g_valve_control_status.valve_open = 0u;
}
```

- [ ] **步骤 8：实现ValveControl_Init函数**

```c
/**
  * @brief  阀门控制模块初始化
  * @note   初始化定时器参数，设置阀门为关闭状态，状态机复位到INIT
  */
void ValveControl_Init(void)
{
    uint32_t tmr_clk = valve_tmr4_apb1_timer_clock_hz();
    uint32_t denom = (VALVE_TMR4_DIV + 1u) * (VALVE_TMR4_PR + 1u);

    g_valve_control_status.ticks_per_sec = (denom != 0u) ? (tmr_clk / denom) : 0u;
    if (g_valve_control_status.ticks_per_sec == 0u) {
        g_valve_control_status.ticks_per_sec = VALVE_FALLBACK_TICKS_PER_SEC;
    }

    g_valve_control_status.state = VALVE_STATE_INIT;
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;
    g_valve_control_status.valve_open = 0u;

    valve_close();
}
```

- [ ] **步骤 9：保存文件并验证基础框架**

---

### 任务 3：实现状态机处理函数

**文件：**
- 修改：`ucode/valve_control.c`

- [ ] **步骤 1：实现INIT状态处理函数**

```c
/**
  * @brief  INIT状态处理
  * @param  ser_level: SER_IN_1当前电平 (0=低, 1=高)
  * @note   等待检测到高电平，转换到INIT_HIGH_WAIT状态
  */
static void valve_state_init_handler(uint8_t ser_level)
{
    if (ser_level == 1u) {
        g_valve_control_status.high_streak_counter = 1u;
        g_valve_control_status.low_streak_counter = 0u;
        g_valve_control_status.state = VALVE_STATE_INIT_HIGH_WAIT;
    } else {
        g_valve_control_status.low_streak_counter++;
        g_valve_control_status.high_streak_counter = 0u;
    }
}
```

- [ ] **步骤 2：实现INIT_HIGH_WAIT状态处理函数**

```c
/**
  * @brief  INIT_HIGH_WAIT状态处理
  * @param  ser_level: SER_IN_1当前电平 (0=低, 1=高)
  * @note   高电平持续1秒后打开阀门，转换到OPEN状态
  */
static void valve_state_init_high_wait_handler(uint8_t ser_level)
{
    if (ser_level == 1u) {
        g_valve_control_status.high_streak_counter++;
        if (g_valve_control_status.high_streak_counter >= VALVE_INIT_HIGH_TIME_TICKS) {
            valve_open();
            g_valve_control_status.state = VALVE_STATE_OPEN;
        }
    } else {
        g_valve_control_status.low_streak_counter++;
        if (g_valve_control_status.low_streak_counter >= 1u) {
            g_valve_control_status.state = VALVE_STATE_INIT;
            g_valve_control_status.high_streak_counter = 0u;
            g_valve_control_status.low_streak_counter = 0u;
        }
    }
}
```

- [ ] **步骤 3：实现OPEN状态处理函数**

```c
/**
  * @brief  OPEN状态处理
  * @note   阀门已打开，立即转换到OPEN_LOW_WAIT状态，开始监测低电平
  */
static void valve_state_open_handler(void)
{
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;
    g_valve_control_status.state = VALVE_STATE_OPEN_LOW_WAIT;
}
```

- [ ] **步骤 4：实现OPEN_LOW_WAIT状态处理函数**

```c
/**
  * @brief  OPEN_LOW_WAIT状态处理
  * @param  ser_level: SER_IN_1当前电平 (0=低, 1=高)
  * @note   阀门已打开，等待低电平持续5秒后关闭阀门
  */
static void valve_state_open_low_wait_handler(uint8_t ser_level)
{
    if (ser_level == 0u) {
        g_valve_control_status.low_streak_counter++;
        if (g_valve_control_status.low_streak_counter >= VALVE_OPEN_LOW_TIME_TICKS) {
            valve_close();
            g_valve_control_status.state = VALVE_STATE_RUNNING;
        }
    } else {
        g_valve_control_status.high_streak_counter++;
        if (g_valve_control_status.high_streak_counter >= 1u) {
            g_valve_control_status.low_streak_counter = 0u;
            g_valve_control_status.state = VALVE_STATE_OPEN;
        }
    }
}
```

- [ ] **步骤 5：实现RUNNING状态处理函数**

```c
/**
  * @brief  RUNNING状态处理
  * @param  ser_level: SER_IN_1当前电平 (0=低, 1=高)
  * @note   系统运行阶段，等待检测到高电平信号
  */
static void valve_state_running_handler(uint8_t ser_level)
{
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;

    if (ser_level == 1u) {
        g_valve_control_status.state = VALVE_STATE_RUNNING_HIGH_WAIT;
    }
}
```

- [ ] **步骤 6：实现RUNNING_HIGH_WAIT状态处理函数**

```c
/**
  * @brief  RUNNING_HIGH_WAIT状态处理
  * @param  ser_level: SER_IN_1当前电平 (0=低, 1=高)
  * @note   高电平持续10秒后打开阀门，转换到OPEN状态
  */
static void valve_state_running_high_wait_handler(uint8_t ser_level)
{
    if (ser_level == 1u) {
        g_valve_control_status.high_streak_counter++;
        if (g_valve_control_status.high_streak_counter >= VALVE_RUNNING_HIGH_TIME_TICKS) {
            valve_open();
            g_valve_control_status.state = VALVE_STATE_OPEN;
        }
    } else {
        g_valve_control_status.state = VALVE_STATE_RUNNING;
        g_valve_control_status.high_streak_counter = 0u;
        g_valve_control_status.low_streak_counter = 0u;
    }
}
```

- [ ] **步骤 7：保存文件并验证状态机逻辑**

---

### 任务 4：实现主中断处理函数和公共接口

**文件：**
- 修改：`ucode/valve_control.c`

- [ ] **步骤 1：实现ValveControl_Tmr4Tick主函数**

```c
/**
  * @brief  TMR4定时器中断回调函数
  * @note   在TMR4中断服务程序中调用，1ms周期执行一次
  *         读取SER_IN_1状态，根据当前状态执行状态机处理
  */
void ValveControl_Tmr4Tick(void)
{
    uint8_t ser_level = valve_read_ser_in1();

    switch (g_valve_control_status.state) {
        case VALVE_STATE_INIT:
            valve_state_init_handler(ser_level);
            break;

        case VALVE_STATE_INIT_HIGH_WAIT:
            valve_state_init_high_wait_handler(ser_level);
            break;

        case VALVE_STATE_OPEN:
            valve_state_open_handler();
            break;

        case VALVE_STATE_OPEN_LOW_WAIT:
            valve_state_open_low_wait_handler(ser_level);
            break;

        case VALVE_STATE_RUNNING:
            valve_state_running_handler(ser_level);
            break;

        case VALVE_STATE_RUNNING_HIGH_WAIT:
            valve_state_running_high_wait_handler(ser_level);
            break;

        default:
            g_valve_control_status.state = VALVE_STATE_INIT;
            break;
    }
}
```

- [ ] **步骤 2：实现ValveControl_GetState函数**

```c
/**
  * @brief  获取当前阀门状态机状态
  * @retval 当前状态枚举值
  */
valve_state_t ValveControl_GetState(void)
{
    return g_valve_control_status.state;
}
```

- [ ] **步骤 3：实现ValveControl_IsValveOpen函数**

```c
/**
  * @brief  查询阀门是否已打开
  * @retval 0=阀门关闭, 1=阀门打开
  */
uint8_t ValveControl_IsValveOpen(void)
{
    return g_valve_control_status.valve_open;
}
```

- [ ] **步骤 4：实现ValveControl_ForceClose函数**

```c
/**
  * @brief  强制关闭阀门
  * @note   用于异常情况下的紧急关闭，将状态机复位到INIT
  */
void ValveControl_ForceClose(void)
{
    g_valve_control_status.state = VALVE_STATE_INIT;
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;
    g_valve_control_status.valve_open = 0u;
    valve_close();
}
```

- [ ] **步骤 5：实现ValveControl_GetStatus函数**

```c
/**
  * @brief  获取阀门控制状态结构体指针
  * @retval 状态结构体指针
  */
valve_control_status_t* ValveControl_GetStatus(void)
{
    return &g_valve_control_status;
}
```

- [ ] **步骤 6：保存并验证完整实现**

---

### 任务 5：集成到TMR4中断

**文件：**
- 修改：`project/src/at32f403a_407_int.c`

- [ ] **步骤 1：在includes区域添加valve_control.h**

在文件顶部的includes区域添加：

```c
#include "valve_control.h"
```

- [ ] **步骤 2：在TMR4中断中添加ValveControl_Tmr4Tick调用**

在TMR4_GLOBAL_IRQHandler函数的TMR4_OVF_FLAG处理中添加：

```c
void TMR4_GLOBAL_IRQHandler(void)
{
    if(tmr_interrupt_flag_get(TMR4, TMR_OVF_FLAG) != RESET)
    {
        tmr_flag_clear(TMR4, TMR_OVF_FLAG);
        TouchKey_Scan();
        SensorInput_Scan();
        ValveControl_Tmr4Tick();  // 新增：阀门控制处理
    }
}
```

- [ ] **步骤 3：保存文件并验证修改**

---

### 任务 6：集成到main函数

**文件：**
- 修改：`project/src/main.c`

- [ ] **步骤 1：在includes区域添加valve_control.h**

在 `#include "led_beep.h"` 之后添加：

```c
#include "led_beep.h"
#include "valve_control.h"
```

- [ ] **步骤 2：在初始化区域调用ValveControl_Init**

在 `FluidInterlock_Init();` 之后添加：

```c
ValveControl_Init();  // 新增：阀门控制初始化
```

完整初始化序列应为：

```c
GlobalSensorData_Init();
DeviceControl_Init();
SensorInput_Init();
FluidInterlock_Init();
ValveControl_Init();  // 新增：阀门控制初始化
TouchKey_Init();
```

- [ ] **步骤 3：保存文件并验证修改**

---

### 任务 7：添加到Keil工程

**文件：**
- 修改：`ucode/ucode.uvprojx`

- [ ] **步骤 1：使用文本编辑器打开工程文件**

在 `ucode.uvprojx` 文件中找到 Source Group 1 部分

- [ ] **步骤 2：添加valve_control.c到工程**

在现有源文件条目中添加 valve_control.c：

```xml
<Files>
  <File>
    <FileName>valve_control.c</FileName>
    <FileType>1</FileType>
    <FilePath>ucode\valve_control.c</FilePath>
  </File>
  <File>
    <FileName>pid_control.c</FileName>
    <FileType>1</FileType>
    <FilePath>ucode\pid_control.c</FilePath>
  </File>
  ...
</Files>
```

- [ ] **步骤 3：保存工程文件**

---

### 任务 8：编译验证

**文件：**
- 测试：编译整个项目

- [ ] **步骤 1：使用Keil MDK编译项目**

在Keil MDK中打开 `ucode.uvprojx` 工程文件

点击 Project -> Rebuild all target files 或按 F7

- [ ] **步骤 2：检查编译输出**

预期结果：
```
compiling valve_control.c...
linking...
Program Size: Code=XXX RO-data=XXX RW-data=XXX ZI-data=XXX
0 Error(s), 0 Warning(s).
```

- [ ] **步骤 3：验证无错误和警告**

如果有错误，根据错误信息修正代码
如果有警告，检查并优化代码

---

### 任务 9：代码审查

**文件：**
- 审查：`ucode/valve_control.h`
- 审查：`ucode/valve_control.c`
- 审查：`project/src/at32f403a_407_int.c`
- 审查：`project/src/main.c`

- [ ] **步骤 1：检查头文件格式和声明完整性**

验证所有枚举、结构和函数都已正确定义

- [ ] **步骤 2：检查状态机逻辑正确性**

对照设计文档验证状态转换符合需求

- [ ] **步骤 3：检查GPIO操作正确性**

验证SER_IN_1读取和SW_VALVE_1输出逻辑正确

- [ ] **步骤 4：检查时间阈值配置正确性**

验证 VALVE_INIT_HIGH_TIME_MS = 1000
验证 VALVE_OPEN_LOW_TIME_MS = 5000
验证 VALVE_RUNNING_HIGH_TIME_MS = 10000

- [ ] **步骤 5：检查与现有模块的集成正确性**

验证TMR4中断调用、main函数初始化调用

---

### 任务 10：功能测试准备

**文件：**
- 准备：测试用例文档

- [ ] **步骤 1：编写单元测试用例**

测试用例1：上电后立即检测到高电平
- 预期：1秒后阀门打开

测试用例2：上电后持续低电平
- 预期：阀门保持关闭

测试用例3：阀门打开后收到低电平
- 预期：5秒后阀门关闭

测试用例4：运行阶段收到高电平
- 预期：10秒后阀门打开

- [ ] **步骤 2：准备硬件测试环境**

连接调试器（J-Link）
连接示波器/逻辑分析仪
准备信号发生器

- [ ] **步骤 3：执行手动测试**

按测试用例逐步验证功能

---

## 规格覆盖度检查

### 需求覆盖检查

| 需求项 | 对应任务 | 状态 |
|--------|---------|------|
| 上电后立即检测SER_IN_1信号 | 任务3-6 | ✓ |
| 高电平持续1秒开阀 | 任务3 | ✓ |
| 低电平持续5秒关阀 | 任务3 | ✓ |
| 系统运行阶段持续监测 | 任务3 | ✓ |
| 高电平持续10秒开阀 | 任务3 | ✓ |
| 与FluidInterlock独立运行 | 任务1-6 | ✓ |
| 1ms精度检测 | 任务3 | ✓ |
| 异常处理（ForceClose） | 任务4 | ✓ |

### 遗漏检查

无遗漏，所有设计需求都已覆盖。

## 自检清单

- [x] 所有文件路径正确
- [x] 所有时间阈值配置正确（1s、5s、10s）
- [x] 状态机6个状态都已实现
- [x] GPIO操作逻辑正确
- [x] TMR4中断集成正确
- [x] main函数初始化集成正确
- [x] 遵循项目代码风格
- [x] 无占位符或TODO
- [x] 函数接口完整

---

## 执行选项

计划已完成并保存到 `docs/superpowers/plans/2026-05-08-valve-control-implementation.md`。两种执行方式：

**1. 子代理驱动（推荐）** - 每个任务调度一个新的子代理，任务间进行审查，快速迭代

**2. 内联执行** - 在当前会话中使用 executing-plans 执行任务，批量执行并设有检查点

**选哪种方式？**
