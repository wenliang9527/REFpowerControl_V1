/**
  ******************************************************************************
  * @file    device_control.c
  * @brief   设备控制驱动实现文件
  * @version V2.0.0
  * @date    2026-04-27
  * @note    功能说明:
  *          1. 多路设备开关控制 (制冷、风扇、水泵、阀门等)
  *          2. GPIO极性: 低电平有效 (gpio_bits_set=关, gpio_bits_reset=开)
  ******************************************************************************
  */

#include "device_control.h"
#include "at32f403a_407_wk_config.h"

static const gpio_type* device_gpio_port[DEVICE_NUM] = {
    SW_REF_GPIO_PORT,
    SW_FAN_1_GPIO_PORT,
    SW_FAN_2_GPIO_PORT,
    SW_WPUMP_1_GPIO_PORT,
    M_POWER_C_GPIO_PORT,
    SW_WPUMP_2_GPIO_PORT,
    SW_VALVE_1_GPIO_PORT,
    SW_VALVE_2_GPIO_PORT
};

static const uint16_t device_gpio_pin[DEVICE_NUM] = {
    SW_REF_PIN,
    SW_FAN_1_PIN,
    SW_FAN_2_PIN,
    SW_WPUMP_1_PIN,
    M_POWER_C_PIN,
    SW_WPUMP_2_PIN,
    SW_VALVE_1_PIN,
    SW_VALVE_2_PIN
};

static const crm_periph_clock_type device_gpio_clock[DEVICE_NUM] = {
    CRM_GPIOB_PERIPH_CLOCK,
    CRM_GPIOA_PERIPH_CLOCK,
    CRM_GPIOA_PERIPH_CLOCK,
    CRM_GPIOA_PERIPH_CLOCK,
    CRM_GPIOC_PERIPH_CLOCK,
    CRM_GPIOC_PERIPH_CLOCK,
    CRM_GPIOD_PERIPH_CLOCK,
    CRM_GPIOC_PERIPH_CLOCK
};

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
