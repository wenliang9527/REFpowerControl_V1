/**
  ******************************************************************************
  * @file    cooling_pid.h
  * @brief   冷却PID控制模块头文件
  * @version V1.0.0
  * @date    2026-05-26
  * @note    基于PID算法控制制冷PWM占空比, 实现温度闭环调节
  *          采用迟滞启停策略: 温度超过ON阈值启动, 低于OFF阈值停止
  *          PWM输出通过TMR2通道4驱动, 占空比范围0~100%
  ******************************************************************************
  */

#ifndef __COOLING_PID_H
#define __COOLING_PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define COOLING_PID_TARGET_TEMP     8.0f   /**< PID目标温度 (单位: °C) */
#define COOLING_PID_ON_THRESHOLD    12.0f  /**< PID启动阈值温度, 当前温度超过此值时PID激活 (单位: °C) */
#define COOLING_PID_OFF_THRESHOLD   4.0f   /**< PID停止阈值温度, 当前温度低于此值时PID关闭 (单位: °C) */
#define COOLING_PID_MIN_DUTY        20     /**< 最小有效占空比, 低于此值时输出0%避免低效运行 (单位: %) */
#define COOLING_PID_MAX_DUTY        100    /**< 最大占空比上限 (单位: %) */
#define COOLING_PID_INTEGRAL_MAX    200.0f  /**< 积分项上限, 防止积分饱和 (单位: °C·周期) */

#define COOLING_PID_DEFAULT_KP      10.0f  /**< 默认比例系数 */
#define COOLING_PID_DEFAULT_KI      0.5f   /**< 默认积分系数 */
#define COOLING_PID_DEFAULT_KD      2.0f   /**< 默认微分系数 */

/**
  * @brief  冷却PID控制结构体
  */
typedef struct {
    float kp;              /**< 比例系数 */
    float ki;              /**< 积分系数 */
    float kd;              /**< 微分系数 */
    float target;          /**< 目标温度 (单位: °C) */
    float integral;        /**< 积分累计值 */
    float prev_error;      /**< 上一次误差值, 用于微分计算 */
    float output;          /**< PID计算输出值 (对应占空比, 0~100) */
    uint8_t pid_active;    /**< PID激活标志: 1=运行中, 0=已停止 */
    uint8_t duty_percent;  /**< 当前PWM占空比 (单位: %) */
} CoolingPID_t;

/**
  * @brief  冷却PID模块初始化
  * @param  pid: PID控制结构体指针
  * @note   使用默认参数初始化, 配置TMR2通道4为PWM模式并关闭输出
  */
void CoolingPID_Init(CoolingPID_t *pid);

/**
  * @brief  设置PID参数
  * @param  pid: PID控制结构体指针
  * @param  kp: 比例系数
  * @param  ki: 积分系数
  * @param  kd: 微分系数
  * @note   修改参数后会清零积分项和上次误差, 避免历史数据干扰
  */
void CoolingPID_SetParams(CoolingPID_t *pid, float kp, float ki, float kd);

/**
  * @brief  设置PID目标温度
  * @param  pid: PID控制结构体指针
  * @param  target: 目标温度 (单位: °C)
  * @note   修改目标后会清零积分项和上次误差
  */
void CoolingPID_SetTarget(CoolingPID_t *pid, float target);

/**
  * @brief  执行PID计算并更新PWM输出
  * @param  pid: PID控制结构体指针
  * @param  current_temp: 当前温度 (单位: °C)
  * @param  wpump_allowed: 水泵允许运行标志, 0=禁止, 1=允许
  * @param  ser2_valid: 传感器2有效标志, 0=无效, 1=有效
  * @note   当wpump_allowed或ser2_valid为0时强制关闭输出;
  *          温度超过ON阈值启动PID, 低于OFF阈值停止PID;
  *          积分项限制在[0, INTEGRAL_MAX]范围内防止积分饱和;
  *          输出低于MIN_DUTY时输出0%避免低效运行
  */
void CoolingPID_Compute(CoolingPID_t *pid, float current_temp,
                         uint8_t wpump_allowed, uint8_t ser2_valid);

/**
  * @brief  获取PID激活状态
  * @param  pid: PID控制结构体指针
  * @return 1=PID运行中, 0=PID已停止
  */
uint8_t CoolingPID_IsActive(CoolingPID_t *pid);

/**
  * @brief  获取当前PWM占空比
  * @param  pid: PID控制结构体指针
  * @return 当前占空比 (单位: %)
  */
uint8_t CoolingPID_GetDuty(CoolingPID_t *pid);

#ifdef __cplusplus
}
#endif

#endif
