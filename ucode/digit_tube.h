/**
  ******************************************************************************
  * @file    digit_tube.h
  * @brief   3位6引脚数码管驱动头文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    V1适配: 引脚映射已更新
  *          DTUBE_1=PD9, DTUBE_2=PD8, DTUBE_3=PB15, DTUBE_4=PB14, DTUBE_5=PB13, DTUBE_6=PB12
  ******************************************************************************
  */

#ifndef __DIGIT_TUBE_H
#define __DIGIT_TUBE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f403a_407.h"

/** 数码管引脚1: PD9 */
#define TUBE_PIN_1_GPIO         GPIOD
#define TUBE_PIN_1_PIN          GPIO_PINS_9
#define TUBE_PIN_1_CLOCK        CRM_GPIOD_PERIPH_CLOCK

/** 数码管引脚2: PD8 */
#define TUBE_PIN_2_GPIO         GPIOD
#define TUBE_PIN_2_PIN          GPIO_PINS_8
#define TUBE_PIN_2_CLOCK        CRM_GPIOD_PERIPH_CLOCK

/** 数码管引脚3: PB15 */
#define TUBE_PIN_3_GPIO         GPIOB
#define TUBE_PIN_3_PIN          GPIO_PINS_15
#define TUBE_PIN_3_CLOCK        CRM_GPIOB_PERIPH_CLOCK

/** 数码管引脚4: PB14 */
#define TUBE_PIN_4_GPIO         GPIOB
#define TUBE_PIN_4_PIN          GPIO_PINS_14
#define TUBE_PIN_4_CLOCK        CRM_GPIOB_PERIPH_CLOCK

/** 数码管引脚5: PB13 */
#define TUBE_PIN_5_GPIO         GPIOB
#define TUBE_PIN_5_PIN          GPIO_PINS_13
#define TUBE_PIN_5_CLOCK        CRM_GPIOB_PERIPH_CLOCK

/** 数码管引脚6: PB12 */
#define TUBE_PIN_6_GPIO         GPIOB
#define TUBE_PIN_6_PIN          GPIO_PINS_12
#define TUBE_PIN_6_CLOCK        CRM_GPIOB_PERIPH_CLOCK

/** 扫描定时器: TMR3 */
#define TUBE_SCAN_TIMER         TMR3
/** 扫描定时器外设时钟 */
#define TUBE_SCAN_TIMER_CLOCK   CRM_TMR3_PERIPH_CLOCK
/** 扫描定时器中断号 */
#define TUBE_SCAN_TIMER_IRQ     TMR3_GLOBAL_IRQn

/** 扫描定时器周期值 (计数上限) */
#define TUBE_SCAN_PERIOD        100
/** 扫描定时器分频值 */
#define TUBE_SCAN_DIV           240

/** 扫描中断抢占优先级 */
#define TUBE_SCAN_IRQ_PREEMPT   0
/** 扫描中断子优先级 */
#define TUBE_SCAN_IRQ_SUB       0

/** 数码管最大显示位数 */
#define TUBE_DIGIT_NUM          3
/** 扫描频率 (Hz), 3位×7段=21次/周期, 实际刷新率=SCAN_FREQ/21 */
#define TUBE_SCAN_FREQ          450

/** 温度单位: 摄氏度 (°C) */
#define TEMP_UNIT_CELSIUS       0
/** 温度单位: 华氏度 (°F) */
#define TEMP_UNIT_FAHRENHEIT    1

/** 段码定义: A段 (顶部水平) */
#define SEG_A                   0x01
/** 段码定义: B段 (右上垂直) */
#define SEG_B                   0x02
/** 段码定义: C段 (右下垂直) */
#define SEG_C                   0x04
/** 段码定义: D段 (底部水平) */
#define SEG_D                   0x08
/** 段码定义: E段 (左下垂直) */
#define SEG_E                   0x10
/** 段码定义: F段 (左上垂直) */
#define SEG_F                   0x20
/** 段码定义: G段 (中间水平) */
#define SEG_G                   0x40

/** 字符段码: 大写'E' */
#define SEG_CHAR_E              0x79
/** 字符段码: 小写'r' */
#define SEG_CHAR_r              0x50
/** 字符段码: 大写'C' (摄氏度标识) */
#define SEG_CHAR_C              (SEG_A | SEG_F | SEG_E | SEG_D)
/** 字符段码: 大写'F' (华氏度标识) */
#define SEG_CHAR_F              (SEG_A | SEG_F | SEG_E | SEG_G)
/** 字符段码: 负号'-' */
#define SEG_CHAR_MINUS          (SEG_G)
/** 字符段码: 空白 (全段熄灭) */
#define SEG_CHAR_BLANK          0x00

/** 扫描计数器, 每次定时器中断递增 */
extern volatile uint32_t g_scan_counter;

/**
  * @brief  将数码管显示数据同步到全局传感器数据结构
  */
void DigitTube_SyncToGlobal(void);
/**
  * @brief  从全局传感器数据同步到数码管显示 (关中断保护)
  */
void DigitTube_SyncFromGlobal(void);
/**
  * @brief  获取当前显示值
  * @return 显示数值, 范围0~999
  */
uint16_t DigitTube_GetValue(void);
/**
  * @brief  设置LED Q1/Q2状态
  * @param  q1: LED Q1状态, 0=灭, 1=亮
  * @param  q2: LED Q2状态, 0=灭, 1=亮
  */
void DigitTube_SetLEDState(uint8_t q1, uint8_t q2);
/**
  * @brief  设置显示位数 (2或3位)
  * @param  digits: 显示位数, 仅支持2或3
  */
void DigitTube_SetDisplayDigits(uint8_t digits);
/**
  * @brief  数码管初始化 (GPIO+定时器)
  */
void DigitTube_Init(void);
/**
  * @brief  数码管动态扫描函数, 在定时器中断中调用
  */
void DigitTube_Scan(void);
/**
  * @brief  显示整数值 (0~999)
  * @param  value: 待显示的整数值
  * @return 0=成功, 1=超出范围
  */
uint8_t DigitTube_DisplayInt(int32_t value);
/**
  * @brief  设置显示值 (关中断保护)
  * @param  value: 显示值, 超过999截断
  */
void DigitTube_SetValue(uint16_t value);
/**
  * @brief  清除显示
  */
void DigitTube_Clear(void);
/**
  * @brief  切换温度单位 (摄氏/华氏), 自动换算温度值
  */
void DigitTube_ToggleUnit(void);
/**
  * @brief  获取当前温度单位
  * @return 0=摄氏(°C), 1=华氏(°F)
  */
uint8_t DigitTube_GetUnit(void);
/**
  * @brief  设置指定LED状态
  * @param  led_id: LED编号, 1=Q1, 2=Q2
  * @param  state: LED状态, 0=灭, 非0=亮
  */
void DigitTube_SetLED(uint8_t led_id, uint8_t state);
/**
  * @brief  显示温度值并设置单位
  * @param  value: 温度值, 范围0~999
  * @param  unit:  温度单位, 0=摄氏, 1=华氏
  * @return 0=成功, 1=超出范围
  */
uint8_t DigitTube_DisplayTemp(int32_t value, uint8_t unit);
/**
  * @brief  清除错误显示标志
  */
void DigitTube_ClearError(void);
/**
  * @brief  查询是否处于错误显示状态
  * @return 0=正常, 1=错误
  */
uint8_t DigitTube_IsError(void);
/**
  * @brief  获取扫描计数器值
  * @return 当前扫描计数
  */
uint32_t DigitTube_GetScanCounter(void);
/**
  * @brief  重置扫描计数器
  */
void DigitTube_ResetScanCounter(void);
/**
  * @brief  显示错误代码 (3位, 强制前导零)
  * @param  code: 错误代码, 范围0~999
  */
void DigitTube_DisplayErrorCode(uint16_t code);
/**
  * @brief  清除错误代码显示模式, 恢复2位显示
  */
void DigitTube_ClearErrorCode(void);

#ifdef __cplusplus
}
#endif

#endif
