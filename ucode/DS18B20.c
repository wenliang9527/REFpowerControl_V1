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
#include "digit_tube.h"
#include <string.h>

#define DS18B20_ROM_LEN     8
#define DS18B20_FAMILY_CODE 0x28

static uint8_t rom_addr[8] = {0};

DS18B20_MultiDeviceTypeDef DSTemp[DS_TempNumber];

GlobalSensorData_t g_sensor_data = {0};

/**
 * @brief  初始化全局传感器数据结构体
 * @note   将温度、显示值、数字传感器等所有字段清零并设为默认值
 */
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

/**
 * @brief  更新所有温度传感器数据并计算平均值
 * @param  dev_list 设备列表指针数组，按总线编号索引
 * @retval 0 成功（至少一个有效通道），1 失败（无有效通道）
 * @note   根据当前显示单位计算平均温度的显示值，范围限制在 -9~99
 */
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

/**
 * @brief  获取有效温度值（摄氏度）
 * @retval 有效温度值，优先级：平均值 > 通道1 > 通道2 > 0.0f
 */
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

/**
 * @brief  获取有效显示值（整型，已按显示单位换算）
 * @retval 有效显示值，优先级：平均值 > 通道1 > 通道2 > 0
 */
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

/**
 * @brief  更新指定通道的全局温度数据
 * @param  DSnum 总线编号 (DSB1/DSB2)
 * @param  dev_list 该总线上的设备列表
 * @retval 0 成功，1 失败（参数无效或无设备）
 */
uint8_t DS18B20_UpdateGlobalData(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    if (dev_list == NULL || dev_list->dev_cnt == 0) {
        return 1;
    }

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

    return 0;
}

/**
 * @brief  更新数字传感器全局数据
 * @note   读取三路数字传感器输入状态并设置更新标志
 */
void DigitalSensor_UpdateGlobalData(void)
{
    g_sensor_data.digital_sensor.sensor1_state = SensorInput_GetState(SENSOR_IN_1);
    g_sensor_data.digital_sensor.sensor2_state = SensorInput_GetState(SENSOR_IN_2);
    g_sensor_data.digital_sensor.sensor3_state = SensorInput_GetState(SENSOR_IN_3);
    g_sensor_data.digital_sensor.update_flag = 1;
    g_sensor_data.last_update_time++;
}

/**
 * @brief  测量温度并更新显示数据
 * @param  DSnum 总线编号 (DSB1/DSB2)
 * @param  dev_list 该总线上的设备列表
 * @retval 0 成功，1 失败
 * @note   先执行温度测量，再根据显示单位换算并更新对应通道的显示值
 */
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

/**
 * @brief  设置温度显示单位
 * @param  unit 0-摄氏度，1-华氏度
 * @note   切换单位后会重新计算所有有效通道的显示值
 */
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

/**
 * @brief  设置目标温度
 * @param  channel 通道号 (1 或 2)
 * @param  temp 目标温度值（摄氏度）
 * @retval 0 成功，1 失败（通道号无效）
 */
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

/**
 * @brief  获取目标温度
 * @param  channel 通道号 (1 或 2)
 * @retval 目标温度值（摄氏度），通道号无效时返回 0.0f
 */
float DS18B20_GetTargetTemp(uint8_t channel)
{
    if (channel == 1) {
        return g_sensor_data.temperature.set_temp1;
    } else if (channel == 2) {
        return g_sensor_data.temperature.set_temp2;
    }

    return 0.0f;
}

/**
 * @brief  更新设定温度的显示值
 * @param  channel 通道号 (1 或 2)
 * @note   根据当前显示单位将设定温度转换为整型显示值
 */
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

/**
 * @brief  微秒延时（NOP空循环）
 * @note   通过执行NOP指令实现约1us延时，受主频影响
 */
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

/**
 * @brief  微秒延时函数
 * @param  num 延时微秒数
 * @note   SL_HALLIB模式下使用wk_delay_us，否则使用NOP循环
 */
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

/**
 * @brief  毫秒延时函数
 * @param  num 延时毫秒数
 * @note   SL_HALLIB模式下使用wk_delay_ms，否则循环调用DS_Delay_us(1000)
 */
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

/**
 * @brief  CRC8校验计算（Dallas 1-Wire多项式 0x8C）
 * @param  data 待校验数据指针
 * @param  len  数据长度
 * @retval CRC8校验值
 */
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
/**
 * @brief  DS18B20_1引脚设置为推挽输出模式
 */
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

/**
 * @brief  DS18B20_1引脚设置为浮空输入模式
 */
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
/**
 * @brief  DS18B20_2引脚设置为推挽输出模式
 */
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

/**
 * @brief  DS18B20_2引脚设置为浮空输入模式
 */
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

/**
 * @brief  设置指定DS18B20总线的引脚方向
 * @param  DSnum 总线编号 (DSB1/DSB2)
 * @param  Inout DSout-输出，DSint-输入
 */
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

/**
 * @brief  设置指定DS18B20总线的输出电平
 * @param  DSnum 总线编号 (DSB1/DSB2)
 * @param  OutHL DSSetH-高电平，DSSetL-低电平
 */
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

/**
 * @brief  读取指定DS18B20总线的输入电平
 * @param  DSnum 总线编号 (DSB1/DSB2)
 * @retval 1-高电平，0-低电平
 */
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

/**
 * @brief  1-Wire复位脉冲，检测设备应答
 * @param  DSnum 总线编号 (DSB1/DSB2)
 * @retval 1 设备应答（存在），0 无应答
 * @note   关闭中断保证时序精度，复位脉冲480us + 读取60us + 等待420us
 */
static uint8_t DS18B20_Reset(u8 DSnum)
{
    uint8_t ack_flag = 0;
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();

    DSIOsetInOut(DSnum, DSout);
    DSout_set_HL(DSnum, DSSetL);
    DS_Delay_us(480);

    DSIOsetInOut(DSnum, DSint);
    DS_Delay_us(60);

    if (!DSIn_read(DSnum)) {
        ack_flag = 1;
    }
    DS_Delay_us(420);

    __set_PRIMASK(primask);

    return ack_flag;
}

/**
 * @brief  1-Wire写单个bit
 * @param  DSnum 总线编号
 * @param  bit  写入的位值 (0 或 1)
 * @note   关闭中断保证时序精度，写1: 拉低2us后释放，写0: 拉低70us后释放
 */
static void DS18B20_WriteBit(u8 DSnum, uint8_t bit)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();

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

    __set_PRIMASK(primask);
}

/**
 * @brief  1-Wire读单个bit
 * @param  DSnum 总线编号
 * @retval 读取的位值 (0 或 1)
 * @note   关闭中断保证时序精度，拉低2us后释放，3us后采样
 */
static uint8_t DS18B20_ReadBit(u8 DSnum)
{
    uint8_t bit = 0;
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();

    DSIOsetInOut(DSnum, DSout);
    DSout_set_HL(DSnum, DSSetL);
    DS_Delay_us(2);
    DSIOsetInOut(DSnum, DSint);
    DS_Delay_us(3);
    if (DSIn_read(DSnum)) {
        bit = 1;
    }
    DS_Delay_us(60);

    __set_PRIMASK(primask);

    return bit;
}

/**
 * @brief  1-Wire写一个字节（LSB优先）
 * @param  DSnum 总线编号
 * @param  byte 待写入的字节
 */
static void DS18B20_WriteByte(u8 DSnum, uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        DS18B20_WriteBit(DSnum, byte & 0x01);
        byte >>= 1;
    }
}

/**
 * @brief  1-Wire读一个字节（LSB优先）
 * @param  DSnum 总线编号
 * @retval 读取的字节
 */
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

/**
 * @brief  读取单个设备的ROM编码
 * @param  DSnum 总线编号
 * @param  dev  设备结构体指针，用于存储ROM编码
 * @retval 0 成功，1 失败（无应答或CRC校验错误）
 * @note   仅适用于总线上只有一个设备的场景
 */
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

/**
 * @brief  读取设备ROM（单设备模式）
 * @param  DSnum 总线编号
 * @param  dev_list 设备列表指针
 * @retval 0 成功，1 失败
 * @note   封装DS18B20_ReadROMSingle，成功后设置dev_cnt为1
 */
uint8_t DS18B20_ReadRom(u8 DSnum, DS18B20_MultiDeviceTypeDef *dev_list)
{
    if(DS18B20_ReadROMSingle(DSnum, &dev_list->devices[0]))
    {
        return 1;
    }

    dev_list->dev_cnt = 1;
    return 0;
}

/**
 * @brief  搜索总线上的设备ROM（二叉树搜索算法）
 * @param  DSnum 总线编号
 * @param  addr  存储找到的ROM地址（8字节）
 * @param  last_discrepancy 上次搜索的分歧点位置
 * @retval 0 搜索完成，>0 下一次搜索的分歧点，1 CRC错误
 * @note   实现1-Wire二叉树搜索协议，每次调用找到一个设备
 */
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

/**
 * @brief  搜索总线上所有DS18B20设备
 * @param  DSnum 总线编号
 * @param  dev_list 设备列表指针，用于存储搜索结果
 * @retval 找到的设备数量
 * @note   使用二叉树搜索算法，最多搜索8个设备，仅识别家族码0x28的设备
 */
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

/**
 * @brief  测量单个设备温度（阻塞方式）
 * @param  DSnum 总线编号
 * @param  dev  设备结构体指针
 * @retval 0 成功，1 失败
 * @note   匹配ROM后启动转换，等待750ms后读取暂存器并计算温度
 */
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

    temp_raw = ((scratchpad[1] << 8) | scratchpad[0]) & 0xFFFC;
    if (temp_raw & 0x8000) {
        dev->temperature = (float)(~temp_raw + 1) * (-0.0625f);
    } else {
        dev->temperature = (float)temp_raw * 0.0625f;
    }

    return 0;
}

/**
 * @brief  测量总线上所有设备温度（阻塞方式）
 * @param  DSnum 总线编号
 * @param  dev_list 设备列表指针
 * @retval 0 成功，1 失败
 * @note   跳过ROM广播启动转换，等待750ms后逐个匹配ROM读取暂存器
 */
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

        temp_raw = ((scratchpad[1] << 8) | scratchpad[0]) & 0xFFFC;
        if (temp_raw & 0x8000) {
            dev_list->devices[i].temperature = (float)(~temp_raw + 1) * (-0.0625f);
        } else {
            dev_list->devices[i].temperature = (float)temp_raw * 0.0625f;
        }
    }
    return 0;
}

/**
 * @brief  DS18B20 GPIO初始化
 * @note   将所有使能的总线引脚配置为输入模式（1-Wire空闲状态）
 */
void DS18B20_gpio_Init(void)
{
#if DS_1
    DSIOsetInOut(DSB1, DSint);
#endif
#if DS_2
    DSIOsetInOut(DSB2, DSint);
#endif
}

/**
 * @brief  设置DS18B20分辨率
 * @param  DSnum 总线编号
 * @param  resolution 分辨率配置值 (DS18B20_RES_9BIT ~ DS18B20_RES_12BIT)
 * @retval 0 成功，1 失败（设备无应答）
 * @note   写入暂存器TH/TL/配置字节后拷贝到EEPROM
 */
uint8_t DS18B20_SetResolution(u8 DSnum, uint8_t resolution)
{
    if (!DS18B20_Reset(DSnum)) {
        return 1;
    }
    DS18B20_WriteByte(DSnum, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DSnum, DS18B20_CMD_WRITE_SCRATCH);
    DS18B20_WriteByte(DSnum, 0x00);
    DS18B20_WriteByte(DSnum, 0x00);
    DS18B20_WriteByte(DSnum, resolution);

    if (!DS18B20_Reset(DSnum)) {
        return 1;
    }
    DS18B20_WriteByte(DSnum, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DSnum, DS18B20_CMD_COPY_SCRATCH);
    DS_Delay_ms(10);

    return 0;
}

/**
 * @brief  DS18B20初始化（搜索设备+设置分辨率）
 * @param  dev_list 设备列表指针数组，按总线编号索引
 * @retval 找到的设备总数
 * @note   初始化GPIO、设置10位分辨率、搜索各总线设备并更新在线状态
 */
uint8_t DS18B20_Init(DS18B20_MultiDeviceTypeDef *dev_list[])
{
    u8 DSbnum = 0;
    u8 cnt1 = 0;
    u8 cnt2 = 0;

    DS18B20_gpio_Init();

#if DS_1
    DS18B20_SetResolution(DSB1, DS18B20_RES_10BIT);
#endif

#if DS_2
    DS18B20_SetResolution(DSB2, DS18B20_RES_10BIT);
#endif

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

static DS18B20_NB_State_t s_nb_state = DS_NB_IDLE;
static uint32_t s_nb_start_tick = 0;

/**
 * @brief  非阻塞模式启动温度转换
 * @param  dev_list 设备列表指针数组
 * @retval 1 至少一条总线成功启动，0 全部失败
 * @note   跳过ROM广播启动转换，不等待转换完成
 */
static uint8_t DS18B20_StartConvert(DS18B20_MultiDeviceTypeDef *dev_list[])
{
    uint8_t any_started = 0;

#if DS_1
    if (g_sensor_data.temperature.sensor1_present && dev_list[DSB1]->dev_cnt > 0) {
        if (DS18B20_Reset(DSB1)) {
            DS18B20_WriteByte(DSB1, DS18B20_CMD_SKIP_ROM);
            DS18B20_WriteByte(DSB1, DS18B20_CMD_CONVERT_T);
            any_started = 1;
        } else {
            dev_list[DSB1]->devices[0].valid = 0;
        }
    }
#endif

#if DS_2
    if (g_sensor_data.temperature.sensor2_present && dev_list[DSB2]->dev_cnt > 0) {
        if (DS18B20_Reset(DSB2)) {
            DS18B20_WriteByte(DSB2, DS18B20_CMD_SKIP_ROM);
            DS18B20_WriteByte(DSB2, DS18B20_CMD_CONVERT_T);
            any_started = 1;
        } else {
            dev_list[DSB2]->devices[0].valid = 0;
        }
    }
#endif

    return any_started;
}

/**
 * @brief  非阻塞模式读取所有转换结果
 * @param  dev_list 设备列表指针数组
 * @note   逐总线复位后读取暂存器，CRC校验通过则更新温度值
 */
static void DS18B20_ReadAllResults(DS18B20_MultiDeviceTypeDef *dev_list[])
{
    uint8_t i, j;
    uint8_t scratchpad[9] = {0};
    uint16_t temp_raw = 0;

#if DS_1
    if (g_sensor_data.temperature.sensor1_present && dev_list[DSB1]->dev_cnt > 0) {
        if (!DS18B20_Reset(DSB1)) {
            dev_list[DSB1]->devices[0].valid = 0;
        } else {
            DS18B20_WriteByte(DSB1, DS18B20_CMD_SKIP_ROM);
            DS18B20_WriteByte(DSB1, DS18B20_CMD_READ_SCRATCH);
            for (j = 0; j < 9; j++) {
                scratchpad[j] = DS18B20_ReadByte(DSB1);
            }
            if (DS18B20_CalcCRC8(scratchpad, 8) != scratchpad[8]) {
                dev_list[DSB1]->devices[0].valid = 0;
            } else {
                temp_raw = ((scratchpad[1] << 8) | scratchpad[0]) & 0xFFFC;
                if (temp_raw & 0x8000) {
                    dev_list[DSB1]->devices[0].temperature = (float)(~temp_raw + 1) * (-0.0625f);
                } else {
                    dev_list[DSB1]->devices[0].temperature = (float)temp_raw * 0.0625f;
                }
                dev_list[DSB1]->devices[0].valid = 1;
            }
        }
    }
#endif

#if DS_2
    if (g_sensor_data.temperature.sensor2_present && dev_list[DSB2]->dev_cnt > 0) {
        if (!DS18B20_Reset(DSB2)) {
            dev_list[DSB2]->devices[0].valid = 0;
        } else {
            DS18B20_WriteByte(DSB2, DS18B20_CMD_SKIP_ROM);
            DS18B20_WriteByte(DSB2, DS18B20_CMD_READ_SCRATCH);
            for (j = 0; j < 9; j++) {
                scratchpad[j] = DS18B20_ReadByte(DSB2);
            }
            if (DS18B20_CalcCRC8(scratchpad, 8) != scratchpad[8]) {
                dev_list[DSB2]->devices[0].valid = 0;
            } else {
                temp_raw = ((scratchpad[1] << 8) | scratchpad[0]) & 0xFFFC;
                if (temp_raw & 0x8000) {
                    dev_list[DSB2]->devices[0].temperature = (float)(~temp_raw + 1) * (-0.0625f);
                } else {
                    dev_list[DSB2]->devices[0].temperature = (float)temp_raw * 0.0625f;
                }
                dev_list[DSB2]->devices[0].valid = 1;
            }
        }
    }
#endif

    (void)i; (void)temp_raw; (void)scratchpad;
}

/**
 * @brief  非阻塞温度采集状态机
 * @param  dev_list 设备列表指针数组
 * @retval 1 数据已更新（完成一次采集周期），0 未完成
 * @note   状态流转：IDLE -> CONVERTING -> READ -> IDLE
 *         CONVERTING状态通过g_scan_counter计时，达到DS18B20_CONVERT_TICKS后进入READ
 */
uint8_t DS18B20_NonBlockingProcess(DS18B20_MultiDeviceTypeDef *dev_list[])
{
    uint8_t updated = 0;
    uint32_t primask;
    uint32_t elapsed;
    uint32_t now;

    switch (s_nb_state) {
        case DS_NB_IDLE:
            if (DS18B20_StartConvert(dev_list)) {
                s_nb_state = DS_NB_CONVERTING;
                primask = __get_PRIMASK();
                __disable_irq();
                s_nb_start_tick = g_scan_counter;
                __set_PRIMASK(primask);
            }
            break;

        case DS_NB_CONVERTING:
            primask = __get_PRIMASK();
            __disable_irq();
            now = g_scan_counter;
            __set_PRIMASK(primask);
            elapsed = now - s_nb_start_tick;
            if (elapsed >= DS18B20_CONVERT_TICKS) {
                s_nb_state = DS_NB_READ;
            }
            break;

        case DS_NB_READ:
            DS18B20_ReadAllResults(dev_list);
            DS18B20_UpdateAllTemp(dev_list);
            s_nb_state = DS_NB_IDLE;
            updated = 1;
            break;

        default:
            s_nb_state = DS_NB_IDLE;
            break;
    }

    return updated;
}

/**
 * @brief  获取非阻塞状态机当前状态
 * @retval 当前状态 (DS_NB_IDLE / DS_NB_CONVERTING / DS_NB_READ)
 */
DS18B20_NB_State_t DS18B20_GetNBState(void)
{
    return s_nb_state;
}
