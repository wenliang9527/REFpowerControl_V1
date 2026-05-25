/**
  ******************************************************************************
  * @file    digit_tube.h
  * @brief   3位6引脚数码管驱动头文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    V1适配: 引脚映射已更新
  *          DTUBE_1=PD9, DTUBE_2=PD8, DTUBE_3=PB15, DTUBE_4=PB14, DTUBE_5=PB13, DTUBE_6=PB12
  ******************************************************************************
  */

#ifndef __DIGIT_TUBE_H
#define __DIGIT_TUBE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define TUBE_PIN_1_GPIO         GPIOD
#define TUBE_PIN_1_PIN          GPIO_PINS_9
#define TUBE_PIN_1_CLOCK        CRM_GPIOD_PERIPH_CLOCK

#define TUBE_PIN_2_GPIO         GPIOD
#define TUBE_PIN_2_PIN          GPIO_PINS_8
#define TUBE_PIN_2_CLOCK        CRM_GPIOD_PERIPH_CLOCK

#define TUBE_PIN_3_GPIO         GPIOB
#define TUBE_PIN_3_PIN          GPIO_PINS_15
#define TUBE_PIN_3_CLOCK        CRM_GPIOB_PERIPH_CLOCK

#define TUBE_PIN_4_GPIO         GPIOB
#define TUBE_PIN_4_PIN          GPIO_PINS_14
#define TUBE_PIN_4_CLOCK        CRM_GPIOB_PERIPH_CLOCK

#define TUBE_PIN_5_GPIO         GPIOB
#define TUBE_PIN_5_PIN          GPIO_PINS_13
#define TUBE_PIN_5_CLOCK        CRM_GPIOB_PERIPH_CLOCK

#define TUBE_PIN_6_GPIO         GPIOB
#define TUBE_PIN_6_PIN          GPIO_PINS_12
#define TUBE_PIN_6_CLOCK        CRM_GPIOB_PERIPH_CLOCK

#define TUBE_SCAN_TIMER         TMR3
#define TUBE_SCAN_TIMER_CLOCK   CRM_TMR3_PERIPH_CLOCK
#define TUBE_SCAN_TIMER_IRQ     TMR3_GLOBAL_IRQn

#define TUBE_SCAN_PERIOD        100
#define TUBE_SCAN_DIV           240

#define TUBE_SCAN_IRQ_PREEMPT   0
#define TUBE_SCAN_IRQ_SUB       0

#define TUBE_DIGIT_NUM          3
#define TUBE_SCAN_FREQ          450

#define TEMP_UNIT_CELSIUS       0
#define TEMP_UNIT_FAHRENHEIT    1

#define SEG_A                   0x01
#define SEG_B                   0x02
#define SEG_C                   0x04
#define SEG_D                   0x08
#define SEG_E                   0x10
#define SEG_F                   0x20
#define SEG_G                   0x40

#define SEG_CHAR_E              0x79
#define SEG_CHAR_r              0x50
#define SEG_CHAR_C              (SEG_A | SEG_F | SEG_E | SEG_D)
#define SEG_CHAR_F              (SEG_A | SEG_F | SEG_E | SEG_G)
#define SEG_CHAR_MINUS          (SEG_G)
#define SEG_CHAR_BLANK          0x00

extern volatile uint32_t g_scan_counter;

void DigitTube_SyncToGlobal(void);
void DigitTube_SyncFromGlobal(void);
uint16_t DigitTube_GetValue(void);
void DigitTube_SetLEDState(uint8_t q1, uint8_t q2);
void DigitTube_SetDisplayDigits(uint8_t digits);
void DigitTube_Init(void);
void DigitTube_Scan(void);
uint8_t DigitTube_DisplayInt(int32_t value);
void DigitTube_SetValue(uint16_t value);
void DigitTube_Clear(void);
void DigitTube_ToggleUnit(void);
uint8_t DigitTube_GetUnit(void);
void DigitTube_SetLED(uint8_t led_id, uint8_t state);
uint8_t DigitTube_DisplayTemp(int32_t value, uint8_t unit);
void DigitTube_ClearError(void);
uint8_t DigitTube_IsError(void);
uint32_t DigitTube_GetScanCounter(void);
void DigitTube_ResetScanCounter(void);
void DigitTube_DisplayErrorCode(uint16_t code);
void DigitTube_ClearErrorCode(void);

#ifdef __cplusplus
}
#endif

#endif
