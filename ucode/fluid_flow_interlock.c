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

#define FLUID_TMR4_DIV           23u
#define FLUID_TMR4_PR            9999u
#define FLUID_FALLBACK_TICKS_SEC 1000u

#define SER1_DEBOUNCE_TICKS      (s_ticks_per_sec * 1u)
#define SER2_DEBOUNCE_TICKS      (s_ticks_per_sec * 1u)

#define DETECT_INTERVAL_FIRST    (s_ticks_per_sec * 30u)
#define DETECT_INTERVAL_LATER    (s_ticks_per_sec * 60u)
#define DETECT_PHASE_THRESHOLD   5u

static uint32_t s_ticks_per_sec = FLUID_FALLBACK_TICKS_SEC;

static uint32_t s_ser1_liquid_streak;
static uint32_t s_ser1_dry_streak;
static uint8_t s_liquid_ok;
static uint8_t s_no_liquid;

static uint32_t s_ser2_valid_streak;
static uint32_t s_ser2_invalid_streak;
static uint8_t s_ser2_low_level;

static uint32_t s_detect_counter;
static uint8_t s_detect_phase;
static uint8_t s_detect_busy;

static uint32_t tmr4_apb1_timer_clock_hz(void)
{
  crm_clocks_freq_type clk;

  crm_clocks_freq_get(&clk);
  if (CRM->cfg_bit.apb1div == CRM_APB1_DIV_1) {
    return clk.apb1_freq;
  }
  return clk.apb1_freq * 2u;
}

/** SER_IN_1 低电平有效: 低=有液 */
static uint8_t ser_in1_liquid_asserted(void)
{
  return (gpio_input_data_bit_read(SER_IN_1_GPIO_PORT, SER_IN_1_PIN) == RESET) ? 1u : 0u;
}

/** SER_IN_2 低电平有效: 低=流量/联锁条件有效(与制冷允许一致) */
static uint8_t ser_in2_valid_asserted(void)
{
  return (gpio_input_data_bit_read(SER_IN_2_GPIO_PORT, SER_IN_2_PIN) == RESET) ? 1u : 0u;
}

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
}

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
}

void FluidInterlock_MainLoopProcess(void)
{
}

uint8_t FluidInterlock_IsWpump1Allowed(void)
{
  return s_liquid_ok;
}

uint8_t FluidInterlock_IsSer2Valid(void)
{
  return s_ser2_low_level;
}

uint8_t FluidInterlock_GetLiquidOk(void)
{
  return s_liquid_ok;
}

uint8_t FluidInterlock_GetNoLiquidFault(void)
{
  return s_no_liquid;
}

uint16_t FluidInterlock_GetEdgesPerMin(void)
{
  return s_ser2_low_level ? 9999u : 0u;
}

uint8_t FluidInterlock_GetSer2LowLevel(void)
{
  return s_ser2_low_level;
}

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
