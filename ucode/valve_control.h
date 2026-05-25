/**
  ******************************************************************************
  * @file    valve_control.h
  * @brief   注水阀控制模块头文件
  * @version V2.0.0
  * @date    2026-05-15
  * @note    基于SER_IN_1信号控制注水阀SW_VALVE_1的开关
  *          上电初始化: 高电平持续1s开阀，低电平持续5s关阀
  *          系统运行: 高电平持续10s开阀，低电平持续5s关阀
  *          V2.0: 重构为TMR4中断驱动(1ms/tick)，阈值直接用ms
  ******************************************************************************
  */

#ifndef __VALVE_CONTROL_H
#define __VALVE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define VALVE_INIT_HIGH_TIME_MS       1000u
#define VALVE_OPEN_LOW_TIME_MS         5000u
#define VALVE_RUNNING_HIGH_TIME_MS    10000u
#define VALVE_DEBOUNCE_MS               50u

#define SW_VALVE_1_PIN                GPIO_PINS_15
#define SW_VALVE_1_GPIO_PORT          GPIOD
#define SER_IN_1_PIN                  GPIO_PINS_12
#define SER_IN_1_GPIO_PORT            GPIOD

typedef enum {
    VALVE_STATE_INIT = 0,
    VALVE_STATE_INIT_HIGH_WAIT,
    VALVE_STATE_OPEN,
    VALVE_STATE_OPEN_LOW_WAIT,
    VALVE_STATE_RUNNING,
    VALVE_STATE_RUNNING_HIGH_WAIT
} valve_state_t;

typedef struct {
    valve_state_t state;
    uint32_t high_streak_counter;
    uint32_t low_streak_counter;
    uint8_t valve_open;
} valve_control_status_t;

extern valve_control_status_t g_valve_control_status;

void ValveControl_Init(void);

void ValveControl_Process(void);

valve_state_t ValveControl_GetState(void);

uint8_t ValveControl_IsValveOpen(void);

void ValveControl_ForceClose(void);

valve_control_status_t* ValveControl_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif
