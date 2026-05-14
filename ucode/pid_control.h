/**
  ******************************************************************************
  * @file    pid_control.h
  * @brief   制冷温度阈值控制模块头文件
  * @version V5.0.0
  * @date    2026-04-27
 * @note    不含 PID(P/I/D) 算法, 仅为温度双阈值迟滞 + SER_IN_1/2 联锁 + GPIO
 *          - 温度 > 6℃ / < 3℃ / 3~6℃ 迟滞: 风扇、水泵需求及制冷温度许可
 *          - 水泵受 wpump1_allowed(SER_IN_1 液位联锁), 制冷另需 ser2_valid(SER_IN_2)
 *          - 制冷 SW_REF: 低=开, 高=关; 含关闭后重试计数再允许合闸
  ******************************************************************************
  */

#ifndef __PID_CONTROL_H
#define __PID_CONTROL_H

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
} PID_Controller_t;

void PID_Init(PID_Controller_t *pid);
void PID_Enable(PID_Controller_t *pid);
void PID_Disable(PID_Controller_t *pid);
void PID_Reset(PID_Controller_t *pid);
void PID_SetTargetTemp(PID_Controller_t *pid, float temp);
void PID_Compute(PID_Controller_t *pid, float current_temp, uint8_t wpump1_allowed, uint8_t ser2_valid);
float PID_GetOutput(PID_Controller_t *pid);

#ifdef __cplusplus
}
#endif

#endif
