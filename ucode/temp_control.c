/**
  ******************************************************************************
  * @file    temp_control.c
  * @brief   温度控制模块实现 — 风扇与水泵的滞回开关控制
  * @note    控制逻辑:
  *          1. 温度 > 12°C: 开启风扇和水泵
  *          2. 温度 < 4°C:  关闭风扇和水泵
  *          3. 温度在4~12°C之间: 保持当前状态(滞回防抖)
  *          4. 水泵额外需 wpump1_allowed=1 才能开启
  *          5. GPIO高电平=关闭, 低电平=开启
  ******************************************************************************
  */

#include "temp_control.h"
#include "at32f403a_407_wk_config.h"
#include <stddef.h>

/**
 * @brief  初始化温度控制器
 * @param  ctrl: 温度控制器指针
 * @note   清零状态变量, GPIO置高关闭风扇和水泵
 */
void TempControl_Init(TempController_t *ctrl)
{
    if (ctrl == NULL) {
        return;
    }

    ctrl->current_temp = 0.0f;
    ctrl->fan1_on = 0;
    ctrl->wpump1_on = 0;

    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);
}

/**
 * @brief  使能温度控制器(预留接口)
 * @param  ctrl: 温度控制器指针
 */
void TempControl_Enable(TempController_t *ctrl)
{
    (void)ctrl;
}

/**
 * @brief  禁用温度控制器
 * @param  ctrl: 温度控制器指针
 * @param  pid:  冷却PID指针, 可为NULL; 非NULL时传入0值重置PID
 * @note   关闭风扇和水泵(GPIO置高), 并以0参数调用PID计算以重置输出
 */
void TempControl_Disable(TempController_t *ctrl, CoolingPID_t *pid)
{
    if (ctrl == NULL) {
        return;
    }

    ctrl->fan1_on = 0;
    ctrl->wpump1_on = 0;

    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);

    if (pid != NULL) {
        CoolingPID_Compute(pid, 0.0f, 0, 0);
    }
}

/**
 * @brief  复位温度控制器
 * @param  ctrl: 温度控制器指针
 * @param  pid:  冷却PID指针, 可为NULL; 非NULL时传入0值重置PID
 * @note   状态清零, 关闭风扇和水泵(GPIO置高), 并重置PID计算
 */
void TempControl_Reset(TempController_t *ctrl, CoolingPID_t *pid)
{
    if (ctrl == NULL) {
        return;
    }

    ctrl->fan1_on = 0;
    ctrl->wpump1_on = 0;

    gpio_bits_set(SW_FAN_1_GPIO_PORT, SW_FAN_1_PIN);
    gpio_bits_set(SW_WPUMP_1_GPIO_PORT, SW_WPUMP_1_PIN);

    if (pid != NULL) {
        CoolingPID_Compute(pid, 0.0f, 0, 0);
    }
}

/**
 * @brief  设置目标温度(预留接口)
 * @param  ctrl: 温度控制器指针
 * @param  temp: 目标温度(°C)
 */
void TempControl_SetTargetTemp(TempController_t *ctrl, float temp)
{
    (void)ctrl;
    (void)temp;
}

/**
 * @brief  温度控制核心计算, 根据当前温度决定风扇和水泵开关
 * @param  ctrl:           温度控制器指针
 * @param  current_temp:   当前温度(°C)
 * @param  wpump1_allowed: 水泵允许标志, 液位检测通过时为1
 * @param  ser2_valid:     SER_IN_2有效标志, 制冷允许时为1
 * @param  pid:            冷却PID指针, 可为NULL; 非NULL时执行PID计算
 * @note   滞回控制策略:
 *         - current_temp > 12°C: fan_wanted=1, wpump_wanted=1
 *         - current_temp < 4°C:  fan_wanted=0, wpump_wanted=0
 *         - 4°C <= current_temp <= 12°C: 保持当前状态
 *         水泵实际开启还需 wpump1_allowed=1
 *         GPIO: 置高=关闭, 置低=开启
 */
void TempControl_Compute(TempController_t *ctrl, float current_temp,
                          uint8_t wpump1_allowed, uint8_t ser2_valid,
                          CoolingPID_t *pid)
{
    uint8_t fan_wanted;
    uint8_t wpump_wanted;

    if (ctrl == NULL) {
        return;
    }

    ctrl->current_temp = current_temp;

    if (current_temp > FAN_TEMP_ON_THRESHOLD) {
        fan_wanted = 1;
        wpump_wanted = 1;
    } else if (current_temp < FAN_TEMP_OFF_THRESHOLD) {
        fan_wanted = 0;
        wpump_wanted = 0;
    } else {
        fan_wanted = ctrl->fan1_on;
        wpump_wanted = ctrl->wpump1_on;
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

    if (pid != NULL) {
        CoolingPID_Compute(pid, current_temp, ctrl->fan1_on && wpump1_allowed, ctrl->wpump1_on && ser2_valid);
    }
}

/**
 * @brief  获取冷却PID输出占空比
 * @param  ctrl: 温度控制器指针(当前未使用)
 * @param  pid:  冷却PID指针, 为NULL时返回0
 * @return PID输出占空比(0.0~100.0), pid为NULL时返回0.0
 */
float TempControl_GetOutput(TempController_t *ctrl, CoolingPID_t *pid)
{
    (void)ctrl;

    if (pid == NULL) {
        return 0.0f;
    }
    return (float)CoolingPID_GetDuty(pid);
}
