/* add user code begin Header */
/**
  **************************************************************************
  * @file     at32f403a_407_wk_config.h
  * @brief    header file of work bench config
  **************************************************************************
  * Copyright (c) 2025, Artery Technology, All rights reserved.
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */
/* add user code end Header */

/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __AT32F403A_407_WK_CONFIG_H
#define __AT32F403A_407_WK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "stdio.h"
#include "at32f403a_407.h"
/* private includes -------------------------------------------------------------*/
/* add user code begin private includes */

/* add user code end private includes */

/* exported types -------------------------------------------------------------*/
/* add user code begin exported types */

/* add user code end exported types */

/* exported constants --------------------------------------------------------*/
/* add user code begin exported constants */

/* add user code end exported constants */

/* exported macro ------------------------------------------------------------*/
/* add user code begin exported macro */

/* add user code end exported macro */

/* add user code begin dma define */
/* user can only modify the dma define value */
#define DMA1_CHANNEL1_BUFFER_SIZE   0
#define DMA1_CHANNEL1_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL1_PERIPHERAL_BASE_ADDR  0

#define DMA1_CHANNEL2_BUFFER_SIZE   0
#define DMA1_CHANNEL2_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL2_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL3_BUFFER_SIZE   0
//#define DMA1_CHANNEL3_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL3_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL4_BUFFER_SIZE   0
//#define DMA1_CHANNEL4_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL4_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL5_BUFFER_SIZE   0
//#define DMA1_CHANNEL5_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL5_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL6_BUFFER_SIZE   0
//#define DMA1_CHANNEL6_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL6_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL7_BUFFER_SIZE   0
//#define DMA1_CHANNEL7_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL7_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL1_BUFFER_SIZE   0
//#define DMA2_CHANNEL1_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL1_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL2_BUFFER_SIZE   0
//#define DMA2_CHANNEL2_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL2_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL3_BUFFER_SIZE   0
//#define DMA2_CHANNEL3_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL3_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL4_BUFFER_SIZE   0
//#define DMA2_CHANNEL4_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL4_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL5_BUFFER_SIZE   0
//#define DMA2_CHANNEL5_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL5_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL6_BUFFER_SIZE   0
//#define DMA2_CHANNEL6_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL6_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL7_BUFFER_SIZE   0
//#define DMA2_CHANNEL7_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL7_PERIPHERAL_BASE_ADDR   0
/* add user code end dma define */

/* Private defines -------------------------------------------------------------*/
#define LED2_PIN    GPIO_PINS_13
#define LED2_GPIO_PORT    GPIOC
#define LED3_PIN    GPIO_PINS_14
#define LED3_GPIO_PORT    GPIOC
#define LED4_PIN    GPIO_PINS_15
#define LED4_GPIO_PORT    GPIOC
#define LED5_PIN    GPIO_PINS_0
#define LED5_GPIO_PORT    GPIOA
#define MCU_LIQUID_PIN    GPIO_PINS_2
#define MCU_LIQUID_GPIO_PORT    GPIOA
#define MCU_CLASH_PIN    GPIO_PINS_3
#define MCU_CLASH_GPIO_PORT    GPIOA
#define TZ_1_PLUS_PIN    GPIO_PINS_4
#define TZ_1_PLUS_GPIO_PORT    GPIOA
#define TZ_1_DIR_PIN    GPIO_PINS_5
#define TZ_1_DIR_GPIO_PORT    GPIOA
#define TZ_2_PLUS_PIN    GPIO_PINS_6
#define TZ_2_PLUS_GPIO_PORT    GPIOA
#define TZ_2_DIR_PIN    GPIO_PINS_7
#define TZ_2_DIR_GPIO_PORT    GPIOA
#define TZ_3_PLUS_PIN    GPIO_PINS_4
#define TZ_3_PLUS_GPIO_PORT    GPIOC
#define TZ_3_DIR_PIN    GPIO_PINS_5
#define TZ_3_DIR_GPIO_PORT    GPIOC
#define TZ_4_PLUS_PIN    GPIO_PINS_0
#define TZ_4_PLUS_GPIO_PORT    GPIOB
#define TZ_4_DIR_PIN    GPIO_PINS_1
#define TZ_4_DIR_GPIO_PORT    GPIOB
#define TZ_SER_3_PIN    GPIO_PINS_7
#define TZ_SER_3_GPIO_PORT    GPIOE
#define TZ_SER_1_PIN    GPIO_PINS_8
#define TZ_SER_1_GPIO_PORT    GPIOE
#define TZ_SER_2_PIN    GPIO_PINS_9
#define TZ_SER_2_GPIO_PORT    GPIOE
#define TZ_SER_4_PIN    GPIO_PINS_10
#define TZ_SER_4_GPIO_PORT    GPIOE
#define MCU_SW1_PIN    GPIO_PINS_11
#define MCU_SW1_GPIO_PORT    GPIOE
#define MCU_SW2_PIN    GPIO_PINS_12
#define MCU_SW2_GPIO_PORT    GPIOE
#define LED1_PIN    GPIO_PINS_14
#define LED1_GPIO_PORT    GPIOE
#define LED_POWER_C_PIN    GPIO_PINS_10
#define LED_POWER_C_GPIO_PORT    GPIOB
#define SW_REF_PIN    GPIO_PINS_11
#define SW_REF_GPIO_PORT    GPIOB
#define DTUBE_6_PIN    GPIO_PINS_12
#define DTUBE_6_GPIO_PORT    GPIOB
#define DTUBE_5_PIN    GPIO_PINS_13
#define DTUBE_5_GPIO_PORT    GPIOB
#define DTUBE_4_PIN    GPIO_PINS_14
#define DTUBE_4_GPIO_PORT    GPIOB
#define DTUBE_3_PIN    GPIO_PINS_15
#define DTUBE_3_GPIO_PORT    GPIOB
#define DTUBE_2_PIN    GPIO_PINS_8
#define DTUBE_2_GPIO_PORT    GPIOD
#define DTUBE_1_PIN    GPIO_PINS_9
#define DTUBE_1_GPIO_PORT    GPIOD
#define TEMP_2_PIN    GPIO_PINS_10
#define TEMP_2_GPIO_PORT    GPIOD
#define TEMP_1_PIN    GPIO_PINS_11
#define TEMP_1_GPIO_PORT    GPIOD
#define DS18B20_1_PIN    GPIO_PINS_11
#define DS18B20_1_GPIO_PORT    GPIOD
#define DS18B20_2_PIN    GPIO_PINS_10
#define DS18B20_2_GPIO_PORT    GPIOD
#define SER_IN_1_PIN    GPIO_PINS_12
#define SER_IN_1_GPIO_PORT    GPIOD
#define SER_IN_2_PIN    GPIO_PINS_13
#define SER_IN_2_GPIO_PORT    GPIOD
#define SER_IN_3_PIN    GPIO_PINS_14
#define SER_IN_3_GPIO_PORT    GPIOD
#define SW_VALVE_1_PIN    GPIO_PINS_15
#define SW_VALVE_1_GPIO_PORT    GPIOD
#define SW_VALVE_2_PIN    GPIO_PINS_6
#define SW_VALVE_2_GPIO_PORT    GPIOC
#define TOUCH_4_PIN    GPIO_PINS_7
#define TOUCH_4_GPIO_PORT    GPIOC
#define TOUCH_3_PIN    GPIO_PINS_8
#define TOUCH_3_GPIO_PORT    GPIOC
#define TOUCH_2_PIN    GPIO_PINS_9
#define TOUCH_2_GPIO_PORT    GPIOC
#define TOUCH_1_PIN    GPIO_PINS_8
#define TOUCH_1_GPIO_PORT    GPIOA
#define SW_FAN_2_PIN    GPIO_PINS_11
#define SW_FAN_2_GPIO_PORT    GPIOA
#define SW_FAN_1_PIN    GPIO_PINS_12
#define SW_FAN_1_GPIO_PORT    GPIOA
#define SW_WPUMP_1_PIN    GPIO_PINS_15
#define SW_WPUMP_1_GPIO_PORT    GPIOA
#define SW_WPUMP_2_PIN    GPIO_PINS_10
#define SW_WPUMP_2_GPIO_PORT    GPIOC
#define M_POWER_C_PIN    GPIO_PINS_11
#define M_POWER_C_GPIO_PORT    GPIOC
#define BEEP_PIN    GPIO_PINS_12
#define BEEP_GPIO_PORT    GPIOC

/* exported functions ------------------------------------------------------- */
  /* system clock config. */
  void wk_system_clock_config(void);

  /* config periph clock. */
  void wk_periph_clock_config(void);

  /* nvic config. */
  void wk_nvic_config(void);

/* add user code begin exported functions */

/* add user code end exported functions */

#ifdef __cplusplus
}
#endif

#endif
