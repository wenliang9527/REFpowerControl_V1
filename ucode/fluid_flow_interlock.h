/**
  ******************************************************************************
  * @file    fluid_flow_interlock.h
  * @brief   SER_IN_1 液位检测与 SER_IN_2 电平检测
 * @note    SER_IN_1: 低电平有效, 低持续1s有液(允许开水泵), 高持续1s无液
 *          SER_IN_2: 低电平有效, 低持续1s有效(允许制冷), 高持续1s无效
  *          液位检测周期: 前5次每30秒检测一次, 之后每60秒检测一次
  ******************************************************************************
  */

#ifndef __FLUID_FLOW_INTERLOCK_H
#define __FLUID_FLOW_INTERLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

/**
 * @brief  初始化液位/流量联锁模块
 * @note   根据 TMR4 时钟参数计算每秒 tick 数, 并重置所有状态变量
 */
void FluidInterlock_Init(void);

/**
 * @brief  TMR4 中断处理函数 (1ms 周期调用)
 * @note   执行 SER_IN_1 液位检测、SER_IN_2 有效检测及无液报警状态机
 */
void FluidInterlock_Tmr4Tick(void);

/** @brief 主循环处理 (预留接口) */
void FluidInterlock_MainLoopProcess(void);

/**
 * @brief  查询水泵1是否允许运行
 * @return 1=允许(液位正常), 0=不允许
 */
uint8_t FluidInterlock_IsWpump1Allowed(void);

/**
 * @brief  查询 SER_IN_2 是否有效
 * @return 1=有效(低电平), 0=无效
 */
uint8_t FluidInterlock_IsSer2Valid(void);

/**
 * @brief  获取液位正常标志
 * @return 1=有液, 0=无液
 */
uint8_t FluidInterlock_GetLiquidOk(void);

/**
 * @brief  获取无液故障标志
 * @return 1=无液故障, 0=液位正常
 */
uint8_t FluidInterlock_GetNoLiquidFault(void);

/**
 * @brief  获取流量边沿数 (简化版)
 * @note   SER_IN_2 有效时返回 9999, 无效时返回 0
 * @return 边沿数/模拟值
 */
uint16_t FluidInterlock_GetEdgesPerMin(void);

/**
 * @brief  获取 SER_IN_2 低电平有效标志
 * @return 1=低电平有效, 0=无效
 */
uint8_t FluidInterlock_GetSer2LowLevel(void);

/**
 * @brief  查询制冷是否允许
 * @note   需同时满足液位正常且 SER_IN_2 有效
 * @return 1=允许制冷, 0=不允许
 */
uint8_t FluidInterlock_CoolingAllowed(void);

/**
 * @brief  查询报警是否激活
 * @return 1=报警中(ACTIVE 或 SILENT), 0=空闲
 */
uint8_t FluidInterlock_IsAlarmActive(void);

#ifdef __cplusplus
}
#endif

#endif
