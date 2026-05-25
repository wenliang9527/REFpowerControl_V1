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

static const uint8_t err_code[3] = {
    0x79,
    0x50,
    0x50
};

static gpio_type* const tube_gpio[6] = {
    TUBE_PIN_1_GPIO,
    TUBE_PIN_2_GPIO,
    TUBE_PIN_3_GPIO,
    TUBE_PIN_4_GPIO,
    TUBE_PIN_5_GPIO,
    TUBE_PIN_6_GPIO
};

static const uint16_t tube_pin[6] = {
    TUBE_PIN_1_PIN,
    TUBE_PIN_2_PIN,
    TUBE_PIN_3_PIN,
    TUBE_PIN_4_PIN,
    TUBE_PIN_5_PIN,
    TUBE_PIN_6_PIN
};

static const crm_periph_clock_type tube_clock[6] = {
    TUBE_PIN_1_CLOCK,
    TUBE_PIN_2_CLOCK,
    TUBE_PIN_3_CLOCK,
    TUBE_PIN_4_CLOCK,
    TUBE_PIN_5_CLOCK,
    TUBE_PIN_6_CLOCK
};

static volatile uint16_t g_display_value = 0;
static volatile uint8_t  g_temp_unit = 0;
static volatile uint8_t  g_led_q1_state = 0;
static volatile uint8_t  g_led_q2_state = 0;
volatile uint32_t g_scan_counter = 0;

static volatile uint8_t g_display_error = 0;
static volatile uint8_t scan_pos = 0;
static volatile uint8_t g_display_digits = 3;
static volatile uint8_t g_force_leading_zero = 0;

void DigitTube_SyncToGlobal(void)
{
    g_sensor_data.temperature.display_value1 = (int32_t)g_display_value;
    g_sensor_data.temperature.display_unit = (uint8_t)g_temp_unit;
}

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

void DigitTube_SetDisplayDigits(uint8_t digits)
{
    if (digits == 2 || digits == 3) {
        g_display_digits = digits;
    }
}

uint16_t DigitTube_GetValue(void)
{
    return (uint16_t)g_display_value;
}

void DigitTube_SetLEDState(uint8_t q1, uint8_t q2)
{
    g_led_q1_state = q1;
    g_led_q2_state = q2;
}

static uint8_t pin_to_index(uint16_t pin)
{
    uint8_t idx = 0;
    while (pin > 1u) { pin >>= 1u; idx++; }
    return idx;
}

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

static void tube_timer_init(void)
{
    crm_periph_clock_enable(TUBE_SCAN_TIMER_CLOCK, TRUE);
    
    tmr_base_init(TUBE_SCAN_TIMER, TUBE_SCAN_PERIOD - 1, TUBE_SCAN_DIV - 1);
    tmr_cnt_dir_set(TUBE_SCAN_TIMER, TMR_COUNT_UP);
    
    tmr_interrupt_enable(TUBE_SCAN_TIMER, TMR_OVF_INT, TRUE);
    
    tmr_counter_enable(TUBE_SCAN_TIMER, TRUE);
}

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

void DigitTube_Init(void)
{
    tube_gpio_init();
    tube_timer_init();
}

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

void DigitTube_SetValue(uint16_t value)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    g_display_error = 0;
    g_display_value = value > 999 ? 999 : value;
    __set_PRIMASK(primask);
}

void DigitTube_Clear(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    g_display_value = 0;
    g_display_error = 0;
    __set_PRIMASK(primask);
}

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

void DigitTube_SetLED(uint8_t led_id, uint8_t state)
{
    if (led_id == 1) {
        g_led_q1_state = state ? 1 : 0;
    } else if (led_id == 2) {
        g_led_q2_state = state ? 1 : 0;
    }
}

uint8_t DigitTube_GetUnit(void)
{
    return g_temp_unit;
}

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

void DigitTube_ClearError(void)
{
    g_display_error = 0;
}

uint8_t DigitTube_IsError(void)
{
    return g_display_error;
}

uint32_t DigitTube_GetScanCounter(void)
{
    return g_scan_counter;
}

void DigitTube_ResetScanCounter(void)
{
    g_scan_counter = 0;
}

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

void DigitTube_ClearErrorCode(void)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();
    g_force_leading_zero = 0;
    g_display_digits = 2;
    __set_PRIMASK(primask);
}
