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

valve_control_status_t g_valve_control_status = {0};

static const uint32_t VALVE_INIT_HIGH_TIME_TICKS = VALVE_INIT_HIGH_TIME_MS;
static const uint32_t VALVE_OPEN_LOW_TIME_TICKS = VALVE_OPEN_LOW_TIME_MS;
static const uint32_t VALVE_RUNNING_HIGH_TIME_TICKS = VALVE_RUNNING_HIGH_TIME_MS;

static uint32_t valve_tmr4_apb1_timer_clock_hz(void)
{
  crm_clocks_freq_type clk;

  crm_clocks_freq_get(&clk);
  if (CRM->cfg_bit.apb1div == CRM_APB1_DIV_1) {
    return clk.apb1_freq;
  }
  return clk.apb1_freq * 2u;
}

static uint8_t valve_read_ser_in1(void)
{
  return (gpio_input_data_bit_read(SER_IN_1_GPIO_PORT, SER_IN_1_PIN) == SET) ? 1u : 0u;
}

static void valve_set_output(uint8_t state)
{
  if (state == 0u) {
    gpio_bits_set(SW_VALVE_1_GPIO_PORT, SW_VALVE_1_PIN);
  } else {
    gpio_bits_reset(SW_VALVE_1_GPIO_PORT, SW_VALVE_1_PIN);
  }
}

static void valve_open(void)
{
  valve_set_output(1u);
  g_valve_control_status.valve_open = 1u;
}

static void valve_close(void)
{
  valve_set_output(0u);
  g_valve_control_status.valve_open = 0u;
}

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

static void valve_state_open_handler(void)
{
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;
    g_valve_control_status.state = VALVE_STATE_OPEN_LOW_WAIT;
}

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

static void valve_state_running_handler(uint8_t ser_level)
{
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;

    if (ser_level == 1u) {
        g_valve_control_status.state = VALVE_STATE_RUNNING_HIGH_WAIT;
    }
}

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

valve_state_t ValveControl_GetState(void)
{
    return g_valve_control_status.state;
}

uint8_t ValveControl_IsValveOpen(void)
{
    return g_valve_control_status.valve_open;
}

void ValveControl_ForceClose(void)
{
    g_valve_control_status.state = VALVE_STATE_INIT;
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;
    g_valve_control_status.valve_open = 0u;
    valve_close();
}

valve_control_status_t* ValveControl_GetStatus(void)
{
    return &g_valve_control_status;
}
