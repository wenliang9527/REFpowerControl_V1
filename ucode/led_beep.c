/**
  ******************************************************************************
  * @file    led_beep.c
  * @brief   LED和蜂鸣器驱动实现文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    功能说明:
  *          1. LED周期闪烁控制 (5路LED)
  *          2. 蜂鸣器周期性鸣响控制
  *          3. 在定时器中断中调用LED_Thread
  *          4. V1适配: LED从1个扩展为5个
  *          5. GPIO极性: LED低电平点亮, 蜂鸣器高电平鸣响
  ******************************************************************************
  */

#include "led_beep.h"

/** LED闪烁计时器 (全局) */
u32 TMR_LEDtimer = 0;
/** 蜂鸣器计时器 (全局) */
u32 TMR_BEEPtimer = 0;
/** 蜂鸣器标志 */
u8 beep_flag = 0;
/** 蜂鸣器设置标志 */
u8 beep_setflag = 1;
/** LED电平状态标志 */
bool LED_LH = false;

/** LED1闪烁计时器 */
static u32 TMR_LED1_timer = 0;
/** 蜂鸣器每次鸣响时间 (定时周期数) */
static u16 beep_on_time = 0;
/** 蜂鸣器每次间隔时间 (定时周期数) */
static u16 beep_off_time = 0;
/** 蜂鸣器鸣响总次数 */
static u8 beep_count = 0;
/** 蜂鸣器当前已鸣响次数 */
static u8 beep_current_count = 0;
/** 蜂鸣器状态: 0=停止, 1=鸣响中, 2=间隔中 */
static u8 beep_state = 0;

static uint8_t s_power_on_state = 0;
static uint8_t s_power_blink_active = 0;
static u32 s_power_blink_timer = 0;
#define POWER_BLINK_PERIOD  5000u

/**
  * @brief  LED闪烁线程处理函数
  * @note   在定时器中断中调用, 处理LED1周期闪烁(每LED1_FLASH_TIME翻转一次)
  *         同时调用BEEP_Process处理蜂鸣器
  */
void LED_Thread(void)
{
    TMR_LED1_timer++;
    
    if(TMR_LED1_timer >= LED1_FLASH_TIME)
    {
        LED_1_Reversal;
        TMR_LED1_timer = 0;
    }

    if (s_power_blink_active) {
        s_power_blink_timer++;
        if (s_power_blink_timer >= POWER_BLINK_PERIOD) {
            s_power_blink_timer = 0;
            if (s_power_on_state) {
                gpio_bits_toggle(LED_POWER_C_GPIO_PORT, LED_POWER_C_PIN);
            } else {
                gpio_bits_toggle(LED2_GPIO_PORT, LED2_PIN);
            }
        }
    }
    
    BEEP_Process();
}

/**
  * @brief  蜂鸣器开启 (输出高电平)
  */
void BEEP_On(void)
{
    gpio_bits_set(BEEP_GPIO_PORT, BEEP_PIN);
}

/**
  * @brief  蜂鸣器关闭 (输出低电平)
  */
void BEEP_Off(void)
{
    gpio_bits_reset(BEEP_GPIO_PORT, BEEP_PIN);
}

/**
  * @brief  设置指定LED的状态
  * @param  led_id: LED编号 (1~5)
  * @param  state: 1=点亮(低电平), 0=熄灭(高电平)
  */
void LED_SetState(uint8_t led_id, uint8_t state)
{
    switch(led_id) {
        case 1:
            if (state) {
                gpio_bits_reset(LED1_GPIO_PORT, LED1_PIN);
            } else {
                gpio_bits_set(LED1_GPIO_PORT, LED1_PIN);
            }
            break;
        case 2:
            if (state) {
                gpio_bits_reset(LED2_GPIO_PORT, LED2_PIN);
            } else {
                gpio_bits_set(LED2_GPIO_PORT, LED2_PIN);
            }
            break;
        case 3:
            if (state) {
                gpio_bits_reset(LED3_GPIO_PORT, LED3_PIN);
            } else {
                gpio_bits_set(LED3_GPIO_PORT, LED3_PIN);
            }
            break;
        case 4:
            if (state) {
                gpio_bits_reset(LED4_GPIO_PORT, LED4_PIN);
            } else {
                gpio_bits_set(LED4_GPIO_PORT, LED4_PIN);
            }
            break;
        case 5:
            if (state) {
                gpio_bits_reset(LED5_GPIO_PORT, LED5_PIN);
            } else {
                gpio_bits_set(LED5_GPIO_PORT, LED5_PIN);
            }
            break;
        default:
            break;
    }
}

/**
  * @brief  翻转指定LED的状态
  * @param  led_id: LED编号 (1~5)
  */
void LED_Toggle(uint8_t led_id)
{
    switch(led_id) {
        case 1:
            gpio_bits_toggle(LED1_GPIO_PORT, LED1_PIN);
            break;
        case 2:
            gpio_bits_toggle(LED2_GPIO_PORT, LED2_PIN);
            break;
        case 3:
            gpio_bits_toggle(LED3_GPIO_PORT, LED3_PIN);
            break;
        case 4:
            gpio_bits_toggle(LED4_GPIO_PORT, LED4_PIN);
            break;
        case 5:
            gpio_bits_toggle(LED5_GPIO_PORT, LED5_PIN);
            break;
        default:
            break;
    }
}

/**
  * @brief  控制蜂鸣器鸣响
  * @param  on_time_ms: 每次鸣响时间 (毫秒)
  * @param  count: 鸣响次数
  * @note   鸣响模式: on_time_ms鸣响 -> on_time_ms间隔, 循环count次
  *         内部将毫秒值乘以4转换为定时周期数
  */
void BEEP_Control(uint16_t on_time_ms, uint8_t count)
{
    if (count == 0 || on_time_ms == 0) {
        return;
    }
    
    TMR_BEEPtimer = 0;
    beep_on_time = on_time_ms * 4;
    beep_off_time = on_time_ms * 4;
    beep_count = count;
    beep_current_count = 0;
    beep_state = 1;
    BEEP_On();
}

/**
  * @brief  蜂鸣器处理函数 (状态机)
  * @note   状态1: 鸣响中, 达到时间后关闭并进入状态2
  *         状态2: 间隔中, 达到时间后开启并回到状态1
  *         鸣响次数达到beep_count后进入状态0停止
  */
void BEEP_Process(void)
{
    if (beep_state == 0) {
        return;
    }
    
    switch (beep_state) {
        case 1:
            if (TMR_BEEPtimer >= beep_on_time) {
                TMR_BEEPtimer = 0;
                BEEP_Off();
                beep_current_count++;
                if (beep_current_count >= beep_count) {
                    beep_state = 0;
                } else {
                    beep_state = 2;
                }
            } else {
                TMR_BEEPtimer++;
            }
            break;
            
        case 2:
            if (TMR_BEEPtimer >= beep_off_time) {
                TMR_BEEPtimer = 0;
                BEEP_On();
                beep_state = 1;
            } else {
                TMR_BEEPtimer++;
            }
            break;
            
        default:
            beep_state = 0;
            break;
    }
}

/**
  * @brief  设置电源指示灯状态 (与 M_POWER_C 开关联动由 touch_key 调用)
  * @param  power_on: 1=电源打开, 0=电源关闭
  * @note   打开: LED_POWER_C 低、LED2 高 (与 TouchKeyShortpressevent 中 M_POWER_C 高一致)
  *         关闭: LED_POWER_C 高、LED2 低 (与 M_POWER_C 低一致)
  */
void LED_PowerIndicator_Set(uint8_t power_on)
{
    if (power_on) {
        gpio_bits_reset(LED_POWER_C_GPIO_PORT, LED_POWER_C_PIN);
        gpio_bits_set(LED2_GPIO_PORT, LED2_PIN);
    } else {
        gpio_bits_set(LED_POWER_C_GPIO_PORT, LED_POWER_C_PIN);
        gpio_bits_reset(LED2_GPIO_PORT, LED2_PIN);
    }
    s_power_on_state = power_on;
}

void LED_PowerIndicator_BlinkStart(void)
{
    s_power_blink_active = 1;
    s_power_blink_timer = 0;
}

void LED_PowerIndicator_BlinkStop(void)
{
    s_power_blink_active = 0;
    s_power_blink_timer = 0;
    LED_PowerIndicator_Set(s_power_on_state);
}

uint8_t LED_PowerIndicator_GetState(void)
{
    return s_power_on_state;
}
