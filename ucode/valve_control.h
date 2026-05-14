/**
  ******************************************************************************
  * @file    valve_control.h
  * @brief   注水阀控制模块头文件
  * @version V1.0.0
  * @date    2026-05-08
  * @note    基于SER_IN_1信号控制注水阀SW_VALVE_1的开关
  *          上电初始化: 高电平持续1s开阀，低电平持续5s关阀
  *          系统运行: 高电平持续10s开阀，低电平持续5s关阀
  ******************************************************************************
  */

#ifndef __VALVE_CONTROL_H
#define __VALVE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

/** 定时器配置 - 与TMR4保持一致 */
#define VALVE_TMR4_DIV                 23u
#define VALVE_TMR4_PR                 9999u

/** 时间阈值配置 (单位: ms) */
#define VALVE_INIT_HIGH_TIME_MS       1000u
#define VALVE_OPEN_LOW_TIME_MS         5000u
#define VALVE_RUNNING_HIGH_TIME_MS    10000u

/** 备用定时周期 (fallback) */
#define VALVE_FALLBACK_TICKS_PER_SEC   1000u

/** 阀门GPIO定义 - 与 at32f403a_407_wk_config.h 保持一致 */
#define SW_VALVE_1_PIN                GPIO_PINS_15
#define SW_VALVE_1_GPIO_PORT          GPIOD
#define SER_IN_1_PIN                  GPIO_PINS_12
#define SER_IN_1_GPIO_PORT            GPIOD

/**
  * @brief  阀门状态枚举
  */
typedef enum {
    VALVE_STATE_INIT = 0,           /**< 初始化状态 */
    VALVE_STATE_INIT_HIGH_WAIT,     /**< 初始化阶段等待高电平 */
    VALVE_STATE_OPEN,               /**< 开阀状态 */
    VALVE_STATE_OPEN_LOW_WAIT,      /**< 开阀后等待低电平 */
    VALVE_STATE_RUNNING,            /**< 运行状态 */
    VALVE_STATE_RUNNING_HIGH_WAIT   /**< 运行阶段等待高电平 */
} valve_state_t;

/**
  * @brief  阀门状态结构体
  */
typedef struct {
    valve_state_t state;             /**< 当前状态机状态 */
    uint32_t high_streak_counter;   /**< 高电平连续计数 */
    uint32_t low_streak_counter;    /**< 低电平连续计数 */
    uint32_t ticks_per_sec;         /**< 每秒定时器tick数 */
    uint8_t valve_open;             /**< 阀门当前开关状态: 0=关闭, 1=开启 */
} valve_control_status_t;

/** 全局变量声明 */
extern valve_control_status_t g_valve_control_status;

/**
  * @brief  阀门控制模块初始化
  * @note   初始化阀门状态和计数器
  */
void ValveControl_Init(void);

/**
  * @brief  定时器TMR4 tick处理函数
  * @note   需在TMR4中断中周期性调用, 执行阀门状态机逻辑
  */
void ValveControl_Tmr4Tick(void);

/**
  * @brief  获取阀门当前状态
  * @return 阀门状态机当前状态
  */
valve_state_t ValveControl_GetState(void);

/**
  * @brief  查询阀门是否开启
  * @return 0=阀门关闭, 1=阀门开启
  */
uint8_t ValveControl_IsValveOpen(void);

/**
  * @brief  强制关闭阀门
  * @note   直接关闭阀门并切换到初始化状态
  */
void ValveControl_ForceClose(void);

/**
  * @brief  获取阀门状态结构体指针
  * @return 阀门状态结构体指针
  */
valve_control_status_t* ValveControl_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __VALVE_CONTROL_H */
