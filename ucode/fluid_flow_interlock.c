/**
  ******************************************************************************
  * @file    fluid_flow_interlock.c
  * @brief   液位/流量联锁控制模块实现文件
  * @version V2.0.0
  * @date    2026-04-27
  * @note    功能说明:
  *          1. SER_IN_1 液位检测: 低电平持续1s判定为有液, 高电平持续1s判定为无液
  *          2. SER_IN_2 低电平有效: 低持续1s判定为有效, 高持续1s判定为无效
  *          3. 液位检测周期: 前5次每30秒检测一次, 之后每60秒检测一次
  *          4. 所有检测在TMR4中断中处理 (1ms周期)
  ******************************************************************************
  */

#include "fluid_flow_interlock.h"
#include "at32f403a_407_wk_config.h"
#include "at32f403a_407_crm.h"
#include "led_beep.h"

/** @brief TMR4 预分频值 */
#define FLUID_TMR4_DIV           23u
/** @brief TMR4 周期值 */
#define FLUID_TMR4_PR            9999u
/** @brief 默认每秒 tick 数 (APB1 时钟计算失败时的回退值) */
#define FLUID_FALLBACK_TICKS_SEC 1000u

/** @brief SER_IN_1 液位消抖 tick 数 (1秒) */
#define SER1_DEBOUNCE_TICKS      (s_ticks_per_sec * 1u)
/** @brief SER_IN_2 有效消抖 tick 数 (1秒) */
#define SER2_DEBOUNCE_TICKS      (s_ticks_per_sec * 1u)

/** @brief 前5次检测间隔 (30秒) */
#define DETECT_INTERVAL_FIRST    (s_ticks_per_sec * 30u)
/** @brief 5次之后检测间隔 (60秒) */
#define DETECT_INTERVAL_LATER    (s_ticks_per_sec * 60u)
/** @brief 检测阶段阈值, 超过此次数后切换为 DETECT_INTERVAL_LATER */
#define DETECT_PHASE_THRESHOLD   5u

/** @brief 无液报警触发时间 (5分钟, 单位:秒) */
#define ALARM_NO_LIQUID_SEC      (5u * 60u)
/** @brief 无液报警触发时间 (5分钟, 单位:tick) */
#define ALARM_NO_LIQUID_TICKS    (s_ticks_per_sec * ALARM_NO_LIQUID_SEC)

/** @brief 报警持续鸣响时间 (3分钟, 单位:秒) */
#define ALARM_DURATION_SEC       (3u * 60u)
/** @brief 报警持续鸣响时间 (3分钟, 单位:tick) */
#define ALARM_DURATION_TICKS     (s_ticks_per_sec * ALARM_DURATION_SEC)

/** @brief 蜂鸣器鸣响切换间隔 (1秒) */
#define ALARM_BEEP_TOGGLE_TICKS  (s_ticks_per_sec * 1u)

/**
 * @brief 报警状态枚举
 * @note  IDLE: 空闲, ACTIVE: 报警鸣响中, SILENT: 报警静默期
 */
typedef enum {
    ALARM_IDLE = 0,
    ALARM_ACTIVE,
    ALARM_SILENT
} AlarmState_t;

/** @brief 每秒 tick 数, 由 TMR4 时钟频率和分频参数计算得出 */
static uint32_t s_ticks_per_sec = FLUID_FALLBACK_TICKS_SEC;

/** @brief SER_IN_1 低电平(有液)连续计数 */
static uint32_t s_ser1_liquid_streak;
/** @brief SER_IN_1 高电平(无液)连续计数 */
static uint32_t s_ser1_dry_streak;
/** @brief 液位正常标志: 1=有液, 0=无液 */
static uint8_t s_liquid_ok;
/** @brief 无液故障标志: 1=无液, 0=有液 */
static uint8_t s_no_liquid;

/** @brief SER_IN_2 低电平(有效)连续计数 */
static uint32_t s_ser2_valid_streak;
/** @brief SER_IN_2 高电平(无效)连续计数 */
static uint32_t s_ser2_invalid_streak;
/** @brief SER_IN_2 低电平有效标志: 1=有效, 0=无效 */
static uint8_t s_ser2_low_level;

/** @brief 检测间隔倒计数器 */
static uint32_t s_detect_counter;
/** @brief 当前检测阶段序号 (用于判断使用哪种检测间隔) */
static uint8_t s_detect_phase;
/** @brief 检测忙标志: 1=正在检测消抖, 0=等待下次检测周期 */
static uint8_t s_detect_busy;

/** @brief 无液状态累计 tick 数 */
static uint32_t s_no_liquid_ticks;
/** @brief 报警鸣响累计 tick 数 */
static uint32_t s_alarm_ticks;
/** @brief 蜂鸣器切换计时 tick 数 */
static uint32_t s_buzzer_ticks;
/** @brief 蜂鸣器当前状态: 1=鸣响, 0=关闭 */
static uint8_t s_buzzer_on;
/** @brief 报警状态机当前状态 */
static AlarmState_t s_alarm_state;

/**
 * @brief  获取 TMR4 的 APB1 定时器时钟频率
 * @note   若 APB1 预分频为1, 则定时器时钟等于 APB1 频率; 否则为 APB1 频率的2倍
 * @return APB1 定时器时钟频率 (Hz)
 */
static uint32_t tmr4_apb1_timer_clock_hz(void)
{
  crm_clocks_freq_type clk;

  crm_clocks_freq_get(&clk);
  if (CRM->cfg_bit.apb1div == CRM_APB1_DIV_1) {
    return clk.apb1_freq;
  }
  return clk.apb1_freq * 2u;
}

/**
 * @brief  读取 SER_IN_1 液位状态
 * @note   低电平表示有液
 * @return 1=有液(低电平), 0=无液(高电平)
 */
static uint8_t ser_in1_liquid_asserted(void)
{
  return (gpio_input_data_bit_read(SER_IN_1_GPIO_PORT, SER_IN_1_PIN) == RESET) ? 1u : 0u;
}

/**
 * @brief  读取 SER_IN_2 有效状态
 * @note   低电平表示有效(与制冷允许条件一致)
 * @return 1=有效(低电平), 0=无效(高电平)
 */
static uint8_t ser_in2_valid_asserted(void)
{
  return (gpio_input_data_bit_read(SER_IN_2_GPIO_PORT, SER_IN_2_PIN) == RESET) ? 1u : 0u;
}

/**
 * @brief  初始化液位/流量联锁模块
 * @note   根据 TMR4 时钟参数计算每秒 tick 数, 并重置所有状态变量
 */
void FluidInterlock_Init(void)
{
  uint32_t tmr_clk = tmr4_apb1_timer_clock_hz();
  uint32_t denom = (FLUID_TMR4_DIV + 1u) * (FLUID_TMR4_PR + 1u);

  s_ticks_per_sec = (denom != 0u) ? (tmr_clk / denom) : 0u;
  if (s_ticks_per_sec == 0u) {
    s_ticks_per_sec = FLUID_FALLBACK_TICKS_SEC;
  }

  s_ser1_liquid_streak = 0;
  s_ser1_dry_streak = 0;
  s_liquid_ok = 0;
  s_no_liquid = 0;

  s_ser2_valid_streak = 0;
  s_ser2_invalid_streak = 0;
  s_ser2_low_level = 0;

  s_detect_counter = 0;
  s_detect_phase = 0;
  s_detect_busy = 1;

  s_no_liquid_ticks = 0;
  s_alarm_ticks = 0;
  s_buzzer_ticks = 0;
  s_buzzer_on = 0;
  s_alarm_state = ALARM_IDLE;
}

/**
 * @brief  TMR4 中断处理函数 (1ms 周期调用)
 * @note   执行 SER_IN_1 液位检测、SER_IN_2 有效检测及无液报警状态机
 */
void FluidInterlock_Tmr4Tick(void)
{
  uint8_t ser1_liquid = ser_in1_liquid_asserted();
  uint8_t ser2_valid = ser_in2_valid_asserted();

  /* SER_IN_1 液位检测: 低电平持续1s判定为有液, 高电平持续1s判定为无液 */
  if (s_detect_busy) {
    if (ser1_liquid) {
      s_ser1_liquid_streak++;
      s_ser1_dry_streak = 0;
      if (s_ser1_liquid_streak >= SER1_DEBOUNCE_TICKS) {
        s_liquid_ok = 1;
        s_no_liquid = 0;
        s_detect_busy = 0;
        s_detect_phase++;
        s_detect_counter = (s_detect_phase <= DETECT_PHASE_THRESHOLD)
                           ? DETECT_INTERVAL_FIRST
                           : DETECT_INTERVAL_LATER;
      }
    } else {
      s_ser1_dry_streak++;
      s_ser1_liquid_streak = 0;
      if (s_ser1_dry_streak >= SER1_DEBOUNCE_TICKS) {
        s_liquid_ok = 0;
        s_no_liquid = 1;
        s_detect_busy = 0;
        s_detect_phase++;
        s_detect_counter = (s_detect_phase <= DETECT_PHASE_THRESHOLD)
                           ? DETECT_INTERVAL_FIRST
                           : DETECT_INTERVAL_LATER;
      }
    }
  } else {
    s_detect_counter--;
    if (s_detect_counter == 0) {
      s_detect_busy = 1;
      s_ser1_liquid_streak = 0;
      s_ser1_dry_streak = 0;
    }
  }

  /* SER_IN_2 低电平有效: 低持续1s判定为有效, 高持续1s判定为无效 */
  if (ser2_valid) {
    s_ser2_valid_streak++;
    s_ser2_invalid_streak = 0;
    if (s_ser2_valid_streak >= SER2_DEBOUNCE_TICKS) {
      s_ser2_low_level = 1;
    }
  } else {
    s_ser2_invalid_streak++;
    s_ser2_valid_streak = 0;
    if (s_ser2_invalid_streak >= SER2_DEBOUNCE_TICKS) {
      s_ser2_low_level = 0;
    }
  }

  /* 液面超时报警状态机 */
  if (s_liquid_ok) {
    s_no_liquid_ticks = 0;
    if (s_alarm_state != ALARM_IDLE) {
      s_alarm_state = ALARM_IDLE;
      s_alarm_ticks = 0;
      s_buzzer_ticks = 0;
      s_buzzer_on = 0;
      BEEP_Off();
    }
  } else {
    s_no_liquid_ticks++;
    if (s_alarm_state == ALARM_IDLE) {
      if (s_no_liquid_ticks >= ALARM_NO_LIQUID_TICKS) {
        s_alarm_state = ALARM_ACTIVE;
        s_alarm_ticks = 0;
        s_buzzer_ticks = 0;
        s_buzzer_on = 1;
        BEEP_On();
      }
    }
  }

  if (s_alarm_state == ALARM_ACTIVE) {
    s_buzzer_ticks++;
    if (s_buzzer_ticks >= ALARM_BEEP_TOGGLE_TICKS) {
      s_buzzer_ticks = 0;
      s_buzzer_on = !s_buzzer_on;
      if (s_buzzer_on) {
        BEEP_On();
      } else {
        BEEP_Off();
      }
    }
    s_alarm_ticks++;
    if (s_alarm_ticks >= ALARM_DURATION_TICKS) {
      s_alarm_state = ALARM_SILENT;
      s_buzzer_on = 0;
      BEEP_Off();
    }
  }
}

/** @brief 主循环处理 (预留接口) */
void FluidInterlock_MainLoopProcess(void)
{
}

/**
 * @brief  查询水泵1是否允许运行
 * @return 1=允许(液位正常), 0=不允许
 */
uint8_t FluidInterlock_IsWpump1Allowed(void)
{
  return s_liquid_ok;
}

/**
 * @brief  查询 SER_IN_2 是否有效
 * @return 1=有效(低电平), 0=无效
 */
uint8_t FluidInterlock_IsSer2Valid(void)
{
  return s_ser2_low_level;
}

/**
 * @brief  获取液位正常标志
 * @return 1=有液, 0=无液
 */
uint8_t FluidInterlock_GetLiquidOk(void)
{
  return s_liquid_ok;
}

/**
 * @brief  获取无液故障标志
 * @return 1=无液故障, 0=液位正常
 */
uint8_t FluidInterlock_GetNoLiquidFault(void)
{
  return s_no_liquid;
}

/**
 * @brief  获取流量边沿数 (简化版)
 * @note   SER_IN_2 有效时返回 9999, 无效时返回 0
 * @return 边沿数/模拟值
 */
uint16_t FluidInterlock_GetEdgesPerMin(void)
{
  return s_ser2_low_level ? 9999u : 0u;
}

/**
 * @brief  获取 SER_IN_2 低电平有效标志
 * @return 1=低电平有效, 0=无效
 */
uint8_t FluidInterlock_GetSer2LowLevel(void)
{
  return s_ser2_low_level;
}

/**
 * @brief  查询制冷是否允许
 * @note   需同时满足液位正常且 SER_IN_2 有效
 * @return 1=允许制冷, 0=不允许
 */
uint8_t FluidInterlock_CoolingAllowed(void)
{
  if (s_no_liquid || !s_liquid_ok) {
    return 0;
  }
  if (!s_ser2_low_level) {
    return 0;
  }
  return 1;
}

/**
 * @brief  查询报警是否激活
 * @return 1=报警中(ACTIVE 或 SILENT), 0=空闲
 */
uint8_t FluidInterlock_IsAlarmActive(void)
{
  return (s_alarm_state != ALARM_IDLE) ? 1u : 0u;
}
