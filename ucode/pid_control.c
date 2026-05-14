/**
  ******************************************************************************
  * @file    pid_control.c
  * @brief   制冷温度阈值控制模块实现文件
  * @version V5.1.0
  * @date    2026-04-27
  * @note    无 PID(P/I/D) 运算。温度 6℃/3℃ 迟滞控制风扇、水泵需求及制冷许可;
  *          水泵受 SER_IN_1 联锁, 制冷另需 SER_IN_2; SW_REF 低=开、高=关, 关断后重试周期再合闸
  ******************************************************************************
  */

#include "pid_control.h"
#include "at32f403a_407_wk_config.h"
#include <stddef.h>

void PID_Init(PID_Controller_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->current_temp = 0.0f;
    pid->fan1_on = 0;
    pid->wpump1_on = 0;
    pid->cooling_on = 0;
    pid->cooling_retry_counter = 0;

    gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

void PID_Enable(PID_Controller_t *pid)
{
    (void)pid;
}

void PID_Disable(PID_Controller_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->fan1_on = 0;
    pid->wpump1_on = 0;
    pid->cooling_on = 0;
    pid->cooling_retry_counter = 0;

    gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

void PID_Reset(PID_Controller_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->fan1_on = 0;
    pid->wpump1_on = 0;
    pid->cooling_on = 0;
    pid->cooling_retry_counter = 0;

    gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

void PID_SetTargetTemp(PID_Controller_t *pid, float temp)
{
    (void)pid;
    (void)temp;
}

void PID_Compute(PID_Controller_t *pid, float current_temp, uint8_t wpump1_allowed, uint8_t ser2_valid)
{
    uint8_t fan_wanted;
    uint8_t wpump_wanted;
    uint8_t temp_allows_cooling;

    if (pid == NULL) {
        return;
    }

    pid->current_temp = current_temp;

    if (current_temp > COOLING_TEMP_ON_THRESHOLD) {
        fan_wanted = 1;
        wpump_wanted = 1;
        temp_allows_cooling = 1;
    } else if (current_temp < COOLING_TEMP_OFF_THRESHOLD) {
        fan_wanted = 0;
        wpump_wanted = 0;
        temp_allows_cooling = 0;
    } else {
        fan_wanted = pid->fan1_on;
        wpump_wanted = pid->wpump1_on;
        temp_allows_cooling = pid->cooling_on;
    }

    if (fan_wanted) {
        if (!pid->fan1_on) {
            pid->fan1_on = 1;
            gpio_bits_reset(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
        }
    } else {
        if (pid->fan1_on) {
            pid->fan1_on = 0;
            gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
        }
    }

    if (wpump_wanted && wpump1_allowed) {
        if (!pid->wpump1_on) {
            pid->wpump1_on = 1;
            gpio_bits_reset(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
        }
    } else {
        if (pid->wpump1_on) {
            pid->wpump1_on = 0;
            gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
        }
    }

    if (temp_allows_cooling) {
        if (pid->wpump1_on && ser2_valid) {
            if (!pid->cooling_on) {
                pid->cooling_on = 1;
                pid->cooling_retry_counter = 0;
                gpio_bits_reset(SW_REF_GPIO_PORT, SW_REF_PIN);
            }
        } else {
            if (pid->cooling_on) {
                pid->cooling_on = 0;
                pid->cooling_retry_counter = COOLING_RETRY_CYCLES;
                gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
            } else {
                if (pid->cooling_retry_counter > 0) {
                    pid->cooling_retry_counter--;
                }
                if (pid->cooling_retry_counter == 0) {
                    if (pid->wpump1_on && ser2_valid) {
                        pid->cooling_on = 1;
                        gpio_bits_reset(SW_REF_GPIO_PORT, SW_REF_PIN);
                    }
                }
            }
        }
    } else {
        if (pid->cooling_on) {
            pid->cooling_on = 0;
            pid->cooling_retry_counter = 0;
            gpio_bits_set(SW_REF_GPIO_PORT, SW_REF_PIN);
        }
    }
}

float PID_GetOutput(PID_Controller_t *pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->cooling_on ? 1.0f : 0.0f;
}
