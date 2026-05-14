/**
  ******************************************************************************
  * @file    sensor_input.h
  * @brief   传感器输入驱动头文件
  * @version V1.0.0
  * @date    2026-03-31
  * @note    提供多路传感器输入检测和消抖处理功能
  *          消抖时间: 50ms, 在中断中调用SensorInput_Scan进行扫描
  ******************************************************************************
  */

#ifndef __SENSOR_INPUT_H
#define __SENSOR_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define SENSOR_INPUT_NUM        3  /**< 传感器输入通道数 */

/**
 * @defgroup 传感器编号定义
 */
#define SENSOR_IN_1             0  /**< SER_IN_1 - 液位检测 (低电平有效/有液) */
#define SENSOR_IN_2             1  /**< SER_IN_2 - 流量/联锁, 低电平有效(与 fluid_flow_interlock 一致) */
#define SENSOR_IN_3             2  /**< SER_IN_3 - 预留 */

#define DEBOUNCE_TIME_MS        50  /**< 消抖时间 (单位: 扫描周期数, 对应50ms) */

/**
  * @brief  单个传感器状态结构体
  */
typedef struct {
    uint8_t current_state;    /**< 消抖后的当前状态: 0=低电平, 1=高电平 */
    uint8_t raw_state;        /**< 原始GPIO状态 (未消抖) */
    uint8_t debounce_count;   /**< 状态变化计数, 用于消抖判定 */
    uint8_t stable_count;     /**< 状态稳定计数 */
} sensor_status_t;

/**
  * @brief  所有传感器状态集合
  */
typedef struct {
    sensor_status_t sensors[SENSOR_INPUT_NUM];  /**< 各传感器状态数组 */
} sensor_input_status_t;

extern sensor_input_status_t g_sensor_input_status;

/**
  * @brief  传感器模块初始化
  * @note   清零所有传感器状态和计数器
  */
void SensorInput_Init(void);

/**
  * @brief  传感器扫描处理函数
  * @note   需在TMR4中断中周期性调用, 执行消抖处理并触发联锁检测
  */
void SensorInput_Scan(void);

/**
  * @brief  获取指定传感器的消抖后状态
  * @param  sensor_id: 传感器编号 (0~SENSOR_INPUT_NUM-1)
  * @return 0=低电平, 1=高电平
  */
uint8_t SensorInput_GetState(uint8_t sensor_id);

/**
  * @brief  获取所有传感器状态结构体指针
  * @return 传感器状态结构体指针
  */
sensor_input_status_t* SensorInput_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif
