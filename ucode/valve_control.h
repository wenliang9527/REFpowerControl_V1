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

/** @brief 初始化阶段高电平持续时间(1000ms) */
#define VALVE_INIT_HIGH_TIME_MS       1000u
/** @brief 开阀后低电平持续时间(5000ms) */
#define VALVE_OPEN_LOW_TIME_MS         5000u
/** @brief 运行阶段高电平持续时间(10000ms) */
#define VALVE_RUNNING_HIGH_TIME_MS    10000u
/** @brief 消抖时间(50ms) */
#define VALVE_DEBOUNCE_MS               50u

/** @brief 阀门控制引脚 */
#define SW_VALVE_1_PIN                GPIO_PINS_15
/** @brief 阀门控制GPIO端口 */
#define SW_VALVE_1_GPIO_PORT          GPIOD
/** @brief 液位检测引脚 */
#define SER_IN_1_PIN                  GPIO_PINS_12
/** @brief 液位检测GPIO端口 */
#define SER_IN_1_GPIO_PORT            GPIOD

/**
 * @brief 阀门状态枚举
 */
typedef enum {
    VALVE_STATE_INIT = 0,           /**< 初始化, 等待高电平 */
    VALVE_STATE_INIT_HIGH_WAIT,     /**< 初始化高电平等待, 持续1s后开阀 */
    VALVE_STATE_OPEN,               /**< 阀门已打开, 转入低电平等待 */
    VALVE_STATE_OPEN_LOW_WAIT,      /**< 开阀后低电平等待, 持续5s后关阀 */
    VALVE_STATE_RUNNING,            /**< 运行态, 等待高电平 */
    VALVE_STATE_RUNNING_HIGH_WAIT   /**< 运行高电平等待, 持续10s后开阀 */
} valve_state_t;

/**
 * @brief 阀门控制状态结构体
 */
typedef struct {
    valve_state_t state;                /**< 当前阀门状态 */
    uint32_t high_streak_counter;       /**< 高电平持续计数(ms) */
    uint32_t low_streak_counter;        /**< 低电平持续计数(ms) */
    uint8_t valve_open;                 /**< 阀门打开标志: 1-打开, 0-关闭 */
} valve_control_status_t;

/** @brief 阀门控制状态全局变量 */
extern valve_control_status_t g_valve_control_status;

/**
 * @brief  初始化阀门控制模块
 * @note   状态置为INIT, 计数器清零, 阀门关闭
 */
void ValveControl_Init(void);

/**
 * @brief  阀门控制状态机处理
 * @note   在TMR4中断中周期性调用(1ms/tick)
 */
void ValveControl_Process(void);

/**
 * @brief  获取当前阀门状态
 * @return 当前阀门状态枚举值
 */
valve_state_t ValveControl_GetState(void);

/**
 * @brief  查询阀门是否打开
 * @return 1-阀门已打开, 0-阀门已关闭
 */
uint8_t ValveControl_IsValveOpen(void);

/**
 * @brief  强制关闭阀门
 * @note   关中断保护, 重置状态为INIT并关阀
 */
void ValveControl_ForceClose(void);

/**
 * @brief  获取阀门控制状态结构体指针
 * @return 阀门控制状态结构体指针
 */
valve_control_status_t* ValveControl_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif
