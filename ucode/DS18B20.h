/**
  ******************************************************************************
  * @file    DS18B20.h
  * @brief   DS18B20温度传感器驱动头文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    V1适配: 引脚从PA11/PA12改为PD11/PD10
  ******************************************************************************
  */

#ifndef __DS18B20_H
#define __DS18B20_H

#include "at32f403a_407.h"
#include "wk_system.h"
#include "at32f403a_407_wk_config.h"
#include <stdbool.h>

/** @brief DS18B20_1 数据引脚，默认映射到 TEMP_1_PIN */
#ifndef DS18B20_1_PIN
#define DS18B20_1_PIN    TEMP_1_PIN
/** @brief DS18B20_1 GPIO端口，默认映射到 TEMP_1_GPIO_PORT */
#define DS18B20_1_GPIO_PORT    TEMP_1_GPIO_PORT
#endif

/** @brief DS18B20_2 数据引脚，默认映射到 TEMP_2_PIN */
#ifndef DS18B20_2_PIN
#define DS18B20_2_PIN    TEMP_2_PIN
/** @brief DS18B20_2 GPIO端口，默认映射到 TEMP_2_GPIO_PORT */
#define DS18B20_2_GPIO_PORT    TEMP_2_GPIO_PORT
#endif

/** @brief DS18B20总线数量（含预留通道） */
#define  DS_TempNumber   3
/** @brief 硬件抽象层选择：1-使用雅特力HAL库，0-裸机操作 */
#define SL_HALLIB 1

/** @brief DS18B20总线1使能开关：1-启用，0-禁用 */
#define DS_1    1
/** @brief DS18B20总线2使能开关：1-启用，0-禁用 */
#define DS_2    1
/** @brief DS18B20总线3使能开关：1-启用，0-禁用 */
#define DS_3    0

/** @brief 引脚方向：输出模式 */
#define DSout  1
/** @brief 引脚方向：输入模式 */
#define DSint  0

/** @brief 输出电平：高电平 */
#define DSSetH  1
/** @brief 输出电平：低电平 */
#define DSSetL  0

#if SL_HALLIB

#if DS_1
/** @brief DS18B20_1 总线拉高 */
#define DS18B20_1_H   gpio_bits_set(DS18B20_1_GPIO_PORT, DS18B20_1_PIN)
/** @brief DS18B20_1 总线拉低 */
#define DS18B20_1_L   gpio_bits_reset(DS18B20_1_GPIO_PORT, DS18B20_1_PIN)
/** @brief DS18B20_1 总线读取电平 */
#define DS18B20_1_R   gpio_input_data_bit_read(DS18B20_1_GPIO_PORT, DS18B20_1_PIN)
#endif

#if DS_2
/** @brief DS18B20_2 总线拉高 */
#define DS18B20_2_H   gpio_bits_set(DS18B20_2_GPIO_PORT, DS18B20_2_PIN)
/** @brief DS18B20_2 总线拉低 */
#define DS18B20_2_L   gpio_bits_reset(DS18B20_2_GPIO_PORT, DS18B20_2_PIN)
/** @brief DS18B20_2 总线读取电平 */
#define DS18B20_2_R   gpio_input_data_bit_read(DS18B20_2_GPIO_PORT, DS18B20_2_PIN)
#endif

#endif

/** @brief 读ROM命令（单设备模式） */
#define DS18B20_CMD_READ_ROM     0x33
/** @brief 跳过ROM命令（广播模式） */
#define DS18B20_CMD_SKIP_ROM     0xCC
/** @brief 匹配ROM命令（指定设备） */
#define DS18B20_CMD_MATCH_ROM    0x55
/** @brief 搜索ROM命令（多设备搜索） */
#define DS18B20_CMD_SEARCH_ROM   0xF0
/** @brief 启动温度转换命令 */
#define DS18B20_CMD_CONVERT_T    0x44
/** @brief 读暂存器命令 */
#define DS18B20_CMD_READ_SCRATCH 0xBE
/** @brief 写暂存器命令 */
#define DS18B20_CMD_WRITE_SCRATCH 0x4E
/** @brief 拷贝暂存器到EEPROM命令 */
#define DS18B20_CMD_COPY_SCRATCH  0x48

/** @brief 9位分辨率，转换时间93.75ms */
#define DS18B20_RES_9BIT    0x1F
/** @brief 10位分辨率，转换时间187.5ms */
#define DS18B20_RES_10BIT   0x3F
/** @brief 11位分辨率，转换时间375ms */
#define DS18B20_RES_11BIT   0x5F
/** @brief 12位分辨率，转换时间750ms */
#define DS18B20_RES_12BIT   0x7F

/**
 * @brief DS18B20总线编号枚举
 */
typedef enum
{
    DSB1,   /**< 总线1 */
    DSB2,   /**< 总线2 */
    DSB3,   /**< 总线3（预留） */
} DSname;

/**
 * @brief 设备搜索状态枚举
 */
typedef enum {
    SEARCH_SUCCESS = 0,  /**< 搜索成功 */
    SEARCH_NO_DEVICE,    /**< 未找到设备 */
    SEARCH_ERROR         /**< 搜索错误 */
} SearchStatus;

/**
 * @brief DS18B20单设备信息结构体
 */
typedef struct {
    uint8_t addr[8];        /**< 64位ROM地址 */
    float temperature;      /**< 温度值（摄氏度） */
    uint8_t valid;          /**< 数据有效标志：1-有效，0-无效 */
} DS18B20_DeviceTypeDef;

/**
 * @brief DS18B20多设备列表结构体
 */
typedef struct {
    DS18B20_DeviceTypeDef devices[8];   /**< 设备数组，最多8个 */
    uint8_t dev_cnt;                    /**< 已发现的设备数量 */
} DS18B20_MultiDeviceTypeDef;

extern DS18B20_MultiDeviceTypeDef DSTemp[DS_TempNumber];

/**
 * @brief 温度传感器数据结构体
 */
typedef struct {
    float temp1;                /**< 通道1温度值（摄氏度） */
    float temp2;                /**< 通道2温度值（摄氏度） */
    uint8_t temp1_valid;        /**< 通道1温度有效标志 */
    uint8_t temp2_valid;        /**< 通道2温度有效标志 */
    uint8_t update_flag;        /**< 温度数据更新标志 */
    
    uint8_t sensor1_present;    /**< 传感器1在线标志 */
    uint8_t sensor2_present;    /**< 传感器2在线标志 */
    uint8_t active_sensor_cnt;  /**< 在线传感器总数 */
    
    float avg_temp;             /**< 平均温度值（摄氏度） */
    int16_t avg_display_value;  /**< 平均温度显示值（整型，已换算） */
    uint8_t avg_valid;          /**< 平均温度有效标志 */
    
    int16_t display_value1;     /**< 通道1显示值（整型，已换算） */
    int16_t display_value2;     /**< 通道2显示值（整型，已换算） */
    uint8_t display_unit;       /**< 显示单位：0-摄氏度，1-华氏度 */
    uint8_t display_valid;      /**< 显示数据有效标志 */
    uint8_t display_update_flag;/**< 显示数据更新标志 */
    
    float set_temp1;            /**< 通道1设定温度（摄氏度） */
    float set_temp2;            /**< 通道2设定温度（摄氏度） */
    int16_t set_display_value1; /**< 通道1设定温度显示值 */
    int16_t set_display_value2; /**< 通道2设定温度显示值 */
    uint8_t set_temp_valid;     /**< 设定温度有效标志 */
    uint8_t set_temp_update_flag;/**< 设定温度更新标志 */
} TempSensorData_t;

/**
 * @brief 数字传感器数据结构体
 */
typedef struct {
    uint8_t sensor1_state;      /**< 数字传感器1状态 */
    uint8_t sensor2_state;      /**< 数字传感器2状态 */
    uint8_t sensor3_state;      /**< 数字传感器3状态 */
    uint8_t update_flag;        /**< 数据更新标志 */
} DigitalSensorData_t;

/**
 * @brief 全局传感器数据结构体
 */
typedef struct {
    TempSensorData_t temperature;   /**< 温度传感器数据 */
    DigitalSensorData_t digital_sensor; /**< 数字传感器数据 */
    uint32_t last_update_time;      /**< 上次更新时间戳 */
} GlobalSensorData_t;

extern GlobalSensorData_t g_sensor_data;

extern uint8_t DS18B20_Init(DS18B20_MultiDeviceTypeDef *dev_list[]);
extern uint8_t DS18B20_UpdateAllTemp(DS18B20_MultiDeviceTypeDef *dev_list[]);
extern float DS18B20_GetValidTemp(void);
extern int16_t DS18B20_GetValidDisplayValue(void);
extern uint8_t DS18B20_UpdateGlobalData(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list);
extern uint8_t DS18B20_UpdateDisplayData(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list);
extern void DS18B20_SetDisplayUnit(uint8_t unit);
extern uint8_t DS18B20_SetTargetTemp(uint8_t channel, float temp);
extern float DS18B20_GetTargetTemp(uint8_t channel);
extern void DS18B20_UpdateSetTempDisplay(uint8_t channel);
extern void DigitalSensor_UpdateGlobalData(void);
extern void GlobalSensorData_Init(void);
extern uint8_t DS18B20_MeasureAll(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list);
extern uint8_t DS18B20_MeasureSingle(u8 DSnum, DS18B20_DeviceTypeDef *dev);

/**
 * @brief 非阻塞采集状态机状态枚举
 */
typedef enum {
    DS_NB_IDLE,        /**< 空闲状态，可启动新转换 */
    DS_NB_CONVERTING,  /**< 转换中，等待转换完成 */
    DS_NB_READ         /**< 读取结果状态 */
} DS18B20_NB_State_t;

/** @brief 非阻塞模式温度转换等待时间（毫秒） */
#define DS18B20_CONVERT_MS  200
/** @brief 非阻塞模式温度转换等待时间（系统滴答数，10ms/滴答） */
#define DS18B20_CONVERT_TICKS  (DS18B20_CONVERT_MS * 10u)

extern uint8_t DS18B20_SetResolution(u8 DSnum, uint8_t resolution);
extern uint8_t DS18B20_NonBlockingProcess(DS18B20_MultiDeviceTypeDef *dev_list[]);
extern DS18B20_NB_State_t DS18B20_GetNBState(void);

#endif
