/**
  ******************************************************************************
  * @file    led_beep.h
  * @brief   LED和蜂鸣器驱动头文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    V1适配: LED从1个扩展为5个
  *          LED控制: 低电平点亮, 高电平熄灭
  *          蜂鸣器: 高电平鸣响, 低电平关闭
  ******************************************************************************
  */

#ifndef __LED_BEEP_H
#define __LED_BEEP_H

#include "at32f403a_407_wk_config.h"
#include "wk_system.h"
#include <stdbool.h>

/** LED常亮时间阈值 (用于其他逻辑) */
#define  LEDONouttime       4000
/** 蜂鸣器响铃时间阈值 */
#define  BEEPONouttime      500
/** LED1闪烁周期 (扫描周期数) */
#define  LED1_FLASH_TIME    4000

/**
 * @defgroup LED翻转宏
 * @note 使用gpio_bits_toggle切换引脚电平
 */
#define LED_1_Reversal  gpio_bits_toggle(LED1_GPIO_PORT, LED1_PIN)
#define LED_2_Reversal  gpio_bits_toggle(LED2_GPIO_PORT, LED2_PIN)
#define LED_3_Reversal  gpio_bits_toggle(LED3_GPIO_PORT, LED3_PIN)
#define LED_4_Reversal  gpio_bits_toggle(LED4_GPIO_PORT, LED4_PIN)
#define LED_5_Reversal  gpio_bits_toggle(LED5_GPIO_PORT, LED5_PIN)

/** 蜂鸣器电平控制宏 */
#define BEEP_H    gpio_bits_set(BEEP_GPIO_PORT, BEEP_PIN)   /**< 蜂鸣器鸣响 */
#define BEEP_L    gpio_bits_reset(BEEP_GPIO_PORT, BEEP_PIN) /**< 蜂鸣器关闭 */

extern bool LED_LH;
extern u8 beep_flag;

/**
  * @brief  LED闪烁线程处理函数
  * @note   在定时器中断中调用, 处理LED1周期闪烁和蜂鸣器控制
  */
void LED_Thread(void);

/**
  * @brief  蜂鸣器开启
  */
void BEEP_On(void);

/**
  * @brief  蜂鸣器关闭
  */
void BEEP_Off(void);

/**
  * @brief  控制蜂鸣器鸣响次数和时间
  * @param  on_time_ms: 每次鸣响时间 (毫秒)
  * @param  count: 鸣响次数
  */
void BEEP_Control(uint16_t on_time_ms, uint8_t count);

/**
  * @brief  蜂鸣器处理函数
  * @note   在LED_Thread中调用, 实现周期性鸣响状态机
  */
void BEEP_Process(void);

/**
  * @brief  设置指定LED的状态
  * @param  led_id: LED编号 (1~5)
  * @param  state: 状态 (1=点亮, 0=熄灭)
  * @note   GPIO低电平有效: state=1输出低电平点亮, state=0输出高电平熄灭
  */
void LED_SetState(uint8_t led_id, uint8_t state);

/**
  * @brief  翻转指定LED的状态
  * @param  led_id: LED编号 (1~5)
  */
void LED_Toggle(uint8_t led_id);

/**
  * @brief  设置电源指示灯状态
  * @param  power_on: 1=电源打开 (LED_POWER_C 低, LED2 高), 0=关闭 (LED_POWER_C 高, LED2 低)
  * @note   与 M_POWER_C 开/关由 touch_key 在同一动作中调用 control_device 配合
  */
void LED_PowerIndicator_Set(uint8_t power_on);

void LED_PowerIndicator_BlinkStart(void);
void LED_PowerIndicator_BlinkStop(void);
uint8_t LED_PowerIndicator_GetState(void);

#endif
