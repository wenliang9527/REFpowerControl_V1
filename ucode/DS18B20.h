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

#define  DS_TempNumber   3
#define SL_HALLIB 1

#define DS_1    1
#define DS_2    1
#define DS_3    0

#define DSout  1
#define DSint  0

#define DSSetH  1
#define DSSetL  0

#if SL_HALLIB

#if DS_1
#define DS18B20_1_H   gpio_bits_set(DS18B20_1_GPIO_PORT, DS18B20_1_PIN)
#define DS18B20_1_L   gpio_bits_reset(DS18B20_1_GPIO_PORT, DS18B20_1_PIN)
#define DS18B20_1_R   gpio_input_data_bit_read(DS18B20_1_GPIO_PORT, DS18B20_1_PIN)
#endif

#if DS_2
#define DS18B20_2_H   gpio_bits_set(DS18B20_2_GPIO_PORT, DS18B20_2_PIN)
#define DS18B20_2_L   gpio_bits_reset(DS18B20_2_GPIO_PORT, DS18B20_2_PIN)
#define DS18B20_2_R   gpio_input_data_bit_read(DS18B20_2_GPIO_PORT, DS18B20_2_PIN)
#endif

#endif

#define DS18B20_CMD_READ_ROM     0x33
#define DS18B20_CMD_SKIP_ROM     0xCC
#define DS18B20_CMD_MATCH_ROM    0x55
#define DS18B20_CMD_SEARCH_ROM   0xF0
#define DS18B20_CMD_CONVERT_T    0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

typedef enum
{
    DSB1,
    DSB2,
    DSB3,
} DSname;

typedef enum {
    SEARCH_SUCCESS = 0,
    SEARCH_NO_DEVICE,
    SEARCH_ERROR
} SearchStatus;

typedef struct {
    uint8_t addr[8];
    float temperature;
    uint8_t valid;
} DS18B20_DeviceTypeDef;

typedef struct {
    DS18B20_DeviceTypeDef devices[8];
    uint8_t dev_cnt;
} DS18B20_MultiDeviceTypeDef;

extern DS18B20_MultiDeviceTypeDef DSTemp[DS_TempNumber];

typedef struct {
    float temp1;
    float temp2;
    uint8_t temp1_valid;
    uint8_t temp2_valid;
    uint8_t update_flag;
    
    uint8_t sensor1_present;
    uint8_t sensor2_present;
    uint8_t active_sensor_cnt;
    
    float avg_temp;
    int16_t avg_display_value;
    uint8_t avg_valid;
    
    int16_t display_value1;
    int16_t display_value2;
    uint8_t display_unit;
    uint8_t display_valid;
    uint8_t display_update_flag;
    
    float set_temp1;
    float set_temp2;
    int16_t set_display_value1;
    int16_t set_display_value2;
    uint8_t set_temp_valid;
    uint8_t set_temp_update_flag;
} TempSensorData_t;

typedef struct {
    uint8_t sensor1_state;
    uint8_t sensor2_state;
    uint8_t sensor3_state;
    uint8_t update_flag;
} DigitalSensorData_t;

typedef struct {
    TempSensorData_t temperature;
    DigitalSensorData_t digital_sensor;
    uint32_t last_update_time;
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

#endif
