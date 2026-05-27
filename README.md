# REFpowerControl_V1 — 制冷电源控制板固件

基于 **ArteryTek AT32F403AVGT7** (ARM Cortex-M4F, 240MHz, 1MB Flash, 96KB SRAM) 的 Peltier 半导体制冷控制系统。

---

## 一、整体架构与运行逻辑

### 1.1 系统层级

```
┌──────────────────────────────────────────────────────────────────────────┐
│  应用层 (ucode/)                                                         │
│  PID温控  温度管理  注水阀FSM  触摸按键  数码管显示  LED/蜂鸣器          │
│  液位联锁  传感器输入  DS18B20  设备控制                                 │
├──────────────────────────────────────────────────────────────────────────┤
│  BSP层 (project/src/)                                                    │
│  主循环(main.c)  中断处理(int.c)  时钟/GPIO/定时器/DMA/USART配置        │
├──────────────────────────────────────────────────────────────────────────┤
│  HAL层 (libraries/drivers/)                                              │
│  AT32F403A_407 标准外设库 (StdPeriph)                                    │
├──────────────────────────────────────────────────────────────────────────┤
│  CMSIS层 (libraries/cmsis/)                                              │
│  Cortex-M4 内核抽象层  启动文件  链接脚本                                │
└──────────────────────────────────────────────────────────────────────────┘
```

### 1.2 系统框图

```
  ┌─────────────────────────────────────────────────────┐
  │                     TMR4 (1kHz)                     │
  │  TouchKey_Scan() → 4路电容触摸按键消抖与状态机       │
  │  SensorInput_Scan() → SER_IN_1/2/3 50ms消抖        │
  │    └─ FluidInterlock_Tmr4Tick() → 液位/流量联锁     │
  └──────────────┬──────────────────────────┬───────────┘
                 │                          │
                 ▼                          ▼
  ┌──────────────────────────┐  ┌─────────────────────────┐
  │   TMR3 (1kHz)            │  │   TMR1 (10kHz)          │
  │   DigitTube_Scan()       │  │   LED_Thread()          │
  │   → 数码管扫描          │  │   → LED闪烁+蜂鸣器     │
  └──────────────────────────┘  └─────────────────────────┘

  ┌─────────────────────────────────────────────────────┐
  │                  main.c 超循环                        │
  │                                                     │
  │  while(1) {                                         │
  │      TouchKey_EventProcess();   // 按键事件         │
  │      ValveControl_Process();    // 注水阀FSM         │
  │                                                     │
  │      if (DS18B2O 温度就绪) {                         │
  │          TempControl_Compute();  // PID+风扇/水泵    │
  │          DigitTube_DisplayTemp(); // 刷新显示        │
  │      }                                              │
  │  }                                                   │
  └─────────────────────────────────────────────────────┘
```

### 1.3 初始化顺序 (`main.c`)

```
① wk_system_clock_config()     系统时钟 (HICK+PLL→240MHz)
② wk_periph_clock_config()     外设时钟使能 (GPIO A/B/C/D/E, TMR1-5, USART1, DMA1)
③ wk_debug_config()            释放JTAG/SWD调试引脚
④ wk_nvic_config()             中断优先级分组 + 使能各中断
⑤ wk_timebase_init()           SysTick延时基准 (us/ms)
⑥ wk_gpio_config()             GPIO初始化
⑦ wk_dma1_channel1/2_init()    USART1 DMA配置
⑧ wk_usart1_init()             USART1 115200/8N1
⑨ wk_tmr1~5_init()             定时器初始化
⑩ 业务模块初始化:
     DeviceControl_Init()       → 7路设备GPIO (默认关闭)
     SensorInput_Init()         → 传感器消抖状态清零
     FluidInterlock_Init()      → 液位联锁定时参数
     ValveControl_Init()        → 注水阀FSM复位
     TouchKey_Init()            → 按键状态清零
     DigitTube_Init()           → 数码管GPIO+定时器
     TempControl_Init()         → 温控器初始化 (风扇/水泵默认关)
     CoolingPID_Init()          → PID参数初始化 (Kp=10, Ki=0.5, Kd=2)
     DS18B20_Init()             → DS18B20传感器搜索
     LED_PowerIndicator_Set(0)  → 电源指示灯默认关
     BEEP_Control(1000, 1)      → 开机蜂鸣器提示
```

### 1.4 时钟树

```
HEXT (8MHz) ─┐
              ├── PLL(x60) ──→ SCLK = 240MHz ──→ AHB = 240MHz
HICK (8MHz) ─┘                                              │
                                                    ┌───────┴───────┐
                                                    ▼               ▼
                                                APB1=120MHz    APB2=120MHz
                                              (TMR2~5: 240M)  (TMR1: 240M)
```

---

## 二、功能模块详解

---

## 模块1: PID温度控制 (Cooling PID)

### 功能说明
增量式 PID 控制器，计算 Peltier 制冷片的目标占空比，通过 PWM 输出驱动。

### 执行逻辑
```
CoolingPID_Compute()
  ├─ 联锁检查: 水泵未允许 或 SER2无效 → 关闭PID, 占空比=0
  ├─ 滞回判定: 温度 > 12°C → 激活PID
  │            温度 <  4°C → 停用PID
  ├─ PID计算 (Ki有下界钳位0, 无负积分):
  │   error = target(8°C) - current_temp
  │   integral += error  (上限50.0, 下限0.0)
  │   derivative = error - prev_error
  │   output = Kp*error + Ki*integral + Kd*derivative  (钳位0~100)
  ├─ 占空比映射: output < 20% → 输出0; 否则输出output%
  └─ CoolingPID_SetPWMDuty() → 写TMR2_CH4 CCR
```

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| TMR2 | PWM 发生器 (1kHz, CH4) |
| SW_REF (PB11) | PWM 输出引脚 → Peltier 驱动 (GPIO 复用模式, 配置在 wk_tmr2_init) |

注: SW_REF 的 GPIO 配置在 `wk_tmr2_init()` 中完成 (GPIO_MODE_MUX + GPIO 重映射 TMR2_GMUX_10)，不在 wk_gpio_config 中处理。

### 引脚配置
| 信号 | 端口 | 引脚 | 复用功能 |
|------|------|------|---------|
| SW_REF | GPIOB | 11 | TMR2_CH4 (PWM输出), IO 重映射 TMR2_GMUX_10 |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/cooling_pid.h` | 结构体定义、常量宏、函数声明 |
| `ucode/cooling_pid.c` | PID 算法实现、PWM 输出控制 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `CoolingPID_Init()` | `main.c` 初始化 | 设置默认 Kp/Ki/Kd, 目标温度8°C, 关闭PWM输出 |
| `CoolingPID_Compute()` | `temp_control.c` → `TempControl_Compute()` | 核心PID计算，含联锁检查+滞回+防积分饱和 |
| `CoolingPID_SetPWMDuty()` | 本文件内部 | 写TMR2_CH4 CCR寄存器，控制占空比 |

### 关键常量
| 宏 | 值 | 说明 |
|----|-----|------|
| `COOLING_PID_TARGET_TEMP` | 8.0°C | PID 目标温度 |
| `COOLING_PID_ON_THRESHOLD` | 12.0°C | PID 开启温度 |
| `COOLING_PID_OFF_THRESHOLD` | 4.0°C | PID 关闭温度 |
| `COOLING_PID_MIN_DUTY` | 20% | 最小有效占空比 |
| `COOLING_PID_MAX_DUTY` | 100% | 最大占空比 |
| `COOLING_PID_INTEGRAL_MAX` | 50.0 | 积分上限 (抗饱和) |
| `COOLING_PID_DEFAULT_KP` | 10.0 | 比例系数 |
| `COOLING_PID_DEFAULT_KI` | 0.5 | 积分系数 |
| `COOLING_PID_DEFAULT_KD` | 2.0 | 微分系数 |

---

## 模块2: 温度管理 (Temp Control)

### 功能说明
协调 PID 控制器与风扇/水泵的开关控制，根据温度滞回阈值决策。

### 执行逻辑
```
TempControl_Compute()
  ├─ 更新当前温度
  ├─ 滞回控制:
  │   温度 > 12°C → 开启风扇1、水泵1
  │   温度 <  4°C → 关闭风扇1、水泵1
  │   中间区域 → 保持当前状态
  ├─ 水泵1受液位联锁约束 (wpump1_allowed)
  └─ 调用 CoolingPID_Compute() 计算PID
```

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| SW_FAN_1 (PA12) | 风扇1 控制输出 |
| SW_WPUMP_1 (PA15) | 水泵1 控制输出 |

### 引脚配置
| 信号 | 端口 | 引脚 | 有效电平 |
|------|------|------|---------|
| SW_FAN_1 | GPIOA | 12 | 低电平=开启 (GPIO复位) |
| SW_WPUMP_1 | GPIOA | 15 | 低电平=开启 (GPIO复位) |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/temp_control.h` | TempController_t 结构体, 函数声明 |
| `ucode/temp_control.c` | 温度管理调度, 风扇/水泵GPIO直操 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `TempControl_Init()` | `main.c` 初始化 | 清零状态, 关风扇/水泵 |
| `TempControl_Enable()` | 初始化后 | 预留启用接口 |
| `TempControl_Disable()` | — | 强制关风扇/水泵 + PID |
| `TempControl_Compute()` | `main.c` 主循环 | 核心控制: 滞回判定 + GPIO输出 + PID调用 |

---

## 模块3: 注水阀控制 (Valve Control)

### 功能说明
基于 SER_IN_1 (PD12) 液位信号，通过 6 状态 FSM 控制 SW_VALVE_1 (PD15) 阀门。上电初始化阶段和系统运行阶段采用不同的时间阈值。

### 6 状态 FSM

```
                  ┌──────────────────────────────────────────────┐
                  │                                              │
                  ▼                                              │
  ┌──────────┐ SER_IN_1=H  ┌────────────────┐                  │
  │  INIT    │────────────→│  INIT_HIGH_WAIT │                  │
  │ (关阀)   │←─消抖复位──│  (等待H持续1s)  │                  │
  └──────────┘             └───────┬────────┘                  │
                                  │ H持续≥1s                   │
                                  ▼                            │
  ┌──────────┐             ┌────────────────┐                  │
  │ RUNNING  │←────────────│     OPEN       │                  │
  │ (等待H)  │   SER_IN_1=H│  (开阀成功)    │                  │
  └─────┬────┘   (RUN阶段) └───────┬────────┘                  │
        │                          │                           │
        │ SER_IN_1=H              │ SER_IN_1=L                │
        ▼                          ▼                            │
  ┌──────────┐             ┌────────────────┐                  │
  │RUNNING_  │────────────→│  OPEN_LOW_WAIT │                  │
  │HIGH_WAIT │  H持续≥10s  │  (等待L持续5s)  │                  │
  │(等待H)   │  开阀       └───────┬────────┘                  │
  └──────────┘                    │ L持续≥5s                   │
                                  ▼                            │
                             ┌──────────┐──────────────────────┘
                             │ RUNNING  │
                             └──────────┘
```

### 时序阈值
| 阶段 | 条件 | 阈值 | 动作 |
|------|------|------|------|
| 初始化 | SER_IN_1=H 持续 | 1000ms (1s) | 开阀 → OPEN |
| 初始化 | SER_IN_1=L 持续 | 50ms (消抖) | 回 INIT |
| 运行 | 脱离 OPEN 后 | 立即进入 | OPEN_LOW_WAIT |
| 开阀后 | SER_IN_1=L 持续 | 5000ms (5s) | 关阀 → RUNNING |
| 运行 | SER_IN_1=H 持续 | 10000ms (10s) | 开阀 → OPEN |
| 运行 | SER_IN_1=L 持续 | 50ms (消抖) | 回 RUNNING |

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| SER_IN_1 (PD12) | 液位传感器输入 (低电平=有液) |
| SW_VALVE_1 (PD15) | 阀门控制输出 |

### 引脚配置
| 信号 | 端口 | 引脚 | 有效电平 |
|------|------|------|---------|
| SER_IN_1 | GPIOD | 12 | 低=有液; 高=无液 |
| SW_VALVE_1 | GPIOD | 15 | 低电平=开阀 |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/valve_control.h` | 6状态枚举, 时间阈值宏, 函数声明 |
| `ucode/valve_control.c` | FSM实现, 读取SER_IN_1, 控制SW_VALVE_1 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `ValveControl_Init()` | `main.c` 初始化 | 状态→INIT, 关阀 |
| `ValveControl_Process()` | `main.c` 主循环 | 读取SER_IN_1电平, 驱动6状态FSM |
| `ValveControl_ForceClose()` | 外部调用 | 强制复位到INIT并关阀 |
| `ValveControl_IsValveOpen()` | 查询接口 | 返回当前阀门开关状态 |

### 内部处理函数
| 函数 | 用途 |
|------|------|
| `valve_read_ser_in1()` | 读取SER_IN_1 GPIO电平 |
| `valve_set_output()` | 设置SW_VALVE_1 GPIO |
| `valve_state_init_handler()` | INIT 状态处理 |
| `valve_state_init_high_wait_handler()` | INIT_HIGH_WAIT 状态处理 |
| `valve_state_open_handler()` | OPEN 状态处理 (过渡到 OPEN_LOW_WAIT) |
| `valve_state_open_low_wait_handler()` | OPEN_LOW_WAIT 状态处理 |
| `valve_state_running_handler()` | RUNNING 状态处理 |
| `valve_state_running_high_wait_handler()` | RUNNING_HIGH_WAIT 状态处理 |

---

## 模块4: 液位/流量联锁 (Fluid Flow Interlock)

### 功能说明
检测液位 (SER_IN_1) 和流量/联锁信号 (SER_IN_2)，控制制冷系统是否允许运行。低电平有效，1s 消抖确认，周期性检测，超时报警。

### 执行逻辑

**SER_IN_1 液位检测:**
```
在 TMR4 中断中每 1ms 采样:
  ┌─ SER_IN_1=低(有液) → liquid_streak++ → ≥1000次 → liquid_ok=1
  └─ SER_IN_1=高(无液) → dry_streak++    → ≥1000次 → liquid_ok=0, no_liquid=1
检测周期:
  前5次: 每30秒检测一次 (DETECT_INTERVAL_FIRST)
  之后:  每60秒检测一次 (DETECT_INTERVAL_LATER)
```

**SER_IN_2 流量联锁:**
```
  ┌─ SER_IN_2=低(有效) → valid_streak++ → ≥1000次 → ser2_low_level=1
  └─ SER_IN_2=高(无效) → invalid_streak++ → ≥1000次 → ser2_low_level=0
```

**报警状态机:**
```
无液持续时间 ≥ 5分钟 → ALARM_ACTIVE (蜂鸣器1s间隔交替鸣响)
报警持续时间 ≥ 3分钟 → ALARM_SILENT (蜂鸣器静音)
有液恢复              → ALARM_IDLE (清除报警)
报警时主循环显示 Err 99
```

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| SER_IN_1 (PD12) | 液位检测输入 (低电平=有液) |
| SER_IN_2 (PD13) | 流量/联锁输入 (低电平=允许) |

### 引脚配置
| 信号 | 端口 | 引脚 | 有效电平 |
|------|------|------|---------|
| SER_IN_1 | GPIOD | 12 | 低=有液 |
| SER_IN_2 | GPIOD | 13 | 低=有效(允许制冷) |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/fluid_flow_interlock.h` | 状态查询接口声明 |
| `ucode/fluid_flow_interlock.c` | 液位检测、流量联锁、报警状态机实现 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `FluidInterlock_Init()` | `main.c` 初始化 | 计算定时器tick频率, 清零所有状态 |
| `FluidInterlock_Tmr4Tick()` | `sensor_input.c` → `SensorInput_Scan()` | 每1ms执行: 液位/流量消抖+检测周期+报警状态机 |
| `FluidInterlock_IsWpump1Allowed()` | `main.c` → `TempControl_Compute()` | 返回 `s_liquid_ok` |
| `FluidInterlock_IsSer2Valid()` | `main.c` → `TempControl_Compute()` | 返回 `s_ser2_low_level` |
| `FluidInterlock_IsAlarmActive()` | `main.c` 主循环 | 返回报警状态, 用于数码管显示Err |
| `FluidInterlock_CoolingAllowed()` | 综合接口 | 同时检查液位+流量 |

### 关键常量
| 宏 | 值 | 说明 |
|----|-----|------|
| SER1_DEBOUNCE_TICKS | 1s (1000 ticks) | 液位消抖时间 |
| SER2_DEBOUNCE_TICKS | 1s (1000 ticks) | 流量消抖时间 |
| DETECT_INTERVAL_FIRST | 30s | 前5次检测间隔 |
| DETECT_INTERVAL_LATER | 60s | 后续检测间隔 |
| ALARM_NO_LIQUID_TICKS | 5min (300s) | 无液报警触发时间 |
| ALARM_DURATION_TICKS | 3min (180s) | 报警持续时间 |
| ALARM_BEEP_TOGGLE_TICKS | 1s | 报警蜂鸣器切换间隔 |

---

## 模块5: 触摸按键 (Touch Key)

### 功能说明
4 通道电容触摸按键检测，含消抖、短按判定、TOUCH_1 (电源键) 5 秒锁定保护。

### 执行逻辑
```
TouchKey_Scan()  [TMR4中断, 1ms周期]
  ├─ touch1_lock_process() — 锁定计时
  ├─ 对每个按键:
  │    ├─ 锁定中: 跳过TOUCH_1
  │    └─ key_state_machine() → 消抖状态机:
  │        RELEASE → DEBOUNCE(30次消抖) → PRESS
  │        PRESS持续 ≥ TOUCH1_SHORT_PRESS_TIME(5000)
  │          → 切换电源状态, 控制M_POWER_C, 更新LED
  └─ TOUCH_1触发后 → 锁定30000次扫描 (~30s)

TouchKey_EventProcess()  [main.c主循环]
  └─ 处理待处理的短按事件 → TouchKeyShortpressevent()
```

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| TOUCH_1 (PA8) | 电源键: 5s长按切换总电源 |
| TOUCH_2 (PC9) | 预留 |
| TOUCH_3 (PC8) | 预留 |
| TOUCH_4 (PC7) | 预留 |

### 引脚配置
| 信号 | 端口 | 引脚 | 有效电平 | GPIO 上拉 |
|------|------|------|---------|-----------|
| TOUCH_1 | GPIOA | 8 | 低电平=按下 (RESET) | PULL_UP |
| TOUCH_2 | GPIOC | 9 | 低电平=按下 | PULL_UP |
| TOUCH_3 | GPIOC | 8 | 低电平=按下 | PULL_UP |
| TOUCH_4 | GPIOC | 7 | 低电平=按下 | PULL_UP |

注: 触摸按键 GPIO 配置为 `GPIO_PULL_UP` 输入模式，确保未触摸时引脚为高电平。

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/touch_key.h` | 按键编号、事件枚举、状态结构体定义 |
| `ucode/touch_key.c` | 消抖、状态机、锁定、事件处理 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `TouchKey_Init()` | `main.c` 初始化 | 清零所有按键状态 |
| `TouchKey_Scan()` | `TMR4_GLOBAL_IRQHandler()` | 中断扫描: 消抖+状态机+锁定 |
| `TouchKey_EventProcess()` | `main.c` 主循环 | 事件派发: 电源开关控制 |
| `TouchKeyShortpressevent()` | 本文件内部 | 按键事件处理 (当前框架预留) |

### 关键常量
| 宏 | 值 | 说明 |
|----|-----|------|
| `DEBOUNCE_COUNT` | 30 | 消抖采样次数 (30ms) |
| `SHORT_PRESS_TIME` | 150 | 普通键短按判定 (150ms) |
| `TOUCH1_SHORT_PRESS_TIME` | 5000 | TOUCH_1 短按判定 (5s) |
| `TOUCH1_LOCK_TIME` | 30000 | TOUCH_1 锁定时间 (~30s) |

---

## 模块6: 数码管显示 (Digit Tube)

### 功能说明
3 位 6 引脚查理复用 (Charlieplexing) 数码管驱动，通过 TMR3 中断定时扫描实现动态显示。支持温度值显示、错误码显示、单位切换。

### 执行逻辑
```
DigitTube_Scan()  [TMR3中断, 1kHz → 每~450Hz完成一轮全扫描]
  ├─ g_scan_counter++ (全局扫描计数器, 供main.c做2s定时)
  ├─ 计算当前扫描位置 scan_pos (digit×7 + seg)
  ├─ 每轮扫描: 3位数 × 7段 = 21个扫描位
  ├─ 扫描结束后: 显示LED小数点 (Q1=°C, Q2=°F)
  ├─ 错误模式: 显示 "Err" 字符
  └─ 正常模式: 显示数值 (2位或3位, 消隐前导零)
```

### Charlieplexing 引脚映射
| 引脚 | 端口 | 引脚 | 初始电平 |
|------|------|------|---------|
| DTUBE_1 | PD9 | 低 |
| DTUBE_2 | PD8 | 低 |
| DTUBE_3 | PB15 | 低 |
| DTUBE_4 | PB14 | 低 |
| DTUBE_5 | PB13 | 低 |
| DTUBE_6 | PB12 | 低 |

注: 所有数码管引脚初始化为低电平，由扫描函数 `DigitTube_Scan()` 动态切换为推挽输出。

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| TMR3 | 扫描定时器 (1kHz 中断) |
| GPIOB 12-15 + GPIOD 8-9 | 6引脚数码管驱动 |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/digit_tube.h` | 引脚宏、段码定义、函数声明 |
| `ucode/digit_tube.c` | 查理复用扫描、段码表、显示控制 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `DigitTube_Init()` | `main.c` 初始化 | 初始化数码管GPIO + TMR3定时器 |
| `DigitTube_Scan()` | `TMR3_GLOBAL_IRQHandler()` | 中断扫描: 每个中断点亮一个段 |
| `DigitTube_DisplayTemp()` | `main.c` 主循环 | 设置温度值 + 单位LED |
| `DigitTube_DisplayErrorCode()` | `main.c` 联锁报警 | 显示错误码 (强制3位, 前导零) |
| `DigitTube_ClearErrorCode()` | `main.c` 正常状态 | 恢复2位显示模式 |
| `DigitTube_ToggleUnit()` | 外部调用 | °C/°F 切换 + 数值换算 |

### 段码表
| 数字 | 段码 (abcdefg) |
|------|----------------|
| 0 | 0x3F |
| 1 | 0x06 |
| 2 | 0x5B |
| 3 | 0x4F |
| 4 | 0x66 |
| 5 | 0x6D |
| 6 | 0x7D |
| 7 | 0x07 |
| 8 | 0x7F |
| 9 | 0x6F |

错误码: E=0x79, r=0x50, r=0x50

---

## 模块7: 温度传感器 (DS18B20)

### 功能说明
双路 DS18B20 温度传感器驱动，1-Wire 协议，非阻塞采集模式。

### 执行逻辑
```
DS18B20_NonBlockingProcess()
  ├─ 状态机: IDLE → CONVERTING (启动转换) → READ (读取结果)
  ├─ CONVERTING: 等待 200ms 转换完成
  ├─ READ: 读取两个传感器温度 → 计算平均值
  └─ 更新全局温度数据 g_sensor_data

主循环中每 2s 触发一次读取:
  if (elapsed >= DS18B20_UPDATE_INTERVAL_TICKS(2000ms)) {
      DS18B20_NonBlockingProcess()
      current_temp = DS18B20_GetValidTemp()
      TempControl_Compute(&temp_controller, current_temp, ...)
      DigitTube_DisplayTemp(DS18B20_GetValidDisplayValue())
  }
```

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| TEMP_1 (PD11) | DS18B20 传感器1 数据线 (1-Wire) |
| TEMP_2 (PD10) | DS18B20 传感器2 数据线 (1-Wire) |

### 引脚配置
| 信号 | 端口 | 引脚 | 说明 |
|------|------|------|------|
| TEMP_1 (DS18B20_2) | GPIOD | 11 | 1-Wire 开漏通信 |
| TEMP_2 (DS18B20_1) | GPIOD | 10 | 1-Wire 开漏通信 |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/DS18B20.h` | 设备结构体, 命令宏, 非阻塞状态枚举 |
| `ucode/DS18B20.c` | 1-Wire时序, 温度采集, 平均值计算, 全局数据管理 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `DS18B20_Init()` | `main.c` 初始化 | 搜索总线上的DS18B20设备 |
| `DS18B20_NonBlockingProcess()` | `main.c` 主循环 | 非阻塞采集状态机 |
| `DS18B20_GetValidTemp()` | `main.c` 主循环 | 获取有效温度平均值 |
| `DS18B20_GetValidDisplayValue()` | `main.c` 主循环 | 获取显示用温度值 |
| `GlobalSensorData_Init()` | `main.c` 初始化 | 清零全局传感器数据 |

---

## 模块8: 设备控制 (Device Control)

### 功能说明
7 路设备 GPIO 的底层控制接口，提供统一的 `control_device()` API。

### 执行逻辑
```
control_device(device_code, switch_state)
  ├─ 参数校验: device_code≥DEVICE_NUM 或 state 非法 → 返回
  └─ GPIO操作:
     DEVICE_STATE_HIGH → gpio_bits_set()
     DEVICE_STATE_LOW  → gpio_bits_reset()
```

### 引脚配置
| 设备宏 | 设备名 | GPIO | 默认电平 |
|--------|--------|------|---------|
| `DEVICE_SW_FAN_1` (0) | 风扇1 | PA12 | 高(关) |
| `DEVICE_SW_FAN_2` (1) | 风扇2 | PA11 | 高(关) |
| `DEVICE_SW_WPUMP_1` (2) | 水泵1 | PA15 | 高(关) |
| `DEVICE_M_POWER_C` (3) | 总电源 | PC11 | **低(关)** |
| `DEVICE_SW_WPUMP_2` (4) | 水泵2 | PC10 | 高(关) |
| `DEVICE_SW_VALVE_1` (5) | 阀门1 | PD15 | 高(关) |
| `DEVICE_SW_VALVE_2` (6) | 阀门2 | PC6 | 高(关) |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/device_control.h` | 设备编号宏定义 |
| `ucode/device_control.c` | 设备GPIO映射表 + 初始化 + control_device() |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `DeviceControl_Init()` | `main.c` 初始化 | 配置7路GPIO为推挽输出, 默认关闭(高电平), M_POWER_C特殊处理为低 |
| `control_device()` | `touch_key.c` 等 | 统一设备控制接口 |

注: SW_REF (PB11) 已从设备控制中移除，现由 cooling_pid 模块直接通过 TMR2_CH4 PWM 控制。

---

## 模块9: LED + 蜂鸣器 (LED & BEEP)

### 功能说明
5 路 LED 控制和蜂鸣器驱动。LED 低电平点亮，蜂鸣器高电平鸣响。

### 执行逻辑
```
LED_Thread()  [TMR1中断, 10kHz → 实际每4000次翻转一次LED1]
  ├─ LED1: 周期闪烁 (LED1_FLASH_TIME=4000次 ≈ 0.4s @10kHz)
  ├─ 电源指示灯闪烁: POWER_BLINK_PERIOD=5000次
  │     电源开 → 闪烁 LED_POWER_C
  │     电源关 → 闪烁 LED2
  └─ BEEP_Process(): 蜂鸣器状态机
       状态0=停止 → 状态1=鸣响 → 状态2=间隔 → 循环count次 → 停止

BEEP_Control(on_time_ms, count):
  ├─ on_time_ms × 4 = 鸣响周期数
  └─ 启动蜂鸣器状态机 (鸣响=间隔=on_time_ms×4)
```

### 引脚配置
| 信号 | 端口 | 引脚 | 有效电平 |
|------|------|------|---------|
| LED1 | GPIOE | 14 | 低电平=点亮 (默认点亮) |
| LED2 | GPIOC | 13 | 低电平=点亮 |
| LED3 | GPIOC | 14 | 低电平=点亮 |
| LED4 | GPIOC | 15 | 低电平=点亮 |
| LED5 | GPIOA | 0 | 低电平=点亮 |
| LED_POWER_C | GPIOB | 10 | 低电平=电源指示 |
| BEEP | GPIOC | 12 | 高电平=鸣响 (默认低电平关闭) |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/led_beep.h` | LED翻转宏, 蜂鸣器控制宏, 函数声明 |
| `ucode/led_beep.c` | LED闪烁线程, 蜂鸣器状态机, 电源指示 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `LED_Thread()` | `TMR1_OVF_TMR10_IRQHandler()` | LED1周期闪烁 + 电源指示灯闪烁 + 蜂鸣器处理 |
| `LED_SetState()` | 外部调用 | 设置指定LED亮/灭 (1~5) |
| `LED_Toggle()` | 外部调用 | 翻转指定LED |
| `BEEP_Control()` | `main.c` 初始化 | 启动蜂鸣器定时鸣响 |
| `BEEP_Process()` | `LED_Thread()` 内部 | 蜂鸣器状态机控制 |
| `LED_PowerIndicator_Set()` | `touch_key.c` | 电源指示灯 (联动 M_POWER_C) |
| `LED_PowerIndicator_BlinkStart()` | `touch_key.c` | 电源键按下时LED闪烁提示 |
| `LED_PowerIndicator_BlinkStop()` | `touch_key.c` | 停止闪烁 |

---

## 模块10: 传感器输入 (Sensor Input)

### 功能说明
3 通道传感器输入消抖处理，50ms 消抖时间，为液位/流量联锁模块提供稳定的输入信号。

### 执行逻辑
```
SensorInput_Scan()  [TMR4中断, 1ms周期]
  └─ 对每个通道 (SER_IN_1/2/3):
       ├─ 读取GPIO原始电平
       ├─ 电平与当前状态不一致 → debounce_count++
       │   debounce_count ≥ 50 → 确认状态变化
       └─ 电平一致 → 清零 debounce_count, 增加稳定计数
  └─ 最后调用 FluidInterlock_Tmr4Tick()
```

### 涉及的硬件外设
| 外设 | 用途 |
|------|------|
| SER_IN_1 (PD12) | 液位检测 |
| SER_IN_2 (PD13) | 流量/联锁 |
| SER_IN_3 (PD14) | 预留传感器 |

### 引脚配置
| 信号 | 端口 | 引脚 | 说明 |
|------|------|------|------|
| SER_IN_1 | GPIOD | 12 | 液位检测 (低=有液) |
| SER_IN_2 | GPIOD | 13 | 流量/联锁 (低=有效) |
| SER_IN_3 | GPIOD | 14 | 预留 |

### 文件
| 文件 | 说明 |
|------|------|
| `ucode/sensor_input.h` | 传感器编号, 消抖时间宏, 状态结构体 |
| `ucode/sensor_input.c` | GPIO读取, 消抖算法, 扫描调度 |

### 主要函数
| 函数 | 调用位置 | 说明 |
|------|---------|------|
| `SensorInput_Init()` | `main.c` 初始化 | 清零3通道传感器状态 |
| `SensorInput_Scan()` | `TMR4_GLOBAL_IRQHandler()` | 每1ms扫描: 消抖 + 触发联锁 |
| `SensorInput_GetState()` | 外部查询 | 获取指定通道消抖后状态 |

---

## 三、定时器资源总表

| 定时器 | PSC | ARR | 频率 | 功能 | 中断服务函数 |
|--------|-----|-----|------|------|-------------|
| TMR1 | 23 | 999 | 10kHz (0.1ms) | LED闪烁 + 蜂鸣器 | `TMR1_OVF_TMR10_IRQHandler()` → `LED_Thread()` |
| TMR2 | 23 | 9999 | 1kHz (1ms) | PWM输出 (CH4) | `TMR2_GLOBAL_IRQHandler()` |
| TMR3 | 23 | 9999 | 1kHz (1ms) | 数码管扫描 | `TMR3_GLOBAL_IRQHandler()` → `DigitTube_Scan()` |
| TMR4 | 23 | 9999 | 1kHz (1ms) | 触摸按键 + 传感器+联锁 | `TMR4_GLOBAL_IRQHandler()` → `TouchKey_Scan()` + `SensorInput_Scan()` |
| TMR5 | 23 | 9999 | 1kHz (1ms) | 预留 (中断未使能) | — |

注: 所有定时器时钟 = APB时钟 × 2 = 240MHz (因 APB1/APB2 分频=2, 定时器时钟自动倍频)

---

## 四、中断优先级配置

| 中断 | 优先级(主/子) | 说明 |
|------|-------------|------|
| DMA1_CH1 | 0/1 | USART1_TX DMA |
| DMA1_CH2 | 0/1 | USART1_RX DMA |
| TMR1_OVF_TMR10 | 0/2 | LED/蜂鸣器线程 |
| TMR2_GLOBAL | 0/1 | PWM控制 |
| TMR3_GLOBAL | 1/3 | 数码管扫描 (最低优先级) |
| TMR4_GLOBAL | 0/0 | 最高优先级: 触摸按键+传感器|
| USART1 | 0/0 | 串口通信 |

注: NVIC 优先级分组 = NVIC_PRIORITY_GROUP_2 (2位抢占, 2位子优先级)

---

## 五、中断调用链

```
TMR4_GLOBAL_IRQHandler (1ms, 最高优先级)
  ├── TouchKey_Scan()
  │     ├── touch1_lock_process()       — TOUCH_1 锁定计时
  │     └── key_state_machine(0~3)      — 4路按键状态机
  └── SensorInput_Scan()
        ├── sensor_debounce_process(0~2) — 3路传感器消抖
        └── FluidInterlock_Tmr4Tick()
              ├── SER_IN_1 液位检测  (1s消抖+周期检测)
              ├── SER_IN_2 流量联锁  (1s消抖)
              └── 无液报警状态机

TMR3_GLOBAL_IRQHandler (1ms)
  └── DigitTube_Scan()
        └── 6引脚Charlieplexing扫描

TMR1_OVF_TMR10_IRQHandler (0.1ms)
  ├── LED_Thread()
  │     ├── LED1 周期闪烁 (4000次翻转)
  │     ├── 电源指示灯闪烁 (5000次翻转)
  │     └── BEEP_Process() — 蜂鸣器状态机
```

---

## 六、构建与使用

### 硬件要求
- MCU: AT32F403AVGT7 (LQFP100, Cortex-M4F)
- 调试器: J-Link (SWD)
- 外部晶振: 8MHz (HEXT)

### 编译

**Keil MDK v5 (推荐)**
1. 打开 `project/MDK_V5/REFpowerControl_V1.uvprojx`
2. 编译器: ARMCC V5.06 update 6 (build 750)
3. 编译并下载

**Artery Workbench**
1. 打开 `REFpowerControl_V1.ATWP`
2. 编译并下载

### 调试
- J-Link SWD, 配置见 `project/MDK_V5/JLinkSettings.ini`
- USART1: 115200bps, 8N1, DMA 模式

### 开发规范
- **BSP层** (`project/src/`): Artery Workbench 生成的初始化代码，不添加业务逻辑
- **业务层** (`ucode/`): 所有业务逻辑模块在此目录，与 BSP 解耦
- 新模块: 在 `ucode/` 下创建 `.c/.h`，在 `uvprojx` 中添加文件引用，在 `main.c` 中包含头文件并调用初始化/主循环函数

---

## 七、外设模块启用配置

定义于 `project/inc/at32f403a_407_conf.h`:

| 已启用 | 已禁用 |
|--------|--------|
| CRM, DEBUG, DMA, FLASH, GPIO, MISC, PWC, TMR, USART | ACC, ADC, BPR, CAN, CRC, DAC, EMAC, EXINT, I2C, RTC, SDIO, SPI, USB, WDT, WWDT, XMC |

---

## 八、版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| V1.0.0 | 2026-03 | 基于 AT32F403A 的初版固件 |
| V1.1.0 | 2026-04-24 | 完成模块移植: PID, 阀门, 触摸按键, 数码管, DS18B20, LED/蜂鸣器等 |
| V2.0.0 | 2026-05-15 | 注水阀重构: 3状态→6状态FSM, TMR4中断驱动 |
| V2.1.0 | 2026-05-25 | 新增 cooling_pid PID 模块; TMR2_CH4 PWM 移至 wk_tmr2_init; SW_REF 从 device_control 移除; 触摸按键改为上拉输入; 数码管默认电平修正; NVIC 优先级重构; 新增 TMR5; 液位超时报警 + DS18B20 10位分辨率 |
