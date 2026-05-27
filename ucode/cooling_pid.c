/**
  ******************************************************************************
  * @file    cooling_pid.c
  * @brief   冷却PID控制模块实现文件
  * @version V1.0.0
  * @date    2026-05-26
  * @note    基于位置式PID算法控制制冷PWM占空比
  *          PWM硬件: TMR2通道4, 占空比0~100%
  *          迟滞启停: 温度>ON阈值启动, 温度<OFF阈值停止
  *          安全条件: 水泵未允许或传感器无效时强制关闭输出
  ******************************************************************************
  */

#include "cooling_pid.h"
#include "at32f403a_407_wk_config.h"

/**
  * @brief  设置PWM占空比
  * @param  duty_percent: 占空比 (0~100%)
  * @note   占空比为0时关闭PWM输出并拉高SW_REF引脚(关闭制冷);
  *          占空比非0时使能PWM输出, CCR值根据占空比和定时器周期计算
  */
static void CoolingPID_SetPWMDuty(uint8_t duty_percent)
{
    uint32_t period = tmr_period_value_get(TMR2) + 1;
    uint32_t ccr = (uint32_t)duty_percent * period / 100;

    if (duty_percent == 0) {
        tmr_channel_enable(TMR2, TMR_SELECT_CHANNEL_4, FALSE);
        tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_4, 0);
    } else {
        tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_4, ccr);
        tmr_channel_enable(TMR2, TMR_SELECT_CHANNEL_4, TRUE);
    }
}

/**
  * @brief  冷却PID模块初始化
  * @param  pid: PID控制结构体指针
  * @note   使用默认参数初始化所有PID变量, 配置TMR2通道4为PWM模式A,
  *          初始占空比为0, 关闭PWM输出, 拉高SW_REF引脚(关闭制冷)
  */
void CoolingPID_Init(CoolingPID_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->kp = COOLING_PID_DEFAULT_KP;
    pid->ki = COOLING_PID_DEFAULT_KI;
    pid->kd = COOLING_PID_DEFAULT_KD;
    pid->target = COOLING_PID_TARGET_TEMP;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output = 0.0f;
    pid->pid_active = 0;
    pid->duty_percent = 0;

    tmr_output_channel_mode_select(TMR2, TMR_SELECT_CHANNEL_4, TMR_OUTPUT_CONTROL_PWM_MODE_A);
    tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_4, 0);
    tmr_channel_enable(TMR2, TMR_SELECT_CHANNEL_4, FALSE);
}

/**
  * @brief  设置PID参数
  * @param  pid: PID控制结构体指针
  * @param  kp: 比例系数
  * @param  ki: 积分系数
  * @param  kd: 微分系数
  * @note   修改参数后清零积分项和上次误差, 避免历史数据干扰新参数下的控制
  */
void CoolingPID_SetParams(CoolingPID_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL) {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}

/**
  * @brief  设置PID目标温度
  * @param  pid: PID控制结构体指针
  * @param  target: 目标温度 (单位: °C)
  * @note   修改目标后清零积分项和上次误差, 使PID从新基准开始调节
  */
void CoolingPID_SetTarget(CoolingPID_t *pid, float target)
{
    if (pid == NULL) {
        return;
    }

    pid->target = target;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}

/**
  * @brief  执行PID计算并更新PWM输出
  * @param  pid: PID控制结构体指针
  * @param  current_temp: 当前温度 (单位: °C)
  * @param  wpump_allowed: 水泵允许运行标志, 0=禁止, 1=允许
  * @param  ser2_valid: 传感器2有效标志, 0=无效, 1=有效
  * @note   执行流程:
  *          1. 安全检查: 水泵未允许或传感器无效时强制关闭输出
  *          2. 迟滞启停: 温度>ON阈值激活, 温度<OFF阈值关闭
  *          3. PID计算: output = Kp*error + Ki*integral + Kd*derivative
  *          4. 积分限幅: integral限制在[0, INTEGRAL_MAX], 防止积分饱和
  *          5. 输出限幅: output限制在[0, MAX_DUTY]
  *          6. 最小占空比: output<MIN_DUTY时输出0%, 避免低效运行
  */
void CoolingPID_Compute(CoolingPID_t *pid, float current_temp,
                         uint8_t wpump_allowed, uint8_t ser2_valid)
{
    float error;
    float derivative;

    if (pid == NULL) {
        return;
    }

    if (!wpump_allowed || !ser2_valid) {
        pid->pid_active = 0;
        pid->duty_percent = 0;
        pid->integral = 0.0f;
        pid->prev_error = 0.0f;
        pid->output = 0.0f;
        CoolingPID_SetPWMDuty(0);
        return;
    }

    if (current_temp > COOLING_PID_ON_THRESHOLD) {
        pid->pid_active = 1;
    } else if (current_temp < COOLING_PID_OFF_THRESHOLD) {
        pid->pid_active = 0;
    }

    if (!pid->pid_active) {
        pid->duty_percent = 0;
        pid->integral = 0.0f;
        pid->prev_error = 0.0f;
        pid->output = 0.0f;
        CoolingPID_SetPWMDuty(0);
        return;
    }

    error = current_temp - pid->target;

    pid->integral += error;
    if (pid->integral > COOLING_PID_INTEGRAL_MAX) {
        pid->integral = COOLING_PID_INTEGRAL_MAX;
    }
    if (pid->integral < 0.0f) {
        pid->integral = 0.0f;
    }

    derivative = error - pid->prev_error;

    pid->output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

    if (pid->output < 0.0f) {
        pid->output = 0.0f;
    }
    if (pid->output > (float)COOLING_PID_MAX_DUTY) {
        pid->output = (float)COOLING_PID_MAX_DUTY;
    }

    pid->prev_error = error;

    if (pid->output < (float)COOLING_PID_MIN_DUTY) {
        pid->duty_percent = 0;
        CoolingPID_SetPWMDuty(0);
    } else {
        pid->duty_percent = (uint8_t)pid->output;
        CoolingPID_SetPWMDuty(pid->duty_percent);
    }
}

/**
  * @brief  获取PID激活状态
  * @param  pid: PID控制结构体指针
  * @return 1=PID运行中, 0=PID已停止或指针为NULL
  */
uint8_t CoolingPID_IsActive(CoolingPID_t *pid)
{
    if (pid == NULL) {
        return 0;
    }
    return pid->pid_active;
}

/**
  * @brief  获取当前PWM占空比
  * @param  pid: PID控制结构体指针
  * @return 当前占空比 (单位: %), 指针为NULL时返回0
  */
uint8_t CoolingPID_GetDuty(CoolingPID_t *pid)
{
    if (pid == NULL) {
        return 0;
    }
    return pid->duty_percent;
}
