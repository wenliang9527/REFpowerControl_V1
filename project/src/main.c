/* add user code begin Header */
/**
  **************************************************************************
  * @file     main.c
  * @brief    main program
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
#include "at32f403a_407_wk_config.h"
#include "wk_debug.h"
#include "wk_tmr.h"
#include "wk_usart.h"
#include "wk_dma.h"
#include "wk_gpio.h"
#include "wk_system.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "pid_control.h"
#include "device_control.h"
#include "sensor_input.h"
#include "fluid_flow_interlock.h"
#include "touch_key.h"
#include "digit_tube.h"
#include "DS18B20.h"
#include "led_beep.h"
#include "valve_control.h"
/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */

/* add user code end private define */

/* private macro -------------------------------------------------------------*/
/* add user code begin private macro */

/* add user code end private macro */

/* private variables ---------------------------------------------------------*/
/* add user code begin private variables */
static PID_Controller_t pid_controller;
static DS18B20_MultiDeviceTypeDef *ds_list[DS_TempNumber];
static float current_temp = 0.0f;
/* add user code end private variables */

/* private function prototypes --------------------------------------------*/
/* add user code begin function prototypes */

/* add user code end function prototypes */

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief main function.
  * @param  none
  * @retval none
  */
int main(void)
{
  /* add user code begin 1 */

  /* add user code end 1 */

  /* system clock config. */
  wk_system_clock_config();

  /* config periph clock. */
  wk_periph_clock_config();

  /* init debug function. */
  wk_debug_config();

  /* nvic config. */
  wk_nvic_config();

  /* timebase config for
     void wk_delay_us(uint32_t delay);
     void wk_delay_ms(uint32_t delay); */
  wk_timebase_init();

  /* init gpio function. */
  wk_gpio_config();

  /* init dma1 channel1 */
  wk_dma1_channel1_init();
  /* config dma channel transfer parameter */
  /* user need to modify define values DMAx_CHANNELy_XXX_BASE_ADDR 
     and DMAx_CHANNELy_BUFFER_SIZE in at32xxx_wk_config.h */
  wk_dma_channel_config(DMA1_CHANNEL1, 
                        (uint32_t)&USART1->dt, 
                        DMA1_CHANNEL1_MEMORY_BASE_ADDR, 
                        DMA1_CHANNEL1_BUFFER_SIZE);
  dma_channel_enable(DMA1_CHANNEL1, TRUE);

  /* init dma1 channel2 */
  wk_dma1_channel2_init();
  /* config dma channel transfer parameter */
  /* user need to modify define values DMAx_CHANNELy_XXX_BASE_ADDR 
     and DMAx_CHANNELy_BUFFER_SIZE in at32xxx_wk_config.h */
  wk_dma_channel_config(DMA1_CHANNEL2, 
                        (uint32_t)&USART1->dt, 
                        DMA1_CHANNEL2_MEMORY_BASE_ADDR, 
                        DMA1_CHANNEL2_BUFFER_SIZE);
  dma_channel_enable(DMA1_CHANNEL2, TRUE);

  /* init usart1 function. */
  wk_usart1_init();

  /* init tmr1 function. */
  wk_tmr1_init();

  /* init tmr2 function. */
  wk_tmr2_init();

  /* init tmr3 function. */
  wk_tmr3_init();

  /* init tmr4 function. */
  wk_tmr4_init();

  /* add user code begin 2 */
  GlobalSensorData_Init();
  DeviceControl_Init();
  SensorInput_Init();
  FluidInterlock_Init();
  ValveControl_Init();  /* 注水阀控制初始化 */
  TouchKey_Init();
  /* 与上电 GPIO 一致: 电源关闭 — M_POWER_C 低、LED2 低、LED_POWER_C 高 */
  LED_PowerIndicator_Set(0);

  DigitTube_Init();
  PID_Init(&pid_controller);

  ds_list[DSB1] = &DSTemp[DSB1];
  ds_list[DSB2] = &DSTemp[DSB2];
  DS18B20_Init(ds_list);

  PID_Enable(&pid_controller);

  DigitTube_SetLEDState(1, 1);
  DigitTube_SetLEDState(2, 0);
  
  BEEP_Control(1000, 1);
  /* add user code end 2 */

  while(1)
  {
    /* add user code begin 3 */
    TouchKey_EventProcess();

    if (g_scan_counter >= 10000)
    {
      g_scan_counter = 0;

      FluidInterlock_MainLoopProcess();

      DS18B20_UpdateAllTemp(ds_list);
      current_temp = DS18B20_GetValidTemp();

      PID_Compute(&pid_controller, current_temp,
                  FluidInterlock_IsWpump1Allowed(),
                  FluidInterlock_IsSer2Valid());

      DigitTube_DisplayTemp(DS18B20_GetValidDisplayValue(), DigitTube_GetUnit());
    }
    /* add user code end 3 */
  }
}

  /* add user code begin 4 */

  /* add user code end 4 */
