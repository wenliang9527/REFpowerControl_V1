/**
  ******************************************************************************
  * @file    temp_control.h
  * @brief   制冷温度阈值控制模块头文件
  * @version V6.0.0
  * @date    2026-06-12
  * @note    温度双阈值迟滞控制 + SER_IN_1/2 联锁 + GPIO
  *          - 温度 > 6℃ / < 3℃ / 3~6℃ 迟滞: 风扇、水泵需求及制冷温度许可
  *          - 水泵受 wpump1_allowed(SER_IN_1 液位联锁), 制冷另需 ser2_valid(SER_IN_2)
  *          - 制冷 SW_REF: 低=开, 高=关; 含关闭后重试计数再允许合闸
  ******************************************************************************
  */

#ifndef __TEMP_CONTROL_H
#define __TEMP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define COOLING_TEMP_ON_THRESHOLD    6.0f
#define COOLING_TEMP_OFF_THRESHOLD   3.0f
#define COOLING_RETRY_CYCLES         6

typedef struct {
    float current_temp;
    uint8_t fan1_on;
    uint8_t wpump1_on;
    uint8_t cooling_on;
    uint8_t cooling_retry_counter;
} TempController_t;

void TempControl_Init(TempController_t *ctrl);
void TempControl_Enable(TempController_t *ctrl);
void TempControl_Disable(TempController_t *ctrl);
void TempControl_Reset(TempController_t *ctrl);
void TempControl_SetTargetTemp(TempController_t *ctrl, float temp);
void TempControl_Compute(TempController_t *ctrl, float current_temp, uint8_t wpump1_allowed, uint8_t ser2_valid);
float TempControl_GetOutput(TempController_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif
