/**
  ******************************************************************************
  * @file    valve_control.c
  * @brief   注水阀控制模块实现文件
  * @version V2.0.0
  * @date    2026-05-15
  * @note    功能说明:
  *          1. 基于SER_IN_1信号控制SW_VALVE_1阀门开关
  *          2. 上电初始化: 高电平持续1s开阀，低电平持续5s关阀
  *          3. 系统运行: 高电平持续10s开阀，低电平持续5s关阀
  *          4. 使用TMR4中断(1ms周期)进行精确定时
  *          5. V2.0: 重构为TMR4中断驱动，修正定时基准
  ******************************************************************************
  */

#include "valve_control.h"
#include "at32f403a_407_wk_config.h"
#include "at32f403a_407_crm.h"

/** @brief 阀门控制状态全局变量 */
valve_control_status_t g_valve_control_status = {VALVE_STATE_INIT, 0u, 0u, 0u};

/**
 * @brief  读取SER_IN_1引脚电平
 * @return 1-高电平(需要注水), 0-低电平(不需要注水)
 */
static uint8_t valve_read_ser_in1(void)
{
  return (gpio_input_data_bit_read(SER_IN_1_GPIO_PORT, SER_IN_1_PIN) == SET) ? 1u : 0u;
}

/**
 * @brief  设置阀门输出电平
 * @param  state 0-关阀(输出高电平), 1-开阀(输出低电平)
 * @note   SW_VALVE_1高电平=关阀, 低电平=开阀
 */
static void valve_set_output(uint8_t state)
{
  if (state == 0u) {
    gpio_bits_set(SW_VALVE_1_GPIO_PORT, SW_VALVE_1_PIN);
  } else {
    gpio_bits_reset(SW_VALVE_1_GPIO_PORT, SW_VALVE_1_PIN);
  }
}

/** @brief 开阀操作, 输出低电平并置位阀门打开标志 */
static void valve_open(void)
{
  valve_set_output(1u);
  g_valve_control_status.valve_open = 1u;
}

/** @brief 关阀操作, 输出高电平并清除阀门打开标志 */
static void valve_close(void)
{
  valve_set_output(0u);
  g_valve_control_status.valve_open = 0u;
}

/**
 * @brief  初始化阀门控制模块
 * @note   状态置为INIT, 计数器清零, 阀门关闭
 */
void ValveControl_Init(void)
{
  g_valve_control_status.state = VALVE_STATE_INIT;
  g_valve_control_status.high_streak_counter = 0u;
  g_valve_control_status.low_streak_counter = 0u;
  g_valve_control_status.valve_open = 0u;

  valve_close();
}

/**
 * @brief  状态INIT处理: 等待SER_IN_1高电平
 * @param  ser_level SER_IN_1当前电平, 1-高电平, 0-低电平
 * @note   检测到高电平转入INIT_HIGH_WAIT, 低电平持续计数
 */
static void valve_state_init_handler(uint8_t ser_level)
{
    if (ser_level == 1u) {
        g_valve_control_status.high_streak_counter = 1u;
        g_valve_control_status.low_streak_counter = 0u;
        g_valve_control_status.state = VALVE_STATE_INIT_HIGH_WAIT;
    } else {
        g_valve_control_status.low_streak_counter++;
        g_valve_control_status.high_streak_counter = 0u;
    }
}

/**
 * @brief  状态INIT_HIGH_WAIT处理: 高电平持续1s后开阀
 * @param  ser_level SER_IN_1当前电平, 1-高电平, 0-低电平
 * @note   高电平持续VALVE_INIT_HIGH_TIME_MS后开阀并转入OPEN;
 *         低电平持续VALVE_DEBOUNCE_MS后回退到INIT(消抖)
 */
static void valve_state_init_high_wait_handler(uint8_t ser_level)
{
    if (ser_level == 1u) {
        g_valve_control_status.high_streak_counter++;
        g_valve_control_status.low_streak_counter = 0u;
        if (g_valve_control_status.high_streak_counter >= VALVE_INIT_HIGH_TIME_MS) {
            valve_open();
            g_valve_control_status.state = VALVE_STATE_OPEN;
        }
    } else {
        g_valve_control_status.low_streak_counter++;
        g_valve_control_status.high_streak_counter = 0u;
        if (g_valve_control_status.low_streak_counter >= VALVE_DEBOUNCE_MS) {
            g_valve_control_status.state = VALVE_STATE_INIT;
            g_valve_control_status.low_streak_counter = 0u;
        }
    }
}

/** @brief 状态OPEN处理: 开阀后转入OPEN_LOW_WAIT等待低电平 */
static void valve_state_open_handler(void)
{
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;
    g_valve_control_status.state = VALVE_STATE_OPEN_LOW_WAIT;
}

/**
 * @brief  状态OPEN_LOW_WAIT处理: 低电平持续5s后关阀进入运行态
 * @param  ser_level SER_IN_1当前电平, 1-高电平, 0-低电平
 * @note   低电平持续VALVE_OPEN_LOW_TIME_MS后关阀并转入RUNNING;
 *         高电平持续VALVE_DEBOUNCE_MS后保持在OPEN(消抖)
 */
static void valve_state_open_low_wait_handler(uint8_t ser_level)
{
    if (ser_level == 0u) {
        g_valve_control_status.low_streak_counter++;
        g_valve_control_status.high_streak_counter = 0u;
        if (g_valve_control_status.low_streak_counter >= VALVE_OPEN_LOW_TIME_MS) {
            valve_close();
            g_valve_control_status.state = VALVE_STATE_RUNNING;
        }
    } else {
        g_valve_control_status.high_streak_counter++;
        g_valve_control_status.low_streak_counter = 0u;
        if (g_valve_control_status.high_streak_counter >= VALVE_DEBOUNCE_MS) {
            g_valve_control_status.state = VALVE_STATE_OPEN;
        }
    }
}

/**
 * @brief  状态RUNNING处理: 等待SER_IN_1高电平
 * @param  ser_level SER_IN_1当前电平, 1-高电平, 0-低电平
 * @note   检测到高电平转入RUNNING_HIGH_WAIT
 */
static void valve_state_running_handler(uint8_t ser_level)
{
    g_valve_control_status.high_streak_counter = 0u;
    g_valve_control_status.low_streak_counter = 0u;

    if (ser_level == 1u) {
        g_valve_control_status.state = VALVE_STATE_RUNNING_HIGH_WAIT;
    }
}

/**
 * @brief  状态RUNNING_HIGH_WAIT处理: 高电平持续10s后开阀
 * @param  ser_level SER_IN_1当前电平, 1-高电平, 0-低电平
 * @note   高电平持续VALVE_RUNNING_HIGH_TIME_MS后开阀并转入OPEN;
 *         低电平持续VALVE_DEBOUNCE_MS后回退到RUNNING(消抖)
 */
static void valve_state_running_high_wait_handler(uint8_t ser_level)
{
    if (ser_level == 1u) {
        g_valve_control_status.high_streak_counter++;
        g_valve_control_status.low_streak_counter = 0u;
        if (g_valve_control_status.high_streak_counter >= VALVE_RUNNING_HIGH_TIME_MS) {
            valve_open();
            g_valve_control_status.state = VALVE_STATE_OPEN;
        }
    } else {
        g_valve_control_status.low_streak_counter++;
        g_valve_control_status.high_streak_counter = 0u;
        if (g_valve_control_status.low_streak_counter >= VALVE_DEBOUNCE_MS) {
            g_valve_control_status.state = VALVE_STATE_RUNNING;
            g_valve_control_status.low_streak_counter = 0u;
        }
    }
}

/**
 * @brief  阀门控制状态机处理
 * @note   在TMR4中断中周期性调用(1ms/tick), 根据当前状态分发到对应处理函数
 */
void ValveControl_Process(void)
{
    uint8_t ser_level = valve_read_ser_in1();

    switch (g_valve_control_status.state) {
        case VALVE_STATE_INIT:
            valve_state_init_handler(ser_level);
            break;

        case VALVE_STATE_INIT_HIGH_WAIT:
            valve_state_init_high_wait_handler(ser_level);
            break;

        case VALVE_STATE_OPEN:
            valve_state_open_handler();
            break;

        case VALVE_STATE_OPEN_LOW_WAIT:
            valve_state_open_low_wait_handler(ser_level);
            break;

        case VALVE_STATE_RUNNING:
            valve_state_running_handler(ser_level);
            break;

        case VALVE_STATE_RUNNING_HIGH_WAIT:
            valve_state_running_high_wait_handler(ser_level);
            break;

        default:
            g_valve_control_status.state = VALVE_STATE_INIT;
            break;
    }
}

/**
 * @brief  获取当前阀门状态
 * @return 当前阀门状态枚举值
 */
valve_state_t ValveControl_GetState(void)
{
  return g_valve_control_status.state;
}

/**
 * @brief  查询阀门是否打开
 * @return 1-阀门已打开, 0-阀门已关闭
 */
uint8_t ValveControl_IsValveOpen(void)
{
  return g_valve_control_status.valve_open;
}

/**
 * @brief  强制关闭阀门
 * @note   关中断保护, 重置状态为INIT并关阀, 防止与中断状态机竞争
 */
void ValveControl_ForceClose(void)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  g_valve_control_status.state = VALVE_STATE_INIT;
  g_valve_control_status.high_streak_counter = 0u;
  g_valve_control_status.low_streak_counter = 0u;
  g_valve_control_status.valve_open = 0u;
  valve_close();

  __set_PRIMASK(primask);
}

/**
 * @brief  获取阀门控制状态结构体指针
 * @return 阀门控制状态结构体指针
 */
valve_control_status_t* ValveControl_GetStatus(void)
{
  return &g_valve_control_status;
}
