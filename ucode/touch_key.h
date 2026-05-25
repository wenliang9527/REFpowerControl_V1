/**
  ******************************************************************************
  * @file    touch_key.h
  * @brief   触摸按键驱动头文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    V1适配: 按键数量从9路调整为4路 (TOUCH_1~4)
  *          消抖计数: 30次采样, 短按判定: 150次采样, TOUCH_1锁定: 5秒
  ******************************************************************************
  */

#ifndef __TOUCH_KEY_H
#define __TOUCH_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

#define TOUCH_KEY_NUM           4  /**< 触摸按键通道数 */

/**
 * @defgroup 触摸按键编号定义
 */
#define TOUCH_KEY_1             0  /**< 触摸按键1 (电源开关, 带5秒锁定) */
#define TOUCH_KEY_2             1  /**< 触摸按键2 (预留) */
#define TOUCH_KEY_3             2  /**< 触摸按键3 (预留) */
#define TOUCH_KEY_4             3  /**< 触摸按键4 (预留) */

/** 消抖采样次数阈值, 连续30次检测到按下才确认 */
#define DEBOUNCE_COUNT          30
/** 短按判定采样次数阈值, 按住150次采样才判定为有效短按 */
#define SHORT_PRESS_TIME        150
/** TOUCH_1短按判定采样次数阈值, 按住5000次采样才判定为有效短按 */
#define TOUCH1_SHORT_PRESS_TIME 5000
/** TOUCH_1锁定时间计数, 触发短按后锁定30000个扫描周期 */
#define TOUCH1_LOCK_TIME        30000

/**
  * @brief  按键事件类型枚举
  */
typedef enum {
    KEY_EVENT_NONE = 0,        /**< 无事件 */
    KEY_EVENT_SHORT_PRESS,     /**< 短按事件 */
    KEY_EVENT_RELEASE          /**< 释放事件 */
} key_event_type;

/**
  * @brief  按键状态类型枚举
  */
typedef enum {
    KEY_STATE_RELEASE = 0,     /**< 释放状态 */
    KEY_STATE_PRESS,           /**< 按下状态 */
    KEY_STATE_DEBOUNCE         /**< 消抖中状态 */
} key_state_type;

/**
  * @brief  单个按键状态结构体
  */
typedef struct {
    uint8_t current_state;     /**< 当前按键状态 (key_state_type) */
    uint8_t last_state;        /**< 上一次按键状态 */
    uint16_t press_time;       /**< 按下持续时间 (扫描周期数) */
    uint16_t debounce_count;   /**< 消抖计数 */
    key_event_type event;      /**< 按键事件类型 */
    uint8_t event_pending;     /**< 事件待处理标志: 1=有待处理事件 */
} key_status_t;

/**
  * @brief  TOUCH_1锁定状态结构体
  */
typedef struct {
    uint8_t locked;            /**< 锁定标志: 1=已锁定, 0=未锁定 */
    uint16_t lock_timer;       /**< 锁定计时器 */
} touch1_lock_t;

/**
  * @brief  所有触摸按键状态集合
  */
typedef struct {
    key_status_t keys[TOUCH_KEY_NUM];  /**< 各按键状态数组 */
    touch1_lock_t touch1_lock;         /**< TOUCH_1锁定状态 */
} touch_key_status_t;

extern volatile touch_key_status_t g_touch_key_status;

/**
  * @brief  触摸按键模块初始化
  * @note   清零所有按键状态和锁定标志
  */
void TouchKey_Init(void);

/**
  * @brief  触摸按键扫描处理函数
  * @note   需在TMR4中断中周期性调用, 执行消抖、状态机处理和锁定逻辑
  */
void TouchKey_Scan(void);

/**
  * @brief  触摸按键事件处理函数
  * @note   在主循环中调用, 处理待处理的按键事件(如短按触发电源开关)
  */
void TouchKey_EventProcess(void);

#ifdef __cplusplus
}
#endif

#endif
