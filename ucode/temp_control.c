/**
  ******************************************************************************
  * @file    temp_control.c
  * @brief   制冷温度阈值控制模块实现文件
  * @version V6.0.0
  * @date    2026-06-12
  * @note    温度 6℃/3℃ 迟滞控制风扇、水泵需求及制冷许可;
  *          水泵受 SER_IN_1 联锁, 制冷另需 SER_IN_2; SW_REF 低=开、高=关, 关断后重试周期再合闸
  ******************************************************************************
  */

#include "temp_control.h"
#include "at32f403a_407_wk_config.h"
#include <stddef.h>

void TempControl_Init(TempController_t *ctrl)
{
    if (ctrl == NULL) {
        return;
    }

    ctrl->current_temp = 0.0f;
    ctrl->fan1_on = 0;
    ctrl->wpump1_on = 0;
    ctrl->cooling_on = 0;
    ctrl->cooling_retry_counter = 0;

    gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

void TempControl_Enable(TempController_t *ctrl)
{
    (void)ctrl;
}

void TempControl_Disable(TempController_t *ctrl)
{
    if (ctrl == NULL) {
        return;
    }

    ctrl->fan1_on = 0;
    ctrl->wpump1_on = 0;
    ctrl->cooling_on = 0;
    ctrl->cooling_retry_counter = 0;

    gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

void TempControl_Reset(TempController_t *ctrl)
{
    if (ctrl == NULL) {
        return;
    }

    ctrl->fan1_on = 0;
    ctrl->wpump1_on = 0;
    ctrl->cooling_on = 0;
    ctrl->cooling_retry_counter = 0;

    gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

void TempControl_SetTargetTemp(TempController_t *ctrl, float temp)
{
    (void)ctrl;
    (void)temp;
}

void TempControl_Compute(TempController_t *ctrl, float current_temp, uint8_t wpump1_allowed, uint8_t ser2_valid)
{
    uint8_t fan_wanted;
    uint8_t wpump_wanted;
    uint8_t temp_allows_cooling;

    if (ctrl == NULL) {
        return;
    }

    ctrl->current_temp = current_temp;

    if (current_temp > COOLING_TEMP_ON_THRESHOLD) {
        fan_wanted = 1;
        wpump_wanted = 1;
        temp_allows_cooling = 1;
    } else if (current_temp < COOLING_TEMP_OFF_THRESHOLD) {
        fan_wanted = 0;
        wpump_wanted = 0;
        temp_allows_cooling = 0;
    } else {
        fan_wanted = ctrl->fan1_on;
        wpump_wanted = ctrl->wpump1_on;
        temp_allows_cooling = ctrl->cooling_on;
    }

    if (fan_wanted) {
        if (!ctrl->fan1_on) {
            ctrl->fan1_on = 1;
            gpio_bits_reset(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
        }
    } else {
        if (ctrl->fan1_on) {
            ctrl->fan1_on = 0;
            gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
        }
    }

    if (wpump_wanted && wpump1_allowed) {
        if (!ctrl->wpump1_on) {
            ctrl->wpump1_on = 1;
            gpio_bits_reset(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
        }
    } else {
        if (ctrl->wpump1_on) {
            ctrl->wpump1_on = 0;
            gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
        }
    }

    if (temp_allows_cooling) {
        if (ctrl->wpump1_on && ser2_valid) {
            if (!ctrl->cooling_on) {
                ctrl->cooling_on = 1;
                ctrl->cooling_retry_counter = 0;
                gpio_bits_reset(SW_REF_GPIO_PORT, SW_REF_PIN);
            }
        } else {
            if (ctrl->cooling_on) {
                ctrl->cooling_on = 0;
                ctrl->cooling_retry_counter = COOLING_RETRY_CYCLES;
                gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
            } else {
                if (ctrl->cooling_retry_counter > 0) {
                    ctrl->cooling_retry_counter--;
                }
                if (ctrl->cooling_retry_counter == 0) {
                    if (ctrl->wpump1_on && ser2_valid) {
                        ctrl->cooling_on = 1;
                        gpio_bits_reset(SW_REF_GPIO_PORT, SW_REF_PIN);
                    }
                }
            }
        }
    } else {
        if (ctrl->cooling_on) {
            ctrl->cooling_on = 0;
            ctrl->cooling_retry_counter = 0;
            gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
        }
    }
}

float TempControl_GetOutput(TempController_t *ctrl)
{
    if (ctrl == NULL) {
        return 0.0f;
    }
    return ctrl->cooling_on ? 1.0f : 0.0f;
}
