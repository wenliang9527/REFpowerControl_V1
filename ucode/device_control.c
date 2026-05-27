/**
  ******************************************************************************
  * @file    device_control.c
  * @brief   设备GPIO控制模块实现文件
  * @version V1.0.0
  * @date    2026-05-26
  * @note    功能说明:
  *          1. 管理风扇、水泵、阀门、主电源等设备的GPIO输出控制
  *          2. GPIO有效电平说明:
  *             - 风扇/水泵/阀门: 高电平=关闭, 低电平=开启
  *             - M_POWER_C: 高电平=开启, 低电平=关闭
  ******************************************************************************
  */

#include "device_control.h"
#include "at32f403a_407_wk_config.h"

/** @brief 设备GPIO端口映射表, 索引对应 DEVICE_SW_FAN_1 ~ DEVICE_SW_VALVE_2 */
static const gpio_type* device_gpio_port[DEVICE_NUM] = {
    SW_FAN_1_GPIO_PORT,
    SW_FAN_2_GPIO_PORT,
    SW_WPUMP_1_GPIO_PORT,
    M_POWER_C_GPIO_PORT,
    SW_WPUMP_2_GPIO_PORT,
    SW_VALVE_1_GPIO_PORT,
    SW_VALVE_2_GPIO_PORT
};

/** @brief 设备GPIO引脚映射表, 索引与 device_gpio_port 一一对应 */
static const uint16_t device_gpio_pin[DEVICE_NUM] = {
    SW_FAN_1_PIN,
    SW_FAN_2_PIN,
    SW_WPUMP_1_PIN,
    M_POWER_C_PIN,
    SW_WPUMP_2_PIN,
    SW_VALVE_1_PIN,
    SW_VALVE_2_PIN
};

/** @brief 设备GPIO外设时钟映射表, 索引与 device_gpio_port 一一对应 */
static const crm_periph_clock_type device_gpio_clock[DEVICE_NUM] = {
    CRM_GPIOA_PERIPH_CLOCK,
    CRM_GPIOA_PERIPH_CLOCK,
    CRM_GPIOA_PERIPH_CLOCK,
    CRM_GPIOC_PERIPH_CLOCK,
    CRM_GPIOC_PERIPH_CLOCK,
    CRM_GPIOD_PERIPH_CLOCK,
    CRM_GPIOC_PERIPH_CLOCK
};

/**
  * @brief  设备控制模块初始化
  * @note   配置所有设备GPIO为推挽输出模式, 并设置初始电平:
  *         - M_POWER_C 初始化为低电平(关闭)
  *         - 其余设备初始化为高电平(关闭)
  */
void DeviceControl_Init(void)
{
    gpio_init_type gpio_init_struct;
    uint8_t i;

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

    for (i = 0; i < DEVICE_NUM; i++) {
        crm_periph_clock_enable(device_gpio_clock[i], TRUE);

        gpio_init_struct.gpio_pins = device_gpio_pin[i];
        gpio_init((gpio_type*)device_gpio_port[i], &gpio_init_struct);

        if (i == DEVICE_M_POWER_C) {
            gpio_bits_reset((gpio_type*)device_gpio_port[i], device_gpio_pin[i]);
        } else {
            gpio_bits_set((gpio_type*)device_gpio_port[i], device_gpio_pin[i]);
        }
    }
}

/**
  * @brief  控制指定设备的开关状态
  * @param  device_code  设备编号, 取值为 DEVICE_SW_FAN_1 ~ DEVICE_SW_VALVE_2
  * @param  switch_state 开关状态, DEVICE_STATE_HIGH(高电平) 或 DEVICE_STATE_LOW(低电平)
  * @return 无
  * @note   高电平=关闭(风扇/水泵/阀门), 低电平=开启(风扇/水泵/阀门);
  *         M_POWER_C相反: 高电平=开启, 低电平=关闭
  *         非法参数时函数直接返回, 不执行任何操作
  */
void control_device(uint8_t device_code, uint8_t switch_state)
{
    if (device_code >= DEVICE_NUM) {
        return;
    }

    if (switch_state != DEVICE_STATE_LOW && switch_state != DEVICE_STATE_HIGH) {
        return;
    }

    if (switch_state == DEVICE_STATE_HIGH) {
        gpio_bits_set((gpio_type*)device_gpio_port[device_code], device_gpio_pin[device_code]);
    } else {
        gpio_bits_reset((gpio_type*)device_gpio_port[device_code], device_gpio_pin[device_code]);
    }
}
