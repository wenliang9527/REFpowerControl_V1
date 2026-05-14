/**
  ******************************************************************************
  * @file    touch_key.c
  * @brief   触摸按键驱动实现文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    功能说明:
  *          1. 多路触摸按键检测 (4路: TOUCH_1~4)
  *          2. 按键消抖处理: 连续30次采样检测到按下才确认
  *          3. 短按事件检测: 按住150次采样才判定为有效短按
  *          4. 触摸键1锁定机制: 触发短按后锁定5秒, 防止重复触发
  *          5. 在TMR4中断中调用TouchKey_Scan扫描
  *          6. V1适配: 按键数量从9路调整为4路
  ******************************************************************************
  */

#include "touch_key.h"
#include "at32f403a_407_wk_config.h"
#include "device_control.h"
#include "led_beep.h"

/** 全局触摸按键状态变量 */
touch_key_status_t g_touch_key_status = {0};

/** 触摸按键GPIO端口映射表, 索引对应TOUCH_KEY_*编号 */
static const gpio_type* touch_key_gpio_port[TOUCH_KEY_NUM] = {
    TOUCH_1_GPIO_PORT,
    TOUCH_2_GPIO_PORT,
    TOUCH_3_GPIO_PORT,
    TOUCH_4_GPIO_PORT
};

/** 触摸按键GPIO引脚映射表, 索引对应TOUCH_KEY_*编号 */
static const uint16_t touch_key_pin[TOUCH_KEY_NUM] = {
    TOUCH_1_PIN,
    TOUCH_2_PIN,
    TOUCH_3_PIN,
    TOUCH_4_PIN
};

/**
  * @brief  读取指定触摸按键的GPIO电平
  * @param  key_id: 按键编号
  * @return 1=按下(低电平), 0=释放(高电平)
  * @note   触摸按键为低电平有效 (RESET = 按下)
  */
static uint8_t read_key_gpio(uint8_t key_id)
{
    if (key_id >= TOUCH_KEY_NUM) {
        return 0;
    }

    return (gpio_input_data_bit_read((gpio_type*)touch_key_gpio_port[key_id],
                                      touch_key_pin[key_id]) == RESET) ? 1 : 0;
}

/**
  * @brief  单路按键消抖处理
  * @param  key_id: 按键编号
  * @param  gpio_state: 当前GPIO状态 (1=按下, 0=释放)
  * @note   连续DEBOUNCE_COUNT次检测到按下才确认状态变化
  */
static void key_debounce_process(uint8_t key_id, uint8_t gpio_state)
{
    key_status_t *key = &g_touch_key_status.keys[key_id];

    if (gpio_state) {
        key->debounce_count++;
        if (key->debounce_count >= DEBOUNCE_COUNT) {
            key->current_state = KEY_STATE_PRESS;
            key->debounce_count = 0;
        }
    } else {
        key->debounce_count = 0;
        key->current_state = KEY_STATE_RELEASE;
    }
}

/**
  * @brief  单路按键状态机处理
  * @param  key_id: 按键编号
  * @note   状态流转: RELEASE -> DEBOUNCE -> PRESS -> 事件判定
  *         短按判定: 按下时间 >= SHORT_PRESS_TIME 且释放时触发
  */
static void key_state_machine(uint8_t key_id)
{
    key_status_t *key = &g_touch_key_status.keys[key_id];
    uint8_t gpio_state;

    gpio_state = read_key_gpio(key_id);

    if (key->current_state == KEY_STATE_RELEASE) {
        /* 释放状态: 检测到按下时进入消抖状态 */
        if (gpio_state) {
            key->current_state = KEY_STATE_DEBOUNCE;
            key->debounce_count = 0;
        }
        key->press_time = 0;
        key->event = KEY_EVENT_NONE;
    }
    else if (key->current_state == KEY_STATE_DEBOUNCE) {
        /* 消抖状态: 进行消抖处理 */
        key_debounce_process(key_id, gpio_state);
    }
    else if (key->current_state == KEY_STATE_PRESS) {
        /* 按下状态: 累计按下时间 */
        if (gpio_state) {
            key->press_time++;
        } else {
            /* 释放: 退回消抖状态 */
            key->current_state = KEY_STATE_DEBOUNCE;
            key->debounce_count = 0;

            /* 按下时间达到阈值且事件未被处理, 触发短按事件 */
            if (key->press_time >= SHORT_PRESS_TIME && !key->event_pending) {
                key->event = KEY_EVENT_SHORT_PRESS;
                key->event_pending = 1;
            }
        }
    }
}

/**
  * @brief  TOUCH_1锁定处理
  * @note   锁定期间递增计时器, 达到TOUCH1_LOCK_TIME后自动解锁
  */
static void touch1_lock_process(void)
{
    if (g_touch_key_status.touch1_lock.locked) {
        g_touch_key_status.touch1_lock.lock_timer++;
        if (g_touch_key_status.touch1_lock.lock_timer >= TOUCH1_LOCK_TIME) {
            g_touch_key_status.touch1_lock.locked = 0;
            g_touch_key_status.touch1_lock.lock_timer = 0;
        }
    }
}

/**
  * @brief  触摸按键模块初始化
  * @note   清零所有按键状态、事件标志和锁定标志
  */
void TouchKey_Init(void)
{
    uint8_t i;

    for (i = 0; i < TOUCH_KEY_NUM; i++) {
        g_touch_key_status.keys[i].current_state = KEY_STATE_RELEASE;
        g_touch_key_status.keys[i].last_state = KEY_STATE_RELEASE;
        g_touch_key_status.keys[i].press_time = 0;
        g_touch_key_status.keys[i].debounce_count = 0;
        g_touch_key_status.keys[i].event = KEY_EVENT_NONE;
        g_touch_key_status.keys[i].event_pending = 0;
    }

    g_touch_key_status.touch1_lock.locked = 0;
    g_touch_key_status.touch1_lock.lock_timer = 0;
}

/**
  * @brief  触摸按键扫描处理函数, 在TMR4中断中调用
  * @note   执行锁定处理、状态机扫描, TOUCH_1触发短按后进入5秒锁定
  */
void TouchKey_Scan(void)
{
    uint8_t i;

    /* 处理TOUCH_1锁定计时 */
    touch1_lock_process();

    for (i = 0; i < TOUCH_KEY_NUM; i++) {
        /* TOUCH_1锁定期间, 忽略其按下事件 */
        if (i == TOUCH_KEY_1) {
            if (g_touch_key_status.touch1_lock.locked) {
                /* 如果当前处于按下状态, 检测释放后重置 */
                if (g_touch_key_status.keys[i].current_state == KEY_STATE_PRESS) {
                    uint8_t gpio_state = read_key_gpio(i);
                    if (!gpio_state) {
                        g_touch_key_status.keys[i].current_state = KEY_STATE_RELEASE;
                        g_touch_key_status.keys[i].press_time = 0;
                    }
                }
                continue;
            }
        }

        /* 执行按键状态机处理 */
        key_state_machine(i);

        /* TOUCH_1触发短按事件时, 启动5秒锁定 */
        if (i == TOUCH_KEY_1) {
            if (g_touch_key_status.keys[i].event_pending) {
                g_touch_key_status.touch1_lock.locked = 1;
                g_touch_key_status.touch1_lock.lock_timer = 0;
            }
        }
    }
}

/**
  * @brief  触摸按键短按事件处理
  * @param  key: 触发短按的按键编号
  * @note   TOUCH_KEY_1: 切换电源状态 (控制LED指示灯和主电源设备)
  */
void TouchKeyShortpressevent(u8 key)
{
    static uint8_t power_state = 0;

    switch(key)
    {
        case TOUCH_KEY_1:
            /* 切换电源状态 */
            power_state = !power_state;
            if (power_state) {
                /* 打开电源: M_POWER_C 高, LED2 高, LED_POWER_C 低 */
                LED_PowerIndicator_Set(1);
                control_device(DEVICE_M_POWER_C, DEVICE_STATE_HIGH);
            } else {
                /* 关闭电源: M_POWER_C 低, LED2 低, LED_POWER_C 高 */
                LED_PowerIndicator_Set(0);
                control_device(DEVICE_M_POWER_C, DEVICE_STATE_LOW);
            }
            break;
        case TOUCH_KEY_2:
            break;
        case TOUCH_KEY_3:
            break;
        case TOUCH_KEY_4:
            break;
        default:
            break;
    }
}

/**
  * @brief  触摸按键事件处理函数, 在主循环中调用
  * @note   遍历所有按键, 处理待处理的短按事件
  */
void TouchKey_EventProcess(void)
{
    uint8_t i;

    for (i = 0; i < TOUCH_KEY_NUM; i++) {
        if (g_touch_key_status.keys[i].event_pending) {
            if (g_touch_key_status.keys[i].event == KEY_EVENT_SHORT_PRESS) {
                TouchKeyShortpressevent(i);
                g_touch_key_status.keys[i].event_pending = 0;
                g_touch_key_status.keys[i].event = KEY_EVENT_NONE;
            }
        }
    }
}
