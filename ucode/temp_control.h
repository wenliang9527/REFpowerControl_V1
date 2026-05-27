/**
  ******************************************************************************
  * @file    temp_control.h
  * @brief   温度控制模块头文件 — 风扇与水泵的开关控制
  * @note    控制策略:
  *          温度 > FAN_TEMP_ON_THRESHOLD(12°C): 开启风扇和水泵
  *          温度 < FAN_TEMP_OFF_THRESHOLD(4°C): 关闭风扇和水泵
  *          温度在两者之间: 保持当前状态(滞回)
  *          水泵需额外满足 wpump1_allowed 条件
  *          GPIO 高电平=关闭, 低电平=开启
  ******************************************************************************
  */

#ifndef __TEMP_CONTROL_H
#define __TEMP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"
#include "cooling_pid.h"

/** 风扇/水泵开启温度阈值, 温度超过此值时开启 */
#define FAN_TEMP_ON_THRESHOLD       12.0f

/** 风扇/水泵关闭温度阈值, 温度低于此值时关闭 */
#define FAN_TEMP_OFF_THRESHOLD      4.0f

/** 温度控制器状态结构体 */
typedef struct {
    float current_temp;     /**< 当前温度(°C) */
    uint8_t fan1_on;        /**< 风扇1开关状态: 0=关, 1=开 */
    uint8_t wpump1_on;      /**< 水泵1开关状态: 0=关, 1=开 */
} TempController_t;

/**
 * @brief  初始化温度控制器
 * @param  ctrl: 温度控制器指针
 * @note   清零状态, GPIO置高(关闭风扇和水泵)
 */
void TempControl_Init(TempController_t *ctrl);

/**
 * @brief  使能温度控制器(预留接口)
 * @param  ctrl: 温度控制器指针
 */
void TempControl_Enable(TempController_t *ctrl);

/**
 * @brief  禁用温度控制器
 * @param  ctrl: 温度控制器指针
 * @param  pid:  冷却PID指针, 可为NULL; 非NULL时重置PID输出
 * @note   关闭风扇和水泵, 并重置PID计算
 */
void TempControl_Disable(TempController_t *ctrl, CoolingPID_t *pid);

/**
 * @brief  复位温度控制器
 * @param  ctrl: 温度控制器指针
 * @param  pid:  冷却PID指针, 可为NULL; 非NULL时重置PID输出
 * @note   状态清零, 关闭风扇和水泵, 并重置PID计算
 */
void TempControl_Reset(TempController_t *ctrl, CoolingPID_t *pid);

/**
 * @brief  设置目标温度(预留接口)
 * @param  ctrl: 温度控制器指针
 * @param  temp: 目标温度(°C)
 */
void TempControl_SetTargetTemp(TempController_t *ctrl, float temp);

/**
 * @brief  温度控制计算, 根据当前温度决定风扇和水泵开关
 * @param  ctrl:           温度控制器指针
 * @param  current_temp:   当前温度(°C)
 * @param  wpump1_allowed: 水泵允许标志(液位检测通过为1)
 * @param  ser2_valid:     SER_IN_2有效标志(制冷允许为1)
 * @param  pid:            冷却PID指针, 可为NULL; 非NULL时执行PID计算
 * @note   滞回控制: >12°C开启, <4°C关闭, 中间保持; 水泵需wpump1_allowed=1
 */
void TempControl_Compute(TempController_t *ctrl, float current_temp,
                          uint8_t wpump1_allowed, uint8_t ser2_valid,
                          CoolingPID_t *pid);

/**
 * @brief  获取冷却PID输出占空比
 * @param  ctrl: 温度控制器指针(当前未使用)
 * @param  pid:  冷却PID指针, 为NULL时返回0
 * @return PID输出占空比(0.0~100.0)
 */
float TempControl_GetOutput(TempController_t *ctrl, CoolingPID_t *pid);

#ifdef __cplusplus
}
#endif

#endif
