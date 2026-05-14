/**
  ******************************************************************************
  * @file    device_control.h
  * @brief   设备控制驱动头文件
  * @version V2.0.0
  * @date    2026-04-27
  * @note    提供多路设备开关控制功能
  *          GPIO极性为低电平有效: gpio_bits_set = 关闭, gpio_bits_reset = 开启
  ******************************************************************************
  */

#ifndef __DEVICE_CONTROL_H
#define __DEVICE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define DEVICE_SW_REF          0
#define DEVICE_SW_FAN_1        1
#define DEVICE_SW_FAN_2        2
#define DEVICE_SW_WPUMP_1      3
#define DEVICE_M_POWER_C       4
#define DEVICE_SW_WPUMP_2      5
#define DEVICE_SW_VALVE_1      6
#define DEVICE_SW_VALVE_2      7

#define DEVICE_NUM             8

#define DEVICE_STATE_LOW       0
#define DEVICE_STATE_HIGH      1

void DeviceControl_Init(void);

void control_device(uint8_t device_code, uint8_t switch_state);

#ifdef __cplusplus
}
#endif

#endif
