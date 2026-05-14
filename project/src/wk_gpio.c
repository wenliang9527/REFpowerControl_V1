/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_gpio.c
  * @brief    work bench config program
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

/* Includes ------------------------------------------------------------------*/
#include "wk_gpio.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init gpio_input/gpio_output/gpio_analog/eventout function.
  * @param  none
  * @retval none
  */
void wk_gpio_config(void)
{
  /* add user code begin gpio_config 0 */

  /* add user code end gpio_config 0 */

  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin gpio_config 1 */

  /* add user code end gpio_config 1 */

  /* gpio input config */
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = MCU_LIQUID_PIN | MCU_CLASH_PIN | TOUCH_1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOA, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = TZ_SER_3_PIN | TZ_SER_1_PIN | TZ_SER_2_PIN | TZ_SER_4_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOE, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = TEMP_2_PIN | TEMP_1_PIN | SER_IN_1_PIN | SER_IN_2_PIN | SER_IN_3_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOD, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = TOUCH_4_PIN | TOUCH_3_PIN | TOUCH_2_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOC, &gpio_init_struct);

  /* gpio output config */
  /* 电源关闭态: LED2 低、LED_POWER_C 高; LED3/4 默认熄灭(高); M_POWER_C 低 */
  gpio_bits_set(GPIOC, LED3_PIN | LED4_PIN);
  gpio_bits_reset(GPIOC, LED2_PIN);
  gpio_bits_reset(GPIOC, TZ_3_PLUS_PIN | TZ_3_DIR_PIN | SW_VALVE_2_PIN | SW_WPUMP_2_PIN | BEEP_PIN);
  gpio_bits_set(GPIOA, LED5_PIN);
  gpio_bits_reset(GPIOA, TZ_1_PLUS_PIN | TZ_1_DIR_PIN | TZ_2_PLUS_PIN | TZ_2_DIR_PIN | SW_FAN_2_PIN | SW_FAN_1_PIN | SW_WPUMP_1_PIN);
  gpio_bits_set(GPIOB, LED_POWER_C_PIN | SW_REF_PIN);
  gpio_bits_reset(GPIOB, TZ_4_PLUS_PIN | TZ_4_DIR_PIN | DTUBE_6_PIN | DTUBE_5_PIN | DTUBE_4_PIN | DTUBE_3_PIN);
  gpio_bits_set(GPIOE, LED1_PIN);
  gpio_bits_reset(GPIOE, MCU_SW1_PIN | MCU_SW2_PIN);
  gpio_bits_set(GPIOD, SW_VALVE_1_PIN);
  gpio_bits_reset(GPIOD, DTUBE_2_PIN | DTUBE_1_PIN);
  gpio_bits_reset(GPIOC, M_POWER_C_PIN);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = LED2_PIN | LED3_PIN | LED4_PIN | TZ_3_PLUS_PIN | TZ_3_DIR_PIN | 
                               SW_VALVE_2_PIN | SW_WPUMP_2_PIN | M_POWER_C_PIN | BEEP_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOC, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = LED5_PIN | TZ_1_PLUS_PIN | TZ_1_DIR_PIN | TZ_2_PLUS_PIN | TZ_2_DIR_PIN | 
                               SW_FAN_2_PIN | SW_FAN_1_PIN | SW_WPUMP_1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOA, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = TZ_4_PLUS_PIN | TZ_4_DIR_PIN | LED_POWER_C_PIN | SW_REF_PIN | DTUBE_6_PIN | 
                               DTUBE_5_PIN | DTUBE_4_PIN | DTUBE_3_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOB, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = MCU_SW1_PIN | MCU_SW2_PIN | LED1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOE, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = DTUBE_2_PIN | DTUBE_1_PIN | SW_VALVE_1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOD, &gpio_init_struct);

  /* add user code begin gpio_config 2 */

  /* add user code end gpio_config 2 */
}

/* add user code begin 1 */

/* add user code end 1 */
