/**
  ******************************************************************************
  * @file    DS18B20.c
  * @brief   DS18B20温度传感器驱动实现文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    功能说明:
  *          1. DS18B20温度传感器驱动 (1-Wire单总线协议)
  *          2. 支持多设备总线搜索和温度测量
  *          3. 支持摄氏度/华氏度显示转换
  *          4. 全局传感器数据管理
  *          5. V1适配: 引脚从PA11/PA12改为PD11/PD10
  ******************************************************************************
  */

#include "DS18B20.h"
#include "sensor_input.h"
#include <string.h>

#define DS18B20_ROM_LEN     8
#define DS18B20_FAMILY_CODE 0x28

static uint8_t rom_addr[8] = {0};

DS18B20_MultiDeviceTypeDef DSTemp[DS_TempNumber];

GlobalSensorData_t g_sensor_data = {0};

void GlobalSensorData_Init(void)
{
    memset(&g_sensor_data, 0, sizeof(GlobalSensorData_t));
    g_sensor_data.temperature.temp1 = 0.0f;
    g_sensor_data.temperature.temp2 = 0.0f;
    g_sensor_data.temperature.temp1_valid = 0;
    g_sensor_data.temperature.temp2_valid = 0;
    g_sensor_data.temperature.update_flag = 0;
    g_sensor_data.temperature.sensor1_present = 0;
    g_sensor_data.temperature.sensor2_present = 0;
    g_sensor_data.temperature.active_sensor_cnt = 0;
    g_sensor_data.temperature.avg_temp = 0.0f;
    g_sensor_data.temperature.avg_display_value = 0;
    g_sensor_data.temperature.avg_valid = 0;
    g_sensor_data.temperature.display_value1 = 0;
    g_sensor_data.temperature.display_value2 = 0;
    g_sensor_data.temperature.display_unit = 0;
    g_sensor_data.temperature.display_valid = 0;
    g_sensor_data.temperature.display_update_flag = 0;
    g_sensor_data.temperature.set_temp1 = 25.0f;
    g_sensor_data.temperature.set_temp2 = 25.0f;
    g_sensor_data.temperature.set_display_value1 = 25;
    g_sensor_data.temperature.set_display_value2 = 25;
    g_sensor_data.temperature.set_temp_valid = 1;
    g_sensor_data.temperature.set_temp_update_flag = 0;
    g_sensor_data.digital_sensor.sensor1_state = 0;
    g_sensor_data.digital_sensor.sensor2_state = 0;
    g_sensor_data.digital_sensor.sensor3_state = 0;
    g_sensor_data.digital_sensor.update_flag = 0;
    g_sensor_data.last_update_time = 0;
}

uint8_t DS18B20_UpdateAllTemp(DS18B20_MultiDeviceTypeDef *dev_list[])
{
    uint8_t ret = 0;
    uint8_t valid_cnt = 0;
    float temp_sum = 0.0f;
    int16_t avg_display = 0;

    if (g_sensor_data.temperature.active_sensor_cnt == 0) {
        g_sensor_data.temperature.avg_valid = 0;
        return 1;
    }

#if DS_1
    if (g_sensor_data.temperature.sensor1_present) {
        ret = DS18B20_UpdateGlobalData(DSB1, dev_list[DSB1]);
        if (ret == 0 && g_sensor_data.temperature.temp1_valid) {
            temp_sum += g_sensor_data.temperature.temp1;
            valid_cnt++;
        }
    }
#endif

#if DS_2
    if (g_sensor_data.temperature.sensor2_present) {
        ret = DS18B20_UpdateGlobalData(DSB2, dev_list[DSB2]);
        if (ret == 0 && g_sensor_data.temperature.temp2_valid) {
            temp_sum += g_sensor_data.temperature.temp2;
            valid_cnt++;
        }
    }
#endif

    if (valid_cnt > 0) {
        g_sensor_data.temperature.avg_temp = temp_sum / valid_cnt;
        g_sensor_data.temperature.avg_valid = 1;

        if (g_sensor_data.temperature.display_unit == 0) {
            avg_display = (int16_t)(g_sensor_data.temperature.avg_temp + 0.5f);
        } else {
            avg_display = (int16_t)((g_sensor_data.temperature.avg_temp * 9 / 5 + 32) + 0.5f);
        }

        if (avg_display > 99) avg_display = 99;
        if (avg_display < -9) avg_display = -9;

        g_sensor_data.temperature.avg_display_value = avg_display;
        g_sensor_data.temperature.display_update_flag = 1;
    } else {
        g_sensor_data.temperature.avg_valid = 0;
    }

    return (valid_cnt > 0) ? 0 : 1;
}

float DS18B20_GetValidTemp(void)
{
    if (g_sensor_data.temperature.avg_valid) {
        return g_sensor_data.temperature.avg_temp;
    }

    if (g_sensor_data.temperature.temp1_valid) {
        return g_sensor_data.temperature.temp1;
    }

    if (g_sensor_data.temperature.temp2_valid) {
        return g_sensor_data.temperature.temp2;
    }

    return 0.0f;
}

int16_t DS18B20_GetValidDisplayValue(void)
{
    if (g_sensor_data.temperature.avg_valid) {
        return g_sensor_data.temperature.avg_display_value;
    }

    if (g_sensor_data.temperature.temp1_valid) {
        return g_sensor_data.temperature.display_value1;
    }

    if (g_sensor_data.temperature.temp2_valid) {
        return g_sensor_data.temperature.display_value2;
    }

    return 0;
}

uint8_t DS18B20_UpdateGlobalData(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    uint8_t ret = 0;

    if (dev_list == NULL || dev_list->dev_cnt == 0) {
        return 1;
    }

    ret = DS18B20_MeasureAll(DSnum, dev_list);
    if (ret == 0) {
        if (DSnum == DSB1 && dev_list->dev_cnt > 0) {
            g_sensor_data.temperature.temp1 = dev_list->devices[0].temperature;
            g_sensor_data.temperature.temp1_valid = dev_list->devices[0].valid;
            g_sensor_data.temperature.update_flag = 1;
        } else if (DSnum == DSB2 && dev_list->dev_cnt > 0) {
            g_sensor_data.temperature.temp2 = dev_list->devices[0].temperature;
            g_sensor_data.temperature.temp2_valid = dev_list->devices[0].valid;
            g_sensor_data.temperature.update_flag = 1;
        }
        g_sensor_data.last_update_time++;
    }

    return ret;
}

void DigitalSensor_UpdateGlobalData(void)
{
    g_sensor_data.digital_sensor.sensor1_state = SensorInput_GetState(SENSOR_IN_1);
    g_sensor_data.digital_sensor.sensor2_state = SensorInput_GetState(SENSOR_IN_2);
    g_sensor_data.digital_sensor.sensor3_state = SensorInput_GetState(SENSOR_IN_3);
    g_sensor_data.digital_sensor.update_flag = 1;
    g_sensor_data.last_update_time++;
}

uint8_t DS18B20_UpdateDisplayData(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    uint8_t ret = 0;
    float temp_celsius = 0.0f;
    int16_t display_value = 0;

    if (dev_list == NULL || dev_list->dev_cnt == 0) {
        return 1;
    }

    ret = DS18B20_MeasureAll(DSnum, dev_list);
    if (ret == 0) {
        if (dev_list->dev_cnt > 0 && dev_list->devices[0].valid) {
            temp_celsius = dev_list->devices[0].temperature;

            if (g_sensor_data.temperature.display_unit == 0) {
                display_value = (int16_t)(temp_celsius + 0.5f);
            } else {
                display_value = (int16_t)((temp_celsius * 9 / 5 + 32) + 0.5f);
            }

            if (display_value > 99) display_value = 99;
            if (display_value < -9) display_value = -9;

            if (DSnum == DSB1) {
                g_sensor_data.temperature.display_value1 = display_value;
                g_sensor_data.temperature.display_valid = 1;
                g_sensor_data.temperature.display_update_flag = 1;
            } else if (DSnum == DSB2) {
                g_sensor_data.temperature.display_value2 = display_value;
                g_sensor_data.temperature.display_valid = 1;
                g_sensor_data.temperature.display_update_flag = 1;
            }
            g_sensor_data.last_update_time++;
        }
    }

    return ret;
}

void DS18B20_SetDisplayUnit(uint8_t unit)
{
    float temp_celsius = 0.0f;
    int16_t display_value = 0;

    g_sensor_data.temperature.display_unit = unit;

    if (g_sensor_data.temperature.temp1_valid) {
        temp_celsius = g_sensor_data.temperature.temp1;
        if (unit == 0) {
            display_value = (int16_t)(temp_celsius + 0.5f);
        } else {
            display_value = (int16_t)((temp_celsius * 9 / 5 + 32) + 0.5f);
        }
        if (display_value > 99) display_value = 99;
        if (display_value < -9) display_value = -9;
        g_sensor_data.temperature.display_value1 = display_value;
    }

    if (g_sensor_data.temperature.temp2_valid) {
        temp_celsius = g_sensor_data.temperature.temp2;
        if (unit == 0) {
            display_value = (int16_t)(temp_celsius + 0.5f);
        } else {
            display_value = (int16_t)((temp_celsius * 9 / 5 + 32) + 0.5f);
        }
        if (display_value > 99) display_value = 99;
        if (display_value < -9) display_value = -9;
        g_sensor_data.temperature.display_value2 = display_value;
    }

    g_sensor_data.temperature.display_update_flag = 1;
    g_sensor_data.last_update_time++;
}

uint8_t DS18B20_SetTargetTemp(uint8_t channel, float temp)
{
    if (channel != 1 && channel != 2) {
        return 1;
    }

    if (channel == 1) {
        g_sensor_data.temperature.set_temp1 = temp;
        DS18B20_UpdateSetTempDisplay(1);
    } else {
        g_sensor_data.temperature.set_temp2 = temp;
        DS18B20_UpdateSetTempDisplay(2);
    }

    g_sensor_data.temperature.set_temp_valid = 1;
    g_sensor_data.temperature.set_temp_update_flag = 1;

    return 0;
}

float DS18B20_GetTargetTemp(uint8_t channel)
{
    if (channel == 1) {
        return g_sensor_data.temperature.set_temp1;
    } else if (channel == 2) {
        return g_sensor_data.temperature.set_temp2;
    }

    return 0.0f;
}

void DS18B20_UpdateSetTempDisplay(uint8_t channel)
{
    float set_temp = 0.0f;
    int16_t display_value = 0;

    if (channel == 1) {
        set_temp = g_sensor_data.temperature.set_temp1;
    } else if (channel == 2) {
        set_temp = g_sensor_data.temperature.set_temp2;
    } else {
        return;
    }

    if (g_sensor_data.temperature.display_unit == 0) {
        display_value = (int16_t)(set_temp + 0.5f);
    } else {
        display_value = (int16_t)((set_temp * 9 / 5 + 32) + 0.5f);
    }

    if (display_value > 99) display_value = 99;
    if (display_value < 0) display_value = 0;

    if (channel == 1) {
        g_sensor_data.temperature.set_display_value1 = display_value;
    } else {
        g_sensor_data.temperature.set_display_value2 = display_value;
    }

    g_sensor_data.temperature.set_temp_update_flag = 1;
    g_sensor_data.last_update_time++;
}

void DSdelay_1us(void)
{
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
}

void DS_Delay_us(u16 num)
{
    uint16_t i;
    for(i = 0; i < num; i++)
    {
#if SL_HALLIB
        wk_delay_us(1);
#else
        DSdelay_1us();
#endif
    }
}

void DS_Delay_ms(u16 num)
{
    uint16_t i;
    for(i = 0; i < num; i++)
    {
#if SL_HALLIB
        wk_delay_ms(1);
#else
        DS_Delay_us(1000);
#endif
    }
}

static uint8_t DS18B20_CalcCRC8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;
    uint8_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

#if SL_HALLIB

#if DS_1
static void DS18B20_1_SetOutput(void)
{
    gpio_init_type gpio_init_struct;

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = DS18B20_1_PIN;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(DS18B20_1_GPIO_PORT, &gpio_init_struct);
}

static void DS18B20_1_SetInput(void)
{
    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_pins = DS18B20_1_PIN;
    gpio_init(DS18B20_1_GPIO_PORT, &gpio_init_struct);
}
#endif

#if DS_2
static void DS18B20_2_SetOutput(void)
{
    gpio_init_type gpio_init_struct;

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = DS18B20_2_PIN;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(DS18B20_2_GPIO_PORT, &gpio_init_struct);
}

static void DS18B20_2_SetInput(void)
{
    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_pins = DS18B20_2_PIN;
    gpio_init(DS18B20_2_GPIO_PORT, &gpio_init_struct);
}
#endif

#endif

void DSIOsetInOut(u8 DSnum, bool Inout)
{
    switch(DSnum)
    {
#if DS_1
        case DSB1:
            if(Inout) {
                DS18B20_1_SetOutput();
            } else {
                DS18B20_1_SetInput();
            }
            break;
#endif
#if DS_2
        case DSB2:
            if(Inout) {
                DS18B20_2_SetOutput();
            } else {
                DS18B20_2_SetInput();
            }
            break;
#endif
        default:
            break;
    }
}

void DSout_set_HL(u8 DSnum, bool OutHL)
{
    switch(DSnum)
    {
#if DS_1
        case DSB1:
            if(OutHL) {
                DS18B20_1_H;
            } else {
                DS18B20_1_L;
            }
            break;
#endif
#if DS_2
        case DSB2:
            if(OutHL) {
                DS18B20_2_H;
            } else {
                DS18B20_2_L;
            }
            break;
#endif
        default:
            break;
    }
}

bool DSIn_read(u8 DSnum)
{
    bool Rbit = 0;
    switch(DSnum)
    {
#if DS_1
        case DSB1:
            Rbit = DS18B20_1_R;
            break;
#endif
#if DS_2
        case DSB2:
            Rbit = DS18B20_2_R;
            break;
#endif
        default:
            break;
    }
    return Rbit;
}

static uint8_t DS18B20_Reset(u8 DSnum)
{
    uint8_t ack_flag = 0;

    DSIOsetInOut(DSnum, DSout);
    DSout_set_HL(DSnum, DSSetL);
    DS_Delay_us(480);

    DSIOsetInOut(DSnum, DSint);
    DS_Delay_us(60);

    if (!DSIn_read(DSnum)) {
        ack_flag = 1;
    }
    DS_Delay_us(420);
    return ack_flag;
}

static void DS18B20_WriteBit(u8 DSnum, uint8_t bit)
{
    DSIOsetInOut(DSnum, DSout);
    DSout_set_HL(DSnum, DSSetL);
    DS_Delay_us(2);
    if (bit) {
        DSIOsetInOut(DSnum, DSint);
        DS_Delay_us(70);
    } else {
        DS_Delay_us(70);
        DSIOsetInOut(DSnum, DSint);
        DS_Delay_us(2);
    }
}

static uint8_t DS18B20_ReadBit(u8 DSnum)
{
    uint8_t bit = 0;

    DSIOsetInOut(DSnum, DSout);
    DSout_set_HL(DSnum, DSSetL);
    DS_Delay_us(2);
    DSIOsetInOut(DSnum, DSint);
    DS_Delay_us(3);
    if (DSIn_read(DSnum)) {
        bit = 1;
    }
    DS_Delay_us(60);
    return bit;
}

static void DS18B20_WriteByte(u8 DSnum, uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        DS18B20_WriteBit(DSnum, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t DS18B20_ReadByte(u8 DSnum)
{
    uint8_t i, byte = 0;
    for (i = 0; i < 8; i++) {
        byte >>= 1;
        if (DS18B20_ReadBit(DSnum)) {
            byte |= 0x80;
        }
    }
    return byte;
}

uint8_t DS18B20_ReadROMSingle(u8 DSnum, DS18B20_DeviceTypeDef *dev)
{
    uint8_t romcode[8] = {0};
    uint8_t i;

    if (!DS18B20_Reset(DSnum)) {
        dev->valid = 0;
        return 1;
    }

    DS18B20_WriteByte(DSnum, DS18B20_CMD_READ_ROM);

    for (i = 0; i < 8; i++) {
        romcode[i] = DS18B20_ReadByte(DSnum);
    }

    if (DS18B20_CalcCRC8(romcode, 7) != romcode[7]) {
        return 1;
    } else {
        for(i = 0; i < 8; i++)
            *(dev->addr + i) = romcode[i];

        dev->valid = 1;
    }

    return 0;
}

uint8_t DS18B20_ReadRom(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    if(DS18B20_ReadROMSingle(DSnum, &dev_list->devices[0]))
    {
        return 1;
    }

    dev_list->dev_cnt = 1;
    return 0;
}

uint8_t DS18B20_SearchRom(u8 DSnum, uint8_t *addr, uint8_t last_discrepancy)
{
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t id_bit = 0;
    uint8_t cmp_id_bit = 0;
    uint8_t crc8 = 0;

    memset(addr, 0, 8);

    if (!DS18B20_Reset(DSnum)) return 0;
    DS18B20_WriteByte(DSnum, DS18B20_CMD_SEARCH_ROM);

    do {
        id_bit = DS18B20_ReadBit(DSnum);
        cmp_id_bit = DS18B20_ReadBit(DSnum);

        if (id_bit && cmp_id_bit) {
            return 0;
        } else if (!id_bit && !cmp_id_bit) {
            if (id_bit_number == last_discrepancy) {
                id_bit = 1;
            } else if (id_bit_number > last_discrepancy) {
                id_bit = 0;
                last_zero = id_bit_number;
            } else {
                id_bit = (rom_addr[rom_byte_number] >> (id_bit_number - 1)) & 0x01;

                if (!id_bit) last_zero = id_bit_number;
            }
        }

        if (id_bit) {
            addr[rom_byte_number] |= (1 << (id_bit_number - 1));
        } else {
            addr[rom_byte_number] &= ~(1 << (id_bit_number - 1));
        }

        DSIOsetInOut(DSnum, DSout);
        DS18B20_WriteBit(DSnum, id_bit);

        id_bit_number++;
        if (id_bit_number > 8) {
            id_bit_number = 1;
            rom_byte_number++;
            if (rom_byte_number >= 8) break;
        }
    } while (1);

    crc8 = DS18B20_CalcCRC8(addr, 7);

    if (crc8 != addr[7]) return 1;
    memcpy(rom_addr, addr, 8);

    return last_zero;
}

uint8_t DS18B20_SearchDevices(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    uint8_t last_discrepancy = 0;
    uint8_t dev_idx = 0;

    if (dev_list == NULL) {
        return 0;
    }

    memset(dev_list, 0, sizeof(DS18B20_MultiDeviceTypeDef));

    do {
        last_discrepancy = DS18B20_SearchRom(DSnum, dev_list->devices[dev_idx].addr, last_discrepancy);
        if(dev_list->devices[dev_idx].addr[0] == 0x28)
        {
            dev_list->devices[dev_idx].valid = 1;
            dev_idx++;
        }
    } while (last_discrepancy != 0 && dev_idx < 8);

    dev_list->dev_cnt = dev_idx;
    return dev_idx;
}

uint8_t DS18B20_MeasureSingle(u8 DSnum, DS18B20_DeviceTypeDef *dev)
{
    uint8_t scratchpad[9] = {0};
    uint16_t temp_raw = 0;
    uint8_t i;

    if (dev == NULL || !dev->valid) {
        return 1;
    }

    if (!DS18B20_Reset(DSnum)) {
        dev->valid = 0;
        return 1;
    }

    DS18B20_WriteByte(DSnum, DS18B20_CMD_MATCH_ROM);
    for (i = 0; i < 8; i++) {
        DS18B20_WriteByte(DSnum, dev->addr[i]);
    }

    DS18B20_WriteByte(DSnum, DS18B20_CMD_CONVERT_T);
    DS_Delay_ms(750);

    if (!DS18B20_Reset(DSnum)) {
        dev->valid = 0;
        return 1;
    }
    DS18B20_WriteByte(DSnum, DS18B20_CMD_MATCH_ROM);
    for (i = 0; i < 8; i++) {
        DS18B20_WriteByte(DSnum, dev->addr[i]);
    }
    DS18B20_WriteByte(DSnum, DS18B20_CMD_READ_SCRATCH);

    for (i = 0; i < 9; i++) {
        scratchpad[i] = DS18B20_ReadByte(DSnum);
    }

    if (DS18B20_CalcCRC8(scratchpad, 8) != scratchpad[8]) {
        return 1;
    }

    temp_raw = (scratchpad[1] << 8) | scratchpad[0];
    if (temp_raw & 0x8000) {
        dev->temperature = (float)(~temp_raw + 1) * (-0.0625f);
    } else {
        dev->temperature = (float)temp_raw * 0.0625f;
    }

    return 0;
}

uint8_t DS18B20_MeasureAll(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    uint8_t i;
    uint8_t j;
    uint8_t scratchpad[9] = {0};
    uint16_t temp_raw = 0;

    if (dev_list == NULL || dev_list->dev_cnt == 0) {
        return 1;
    }

    if (!DS18B20_Reset(DSnum)) {
        return 1;
    }

    DS18B20_WriteByte(DSnum, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DSnum, DS18B20_CMD_CONVERT_T);
    DS_Delay_ms(750);

    for (i = 0; i < dev_list->dev_cnt; i++) {
        if (!dev_list->devices[i].valid) {
            continue;
        }

        if (!DS18B20_Reset(DSnum)) {
            dev_list->devices[i].valid = 0;
            continue;
        }

        DS18B20_WriteByte(DSnum, DS18B20_CMD_MATCH_ROM);
        for (j = 0; j < 8; j++) {
            DS18B20_WriteByte(DSnum, dev_list->devices[i].addr[j]);
        }
        DS18B20_WriteByte(DSnum, DS18B20_CMD_READ_SCRATCH);

        for (j = 0; j < 9; j++) {
            scratchpad[j] = DS18B20_ReadByte(DSnum);
        }

        if (DS18B20_CalcCRC8(scratchpad, 8) != scratchpad[8]) {
            dev_list->devices[i].valid = 0;
            continue;
        }

        temp_raw = (scratchpad[1] << 8) | scratchpad[0];
        if (temp_raw & 0x8000) {
            dev_list->devices[i].temperature = (float)(~temp_raw + 1) * (-0.0625f);
        } else {
            dev_list->devices[i].temperature = (float)temp_raw * 0.0625f;
        }
    }
    return 0;
}

void DS18B20_gpio_Init(void)
{
#if DS_1
    DSIOsetInOut(DSB1, DSint);
#endif
#if DS_2
    DSIOsetInOut(DSB2, DSint);
#endif
}

uint8_t DS18B20_Init(DS18B20_MultiDeviceTypeDef *dev_list[])
{
    u8 DSbnum = 0;
    u8 cnt1 = 0;
    u8 cnt2 = 0;

    DS18B20_gpio_Init();

#if DS_1
    cnt1 = DS18B20_SearchDevices(DSB1, dev_list[DSB1]);
    g_sensor_data.temperature.sensor1_present = (cnt1 > 0) ? 1 : 0;
    DSbnum += cnt1;
#endif

#if DS_2
    cnt2 = DS18B20_SearchDevices(DSB2, dev_list[DSB2]);
    g_sensor_data.temperature.sensor2_present = (cnt2 > 0) ? 1 : 0;
    DSbnum += cnt2;
#endif

    g_sensor_data.temperature.active_sensor_cnt =
        g_sensor_data.temperature.sensor1_present +
        g_sensor_data.temperature.sensor2_present;

    return DSbnum;
}
