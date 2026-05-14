/**
  ******************************************************************************
  * @file    sensor_input.c
  * @brief   传感器输入驱动实现文件
  * @version V1.0.0
  * @date    2026-04-23
  * @note    功能说明:
  *          1. 多路传感器输入检测 (SER_IN_1/2/3)
  *          2. 50ms消抖处理: 检测到状态变化后需持续50个扫描周期才确认
  *          3. 在TMR4中断中调用SensorInput_Scan进行扫描
  *          4. 每次扫描后调用液位流量联锁处理函数
  ******************************************************************************
  */

#include "sensor_input.h"
#include "fluid_flow_interlock.h"
#include "at32f403a_407_wk_config.h"

/** 全局传感器输入状态变量, 记录所有通道的消抖后状态 */
sensor_input_status_t g_sensor_input_status = {0};

/** 传感器GPIO端口映射表, 索引对应SENSOR_IN_*编号 */
static const gpio_type* sensor_gpio_port[SENSOR_INPUT_NUM] = {
    SER_IN_1_GPIO_PORT,
    SER_IN_2_GPIO_PORT,
    SER_IN_3_GPIO_PORT
};

/** 传感器GPIO引脚映射表, 索引对应SENSOR_IN_*编号 */
static const uint16_t sensor_gpio_pin[SENSOR_INPUT_NUM] = {
    SER_IN_1_PIN,
    SER_IN_2_PIN,
    SER_IN_3_PIN
};

/**
  * @brief  读取指定传感器的原始GPIO电平
  * @param  sensor_id: 传感器编号
  * @return 0=低电平, 1=高电平
  */
static uint8_t read_sensor_gpio(uint8_t sensor_id)
{
    if (sensor_id >= SENSOR_INPUT_NUM) {
        return 0;
    }

    return (gpio_input_data_bit_read((gpio_type*)sensor_gpio_port[sensor_id],
                                      sensor_gpio_pin[sensor_id]) == SET) ? 1 : 0;
}

/**
  * @brief  单路传感器消抖处理
  * @param  sensor_id: 传感器编号
  * @note   消抖逻辑: 当GPIO状态与当前消抖后状态不一致时, 开始计数
  *         持续DEBOUNCE_TIME_MS个周期后确认状态变化
  */
static void sensor_debounce_process(uint8_t sensor_id)
{
    sensor_status_t *sensor = &g_sensor_input_status.sensors[sensor_id];
    uint8_t gpio_state;

    /* 读取当前GPIO原始电平 */
    gpio_state = read_sensor_gpio(sensor_id);
    sensor->raw_state = gpio_state;

    /* GPIO状态与消抖后状态不一致, 启动消抖计数 */
    if (gpio_state != sensor->current_state) {
        sensor->debounce_count++;
        sensor->stable_count = 0;

        /* 消抖计数达到阈值, 确认状态变化 */
        if (sensor->debounce_count >= DEBOUNCE_TIME_MS) {
            sensor->current_state = gpio_state;
            sensor->debounce_count = 0;
        }
    } else {
        /* 状态一致, 清零消抖计数, 增加稳定计数 */
        sensor->debounce_count = 0;
        if (sensor->stable_count < DEBOUNCE_TIME_MS) {
            sensor->stable_count++;
        }
    }
}

/**
  * @brief  传感器模块初始化
  * @note   清零所有传感器状态和计数器
  */
void SensorInput_Init(void)
{
    uint8_t i;

    for (i = 0; i < SENSOR_INPUT_NUM; i++) {
        g_sensor_input_status.sensors[i].current_state = 0;
        g_sensor_input_status.sensors[i].raw_state = 0;
        g_sensor_input_status.sensors[i].debounce_count = 0;
        g_sensor_input_status.sensors[i].stable_count = 0;
    }
}

/**
  * @brief  传感器扫描处理函数, 在TMR4中断中调用
  * @note   对所有传感器通道执行消抖处理, 然后触发联锁检测
  */
void SensorInput_Scan(void)
{
    uint8_t i;

    for (i = 0; i < SENSOR_INPUT_NUM; i++) {
        sensor_debounce_process(i);
    }

    /* 触发液位流量联锁定时处理 */
    FluidInterlock_Tmr4Tick();
}

/**
  * @brief  获取指定传感器的消抖后状态
  * @param  sensor_id: 传感器编号
  * @return 0=低电平, 1=高电平
  */
uint8_t SensorInput_GetState(uint8_t sensor_id)
{
    if (sensor_id >= SENSOR_INPUT_NUM) {
        return 0;
    }

    return g_sensor_input_status.sensors[sensor_id].current_state;
}

/**
  * @brief  获取所有传感器状态结构体指针
  * @return 传感器状态结构体指针
  */
sensor_input_status_t* SensorInput_GetStatus(void)
{
    return &g_sensor_input_status;
}
