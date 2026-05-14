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

void FluidInterlock_Init(void);

void FluidInterlock_Tmr4Tick(void);

void FluidInterlock_MainLoopProcess(void);

uint8_t FluidInterlock_IsWpump1Allowed(void);

uint8_t FluidInterlock_IsSer2Valid(void);

uint8_t FluidInterlock_GetLiquidOk(void);

uint8_t FluidInterlock_GetNoLiquidFault(void);

uint16_t FluidInterlock_GetEdgesPerMin(void);

uint8_t FluidInterlock_GetSer2LowLevel(void);

uint8_t FluidInterlock_CoolingAllowed(void);

#ifdef __cplusplus
}
#endif

#endif
