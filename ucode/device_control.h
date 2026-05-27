/**
  ******************************************************************************
  * @file    device_control.h
  * @brief   设备GPIO控制模块头文件
  * @version V1.0.0
  * @date    2026-05-26
  * @note    管理风扇、水泵、阀门、主电源等设备的GPIO开关控制
  *          GPIO有效电平说明:
  *          - 风扇/水泵/阀门: 高电平=关闭, 低电平=开启
  *          - M_POWER_C: 高电平=开启, 低电平=关闭
  ******************************************************************************
  */

#ifndef __DEVICE_CONTROL_H
#define __DEVICE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

/** @defgroup DEVICE_CODE 设备编号定义
  * @{ */
#define DEVICE_SW_FAN_1        0   /**< 风扇1 */
#define DEVICE_SW_FAN_2        1   /**< 风扇2 */
#define DEVICE_SW_WPUMP_1      2   /**< 水泵1 */
#define DEVICE_M_POWER_C       3   /**< 主电源控制 */
#define DEVICE_SW_WPUMP_2      4   /**< 水泵2 */
#define DEVICE_SW_VALVE_1      5   /**< 阀门1 */
#define DEVICE_SW_VALVE_2      6   /**< 阀门2 */
/** @} */

#define DEVICE_NUM             7   /**< 设备总数 */

/** @defgroup DEVICE_STATE 设备状态定义
  * @{ */
#define DEVICE_STATE_LOW       0   /**< 低电平状态 */
#define DEVICE_STATE_HIGH      1   /**< 高电平状态 */
/** @} */

/**
  * @brief  设备控制模块初始化
  * @note   配置所有设备GPIO为推挽输出模式, 并设置初始电平:
  *         - M_POWER_C 初始化为低电平(关闭)
  *         - 其余设备初始化为高电平(关闭)
  */
void DeviceControl_Init(void);

/**
  * @brief  控制指定设备的开关状态
  * @param  device_code  设备编号, 取值为 DEVICE_SW_FAN_1 ~ DEVICE_SW_VALVE_2
  * @param  switch_state 开关状态, DEVICE_STATE_HIGH(高电平) 或 DEVICE_STATE_LOW(低电平)
  * @note   高电平=关闭(风扇/水泵/阀门), 低电平=开启(风扇/水泵/阀门);
  *         M_POWER_C相反: 高电平=开启, 低电平=关闭
  *         非法参数时函数直接返回, 不执行任何操作
  */
void control_device(uint8_t device_code, uint8_t switch_state);

#ifdef __cplusplus
}
#endif

#endif
