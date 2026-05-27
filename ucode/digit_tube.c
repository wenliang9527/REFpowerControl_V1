/**
  ******************************************************************************
  * @file    digit_tube.c
  * @brief   3位6引脚数码管驱动实现文件 (V1适配版)
  * @version V1.1.0
  * @date    2026-04-24
  * @note    采用时分复用(动态扫描)技术
  *          V1适配: 引脚映射已更新
  *          DTUBE_1=PD9, DTUBE_2=PD8, DTUBE_3=PB15, DTUBE_4=PB14, DTUBE_5=PB13, DTUBE_6=PB12
  ******************************************************************************
  */

#include "digit_tube.h"
#include "DS18B20.h"

/** 数字0~9的七段码编码表, 每位对应SEG_A~SEG_G */
static const uint8_t seg_code[10] = {
    0x3F,
    0x06,
    0x5B,
    0x4F,
    0x66,
    0x6D,
    0x7D,
    0x07,
    0x7F,
    0x6F
};

/** 错误显示"Err"的段码, 分别对应3位数字的段编码 */
static const uint8_t err_code[3] = {
    0x79,
    0x50,
    0x50
};

/** 数码管6个引脚的GPIO端口映射, 索引0~5对应PIN_1~PIN_6 */
static gpio_type* const tube_gpio[6] = {
    TUBE_PIN_1_GPIO,
    TUBE_PIN_2_GPIO,
    TUBE_PIN_3_GPIO,
    TUBE_PIN_4_GPIO,
    TUBE_PIN_5_GPIO,
    TUBE_PIN_6_GPIO
};

/** 数码管6个引脚的引脚号映射, 索引0~5对应PIN_1~PIN_6 */
static const uint16_t tube_pin[6] = {
    TUBE_PIN_1_PIN,
    TUBE_PIN_2_PIN,
    TUBE_PIN_3_PIN,
    TUBE_PIN_4_PIN,
    TUBE_PIN_5_PIN,
    TUBE_PIN_6_PIN
};

/** 数码管6个引脚的外设时钟映射, 索引0~5对应PIN_1~PIN_6 */
static const crm_periph_clock_type tube_clock[6] = {
    TUBE_PIN_1_CLOCK,
    TUBE_PIN_2_CLOCK,
    TUBE_PIN_3_CLOCK,
    TUBE_PIN_4_CLOCK,
    TUBE_PIN_5_CLOCK,
    TUBE_PIN_6_CLOCK
};

/** 当前显示数值, 范围0~999 */
static volatile uint16_t g_display_value = 0;
/** 温度单位: 0=摄氏(°C), 1=华氏(°F) */
static volatile uint8_t  g_temp_unit = 0;
/** LED Q1状态指示灯: 0=灭, 1=亮 (表示摄氏) */
static volatile uint8_t  g_led_q1_state = 0;
/** LED Q2状态指示灯: 0=灭, 1=亮 (表示华氏) */
static volatile uint8_t  g_led_q2_state = 0;
/** 扫描计数器, 每次定时器中断递增, 用于监测扫描是否正常 */
volatile uint32_t g_scan_counter = 0;

/** 错误显示标志: 0=正常显示, 1=显示"Err" */
static volatile uint8_t g_display_error = 0;
/** 当前扫描位置 (0~digit*7-1), 每次中断递增实现逐段扫描 */
static volatile uint8_t scan_pos = 0;
/** 显示位数: 2=两位显示, 3=三位显示 */
static volatile uint8_t g_display_digits = 3;
/** 强制前导零标志: 0=自动消隐前导零, 1=强制显示所有位(含前导零) */
static volatile uint8_t g_force_leading_zero = 0;

/**
  * @brief  将数码管显示数据同步到全局传感器数据结构
  * @note   将当前显示值和温度单位写入g_sensor_data, 供其他模块读取
  */
void DigitTube_SyncToGlobal(void)
{
    g_sensor_data.temperature.display_value1 = (int32_t)g_display_value;
    g_sensor_data.temperature.display_unit = (uint8_t)g_temp_unit;
}

/**
  * @brief  从全局传感器数据同步到数码管显示
  * @note   关中断保护, 防止扫描中断中读取到不一致的数据; 同时将显示位数设为2位并重置扫描位置
  */
void DigitTube_SyncFromGlobal(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    g_display_value = (uint16_t)g_sensor_data.temperature.display_value1;
    g_temp_unit = g_sensor_data.temperature.display_unit;
    g_display_digits = 2;
    scan_pos = 0;
    __set_PRIMASK(primask);
}

/**
  * @brief  设置显示位数
  * @param  digits: 显示位数, 仅支持2或3
  */
void DigitTube_SetDisplayDigits(uint8_t digits)
{
    if (digits == 2 || digits == 3) {
        g_display_digits = digits;
    }
}

/**
  * @brief  获取当前显示值
  * @return 当前显示数值, 范围0~999
  */
uint16_t DigitTube_GetValue(void)
{
    return (uint16_t)g_display_value;
}

/**
  * @brief  设置LED Q1/Q2状态
  * @param  q1: LED Q1状态, 0=灭, 1=亮
  * @param  q2: LED Q2状态, 0=灭, 1=亮
  */
void DigitTube_SetLEDState(uint8_t q1, uint8_t q2)
{
    g_led_q1_state = q1;
    g_led_q2_state = q2;
}

/**
  * @brief  将GPIO引脚位掩码转换为引脚索引号
  * @param  pin: GPIO引脚位掩码 (如GPIO_PINS_9 = 0x0200)
  * @return 引脚索引号 (0~15), 对应引脚在端口中的位位置
  */
static uint8_t pin_to_index(uint16_t pin)
{
    uint8_t idx = 0;
    while (pin > 1u) { pin >>= 1u; idx++; }
    return idx;
}

/**
  * @brief  关闭所有数码管引脚
  * @note   将6个引脚全部设为模拟输入模式, 使所有段熄灭
  */
static void all_pins_off(void)
{
    uint8_t i;
    for (i = 0; i < 6; i++) {
        uint8_t idx = pin_to_index(tube_pin[i]);
        if (idx < 8u) {
            tube_gpio[i]->cfglr &= ~(0x0Fu << (idx * 4u));
        } else {
            tube_gpio[i]->cfghr &= ~(0x0Fu << ((idx - 8u) * 4u));
        }
    }
}

/**
  * @brief  点亮指定段, Charlieplexing驱动核心函数
  * @param  high_pin: 高电平引脚编号 (1~6), 对应PIN_1~PIN_6
  * @param  low_pin:  低电平引脚编号 (1~6), 对应PIN_1~PIN_6
  * @note   先关闭所有引脚, 再将高引脚设为推挽输出高电平、低引脚设为推挽输出低电平,
  *         形成电流通路点亮对应LED段; 引脚号无效时仅关闭所有引脚
  */
static void light_seg(uint8_t high_pin, uint8_t low_pin)
{
    uint8_t hp, lp;
    uint8_t idx;

    if (high_pin < 1 || high_pin > 6 || low_pin < 1 || low_pin > 6) {
        all_pins_off();
        return;
    }

    hp = high_pin - 1;
    lp = low_pin - 1;

    all_pins_off();

    idx = pin_to_index(tube_pin[hp]);
    if (idx < 8u) {
        tube_gpio[hp]->cfglr = (tube_gpio[hp]->cfglr & ~(0x0Fu << (idx * 4u)))
                              | (0x02u << (idx * 4u));
    } else {
        tube_gpio[hp]->cfghr = (tube_gpio[hp]->cfghr & ~(0x0Fu << ((idx - 8u) * 4u)))
                              | (0x02u << ((idx - 8u) * 4u));
    }
    tube_gpio[hp]->scr = tube_pin[hp];

    idx = pin_to_index(tube_pin[lp]);
    if (idx < 8u) {
        tube_gpio[lp]->cfglr = (tube_gpio[lp]->cfglr & ~(0x0Fu << (idx * 4u)))
                              | (0x02u << (idx * 4u));
    } else {
        tube_gpio[lp]->cfghr = (tube_gpio[lp]->cfghr & ~(0x0Fu << ((idx - 8u) * 4u)))
                              | (0x02u << ((idx - 8u) * 4u));
    }
    tube_gpio[lp]->clr = tube_pin[lp];
}

/**
  * @brief  数码管GPIO初始化
  * @note   使能6个引脚的外设时钟, 配置为推挽输出模式, 最后关闭所有引脚
  */
static void tube_gpio_init(void)
{
    gpio_init_type gpio_init_struct;
    uint8_t i;

    for (i = 0; i < 6; i++) {
        crm_periph_clock_enable(tube_clock[i], TRUE);
    }

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

    for (i = 0; i < 6; i++) {
        gpio_init_struct.gpio_pins = tube_pin[i];
        gpio_init(tube_gpio[i], &gpio_init_struct);
    }

    all_pins_off();
}

/**
  * @brief  数码管扫描定时器初始化
  * @note   配置TMR3为向上计数模式, 周期和分频由TUBE_SCAN_PERIOD/TUBE_SCAN_DIV决定,
  *         使能溢出中断并启动定时器
  */
static void tube_timer_init(void)
{
    crm_periph_clock_enable(TUBE_SCAN_TIMER_CLOCK, TRUE);
    
    tmr_base_init(TUBE_SCAN_TIMER, TUBE_SCAN_PERIOD - 1, TUBE_SCAN_DIV - 1);
    tmr_cnt_dir_set(TUBE_SCAN_TIMER, TMR_COUNT_UP);
    
    tmr_interrupt_enable(TUBE_SCAN_TIMER, TMR_OVF_INT, TRUE);
    
    tmr_counter_enable(TUBE_SCAN_TIMER, TRUE);
}

/**
  * @brief  根据数字位和段号获取Charlieplexing对应的高低引脚编号
  * @param  digit: 数字位索引 (0~2), 对应第1~3位数码管
  * @param  seg:   段索引 (0~6), 对应SEG_A~SEG_G
  * @param  high:  输出高电平引脚编号 (1~6), 0表示无效
  * @param  low:   输出低电平引脚编号 (1~6), 0表示无效
  * @note   3位6引脚Charlieplexing引脚映射表, 每位7段共21种组合
  */
static void get_seg_pins(uint8_t digit, uint8_t seg, uint8_t *high, uint8_t *low)
{
    *high = 0;
    *low = 0;
    
    switch(digit) {
        case 0:
            switch(seg) {
                case 0: *high = 1; *low = 2; break;
                case 1: *high = 1; *low = 3; break;
                case 2: *high = 1; *low = 4; break;
                case 3: *high = 1; *low = 5; break;
                case 4: *high = 1; *low = 6; break;
                case 5: *high = 2; *low = 1; break;
                case 6: *high = 2; *low = 3; break;
            }
            break;
        case 1:
            switch(seg) {
                case 0: *high = 2; *low = 4; break;
                case 1: *high = 2; *low = 5; break;
                case 2: *high = 2; *low = 6; break;
                case 3: *high = 3; *low = 1; break;
                case 4: *high = 3; *low = 2; break;
                case 5: *high = 3; *low = 4; break;
                case 6: *high = 3; *low = 5; break;
            }
            break;
        case 2:
            switch(seg) {
                case 0: *high = 3; *low = 6; break;
                case 1: *high = 4; *low = 1; break;
                case 2: *high = 4; *low = 2; break;
                case 3: *high = 4; *low = 3; break;
                case 4: *high = 4; *low = 5; break;
                case 5: *high = 4; *low = 6; break;
                case 6: *high = 5; *low = 1; break;
            }
            break;
    }
}

/**
  * @brief  数码管初始化
  * @note   依次调用GPIO初始化和定时器初始化, 启动动态扫描
  */
void DigitTube_Init(void)
{
    tube_gpio_init();
    tube_timer_init();
}

/**
  * @brief  数码管动态扫描函数, 在定时器中断中调用
  * @note   逐段逐位扫描, 每次中断点亮一个段; 扫描完所有段后点亮LED指示灯;
  *         支持错误显示("Err")、2位/3位显示、前导零消隐
  */
void DigitTube_Scan(void)
{
    uint8_t digit, seg;
    uint8_t num, code;
    uint8_t high_pin, low_pin;
    uint16_t max_scan_pos;
    
    g_scan_counter++;
    
    max_scan_pos = (uint16_t)(g_display_digits * 7);
    
    if (scan_pos >= max_scan_pos) {
        if (g_led_q1_state) {
            light_seg(5, 2);
        } else if (g_led_q2_state) {
            light_seg(5, 3);
        } else {
            all_pins_off();
        }
        scan_pos = 0;
        return;
    }
    
    digit = scan_pos / 7;
    seg = scan_pos % 7;
    
    if (g_display_error) {
        code = err_code[digit];
        if (code & (1 << seg)) {
            get_seg_pins(digit, seg, &high_pin, &low_pin);
            if (high_pin > 0 && low_pin > 0) {
                light_seg(high_pin, low_pin);
            } else {
                all_pins_off();
            }
        } else {
            all_pins_off();
        }
        scan_pos++;
        return;
    }
    
    if (g_display_digits == 2) {
        switch(digit) {
            case 0: num = g_display_value / 10; break;
            case 1: num = g_display_value % 10; break;
            default: num = 0; break;
        }
        
        if (!g_force_leading_zero && (num == 0 && digit == 0 && g_display_value < 10)) {
            all_pins_off();
        } else {
            code = seg_code[num];
            if (code & (1 << seg)) {
                get_seg_pins(digit, seg, &high_pin, &low_pin);
                if (high_pin > 0 && low_pin > 0) {
                    light_seg(high_pin, low_pin);
                } else {
                    all_pins_off();
                }
            } else {
                all_pins_off();
            }
        }
    } else {
        switch(digit) {
            case 0: num = g_display_value / 100; break;
            case 1: num = (g_display_value % 100) / 10; break;
            case 2: num = g_display_value % 10; break;
            default: num = 0;
        }
        
        if (!g_force_leading_zero &&
            ((num == 0 && digit == 0 && g_display_value < 100) ||
             (num == 0 && digit == 1 && g_display_value < 10))) {
            all_pins_off();
        } else {
            code = seg_code[num];
            if (code & (1 << seg)) {
                get_seg_pins(digit, seg, &high_pin, &low_pin);
                if (high_pin > 0 && low_pin > 0) {
                    light_seg(high_pin, low_pin);
                } else {
                    all_pins_off();
                }
            } else {
                all_pins_off();
            }
        }
    }
    
    scan_pos++;
}

/**
  * @brief  显示整数值
  * @param  value: 待显示的整数值, 范围0~999
  * @return 0=成功, 1=超出范围(显示"Err")
  * @note   关中断保护, 防止扫描中断中读取到不一致的数据
  */
uint8_t DigitTube_DisplayInt(int32_t value)
{
    uint32_t primask;
    if (value < 0 || value > 999) {
        g_display_error = 1;
        return 1;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    g_display_error = 0;
    g_display_value = (uint16_t)value;
    __set_PRIMASK(primask);
    return 0;
}

/**
  * @brief  设置显示值
  * @param  value: 显示值, 超过999时自动截断为999
  * @note   关中断保护, 同时清除错误显示标志
  */
void DigitTube_SetValue(uint16_t value)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    g_display_error = 0;
    g_display_value = value > 999 ? 999 : value;
    __set_PRIMASK(primask);
}

/**
  * @brief  清除显示
  * @note   关中断保护, 将显示值清零并清除错误标志
  */
void DigitTube_Clear(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    g_display_value = 0;
    g_display_error = 0;
    __set_PRIMASK(primask);
}

/**
  * @brief  切换温度单位 (摄氏/华氏)
  * @note   关中断保护; 切换时自动进行温度值换算:
  *         摄氏→华氏: F = C * 9 / 5 + 32;
  *         华氏→摄氏: C = (F - 32) * 5 / 9;
  *         同时更新LED指示灯状态 (Q1=摄氏, Q2=华氏)
  */
void DigitTube_ToggleUnit(void)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();

    if (g_temp_unit == 0) {
        g_display_value = (uint16_t)((uint32_t)g_display_value * 9 / 5 + 32);
        g_temp_unit = 1;
        g_led_q1_state = 0;
        g_led_q2_state = 1;
    } else {
        if (g_display_value > 32) {
            g_display_value = (uint16_t)((uint32_t)(g_display_value - 32) * 5 / 9);
        } else {
            g_display_value = 0;
        }
        g_temp_unit = 0;
        g_led_q1_state = 1;
        g_led_q2_state = 0;
    }

    __set_PRIMASK(primask);
}

/**
  * @brief  设置指定LED状态
  * @param  led_id: LED编号, 1=Q1, 2=Q2
  * @param  state: LED状态, 0=灭, 非0=亮
  */
void DigitTube_SetLED(uint8_t led_id, uint8_t state)
{
    if (led_id == 1) {
        g_led_q1_state = state ? 1 : 0;
    } else if (led_id == 2) {
        g_led_q2_state = state ? 1 : 0;
    }
}

/**
  * @brief  获取当前温度单位
  * @return 0=摄氏(°C), 1=华氏(°F)
  */
uint8_t DigitTube_GetUnit(void)
{
    return g_temp_unit;
}

/**
  * @brief  显示温度值并设置单位
  * @param  value: 温度值, 范围0~999
  * @param  unit:  温度单位, 0=摄氏(°C), 1=华氏(°F)
  * @return 0=成功, 1=超出范围(显示"Err")
  * @note   关中断保护, 同时更新LED指示灯 (Q1=摄氏, Q2=华氏)
  */
uint8_t DigitTube_DisplayTemp(int32_t value, uint8_t unit)
{
    uint32_t primask;

    if (value < 0 || value > 999) {
        g_display_error = 1;
        return 1;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    g_display_error = 0;
    g_display_value = (uint16_t)value;

    if (unit == 0) {
        g_temp_unit = 0;
        g_led_q1_state = 1;
        g_led_q2_state = 0;
    } else {
        g_temp_unit = 1;
        g_led_q1_state = 0;
        g_led_q2_state = 1;
    }

    __set_PRIMASK(primask);

    return 0;
}

/**
  * @brief  清除错误显示标志, 恢复正常数值显示
  */
void DigitTube_ClearError(void)
{
    g_display_error = 0;
}

/**
  * @brief  查询是否处于错误显示状态
  * @return 0=正常显示, 1=正在显示"Err"
  */
uint8_t DigitTube_IsError(void)
{
    return g_display_error;
}

/**
  * @brief  获取扫描计数器值
  * @return 当前扫描计数, 用于监测扫描是否正常运行
  */
uint32_t DigitTube_GetScanCounter(void)
{
    return g_scan_counter;
}

/**
  * @brief  重置扫描计数器归零
  */
void DigitTube_ResetScanCounter(void)
{
    g_scan_counter = 0;
}

/**
  * @brief  显示错误代码 (3位, 强制前导零)
  * @param  code: 错误代码, 范围0~999, 超过999截断
  * @note   关中断保护; 强制设为3位显示模式并启用前导零,
  *         适用于显示如"001""025"等格式的错误代码
  */
void DigitTube_DisplayErrorCode(uint16_t code)
{
    uint32_t primask;

    if (code > 999) {
        code = 999;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    g_display_error = 0;
    g_display_value = (uint16_t)code;
    g_display_digits = 3;
    g_force_leading_zero = 1;
    __set_PRIMASK(primask);
}

/**
  * @brief  清除错误代码显示模式
  * @note   关中断保护; 恢复前导零消隐和2位显示模式
  */
void DigitTube_ClearErrorCode(void)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();
    g_force_leading_zero = 0;
    g_display_digits = 2;
    __set_PRIMASK(primask);
}
