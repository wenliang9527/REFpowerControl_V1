# 制冷 PID 控制设计规格

## 概述

将制冷控制从纯开关迟滞模式改为 PID + PWM 模式，使用帕尔贴制冷片，通过 TMR2_CH4 (PB11) 输出 PWM 信号控制制冷功率。

## 需求

- 制冷启动温度：12℃
- 制冷关闭温度：4℃
- 目标温度：8℃
- PWM 引脚：PB11（TMR2_CH4，需重映射 TMR2_GMUX_10）
- PWM 频率：1kHz（period=9999, div=23, 240MHz/24/10000=1kHz）
- PID 计算周期：2秒（与温度采样同步）
- 最小占空比：20%（低于此值关闭 PWM，避免帕尔贴低效运行）
- 最大占空比：100%

## 硬件配置（已由工具完成）

| 项 | 配置 | 状态 |
|---|---|---|
| PB11 GPIO 模式 | GPIO_MODE_MUX, PUSH_PULL | ✅ 已配置 |
| TMR2 重映射 | TMR2_GMUX_10 (CH4→PB11) | ✅ 已配置 |
| TMR2 CH4 输出 | oc_output_state=TRUE, oc_polarity=ACTIVE_HIGH | ✅ 已配置 |
| TMR2 period/div | 9999/23 → 1kHz | ✅ 已配置 |
| TMR2 CH4 OC 模式 | TMR_OUTPUT_CONTROL_OFF | ⚠️ 需在 PID 初始化时改为 PWM_MODE_A |

## 架构

```
温度采样(2s) → TempControl_Compute()
                     │
                     ├─ 风扇/水泵: 迟滞控制（不变）
                     │
                     └─ 制冷: CoolingPID_Compute()
                                │
                                ├─ 联锁检查(wpump+ser2)
                                ├─ 温度迟滞(12℃开/4℃关)
                                ├─ PID计算
                                └─ PWM占空比输出 → TMR2_CH4
```

## 新建文件

### cooling_pid.h

```c
#define COOLING_PID_TARGET_TEMP     8.0f
#define COOLING_PID_ON_THRESHOLD    12.0f
#define COOLING_PID_OFF_THRESHOLD   4.0f
#define COOLING_PID_MIN_DUTY        20
#define COOLING_PID_MAX_DUTY        100
#define COOLING_PID_INTEGRAL_MAX    50.0f

typedef struct {
    float kp;
    float ki;
    float kd;
    float target;
    float integral;
    float prev_error;
    float output;
    uint8_t pid_active;
    uint8_t duty_percent;
} CoolingPID_t;

void CoolingPID_Init(CoolingPID_t *pid);
void CoolingPID_SetParams(CoolingPID_t *pid, float kp, float ki, float kd);
void CoolingPID_SetTarget(CoolingPID_t *pid, float target);
void CoolingPID_Compute(CoolingPID_t *pid, float current_temp,
                         uint8_t wpump_allowed, uint8_t ser2_valid);
uint8_t CoolingPID_IsActive(CoolingPID_t *pid);
uint8_t CoolingPID_GetDuty(CoolingPID_t *pid);
```

### cooling_pid.c

#### CoolingPID_Init

- 初始化所有字段为0
- 设置默认PID参数：Kp=10.0, Ki=0.5, Kd=2.0
- 设置 target=COOLING_PID_TARGET_TEMP
- 配置 TMR2 CH4 为 PWM 模式（TMR_OUTPUT_CONTROL_PWM_MODE_A）
- 设置初始占空比为0

#### CoolingPID_Compute 逻辑

```
1. 联锁检查：
   if wpump_allowed==0 || ser2_valid==0:
       pid_active = 0
       duty_percent = 0
       SetPWMDuty(0)
       integral = 0
       return

2. 温度迟滞判断：
   if current_temp > COOLING_PID_ON_THRESHOLD(12℃):
       pid_active = 1
   elif current_temp < COOLING_PID_OFF_THRESHOLD(4℃):
       pid_active = 0
   else:
       保持当前 pid_active 状态

3. PID 计算（仅 pid_active==1）：
   error = target - current_temp
   // 温度高于目标 → error>0 → 需要制冷

   integral += error * dt  (dt=1, 因为每次调用间隔=PID周期)
   if integral > INTEGRAL_MAX: integral = INTEGRAL_MAX
   if integral < 0: integral = 0  // 不允许反向积分

   derivative = error - prev_error

   output = Kp*error + Ki*integral + Kd*derivative

   if output < 0: output = 0
   if output > 100: output = 100

   prev_error = error

4. 占空比映射：
   if pid_active==0 || output < MIN_DUTY:
       duty_percent = 0
       SetPWMDuty(0)
   else:
       duty_percent = (uint8_t)output
       SetPWMDuty(duty_percent)
```

#### CoolingPID_SetPWMDuty（内部静态函数）

```c
static void CoolingPID_SetPWMDuty(uint8_t duty_percent)
{
    uint32_t period = tmr_period_value_get(TMR2) + 1;
    uint32_t ccr = (uint32_t)duty_percent * period / 100;

    if (duty_percent == 0) {
        tmr_output_channel_enable(TMR2, TMR_SELECT_CHANNEL_4, FALSE);
        tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_4, 0);
    } else {
        tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_4, ccr);
        tmr_output_channel_enable(TMR2, TMR_SELECT_CHANNEL_4, TRUE);
    }
}
```

## 修改文件

### temp_control.h

- 删除 `COOLING_TEMP_ON_THRESHOLD`、`COOLING_TEMP_OFF_THRESHOLD`、`COOLING_RETRY_CYCLES` 宏
- 删除 `TempController_t` 中的 `cooling_on`、`cooling_retry_counter` 字段
- 修改 `TempControl_Compute` 签名，增加 `CoolingPID_t *pid` 参数
- 增加 `#include "cooling_pid.h"` 前向声明

### temp_control.c

- 删除所有 `SW_REF` 的 GPIO 操作（gpio_bits_set/reset）
- `TempControl_Init`：删除 SW_REF GPIO 初始化
- `TempControl_Disable`：删除 SW_REF GPIO 操作，改为调用 `CoolingPID_Compute` 关闭
- `TempControl_Reset`：同上
- `TempControl_SetTarget`：改为调用 `CoolingPID_SetTarget`
- `TempControl_Compute`：
  - 风扇/水泵迟滞逻辑保留不变
  - 制冷部分改为调用 `CoolingPID_Compute(pid, current_temp, wpump1_allowed, ser2_valid)`
- `TempControl_GetOutput`：改为返回 `CoolingPID_GetDuty(pid)`

### device_control.c / device_control.h

- 从设备列表中移除 `DEVICE_SW_REF`（索引0）
- 调整后续设备索引编号
- `DEVICE_NUM` 从 8 改为 7

### main.c

- 新增 `#include "cooling_pid.h"`
- 新增 `static CoolingPID_t cooling_pid;`
- 初始化部分添加 `CoolingPID_Init(&cooling_pid);`
- `TempControl_Compute` 调用增加 `&cooling_pid` 参数

## PID 参数初始值

| 参数 | 值 | 说明 |
|------|-----|------|
| Kp | 10.0 | 温度偏差1℃ → 10%占空比 |
| Ki | 0.5 | 积分系数，消除稳态误差 |
| Kd | 2.0 | 微分系数，抑制超调 |
| integral_max | 50.0 | 积分限幅 |

## 不受影响的功能

| 功能 | 原因 |
|------|------|
| 风扇控制 | 迟滞逻辑保留在 temp_control.c |
| 水泵控制 | 迟滞逻辑+液位联锁保留 |
| 液面报警 | fluid_flow_interlock.c 独立 |
| 数码管显示 | 温度显示逻辑不变 |
| 注水阀控制 | valve_control.c 独立 |
| 触摸按键 | touch_key.c 独立 |
| TMR3 数码管扫描 | TMR3 独立 |
| TMR4 传感器扫描 | TMR4 独立 |
