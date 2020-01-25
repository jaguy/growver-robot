//*****************************************************************************
//
// peripheral.c - Handles simple peripheral functions for Growver
//
// License: GPL-3.0-or-later
// Copyright 2018 Revely Microsystems LLC.
//
//*****************************************************************************

#include "driver/gpio.h"
#include "peripheral.h"
#include "driver/adc.h"
//#include "esp_adc_cal.h"

// Pump control I/O assignments
#define GPIO_OUTPUT_PUMP   2
#define GPIO_OUTPUT_PIN_SEL  (1<<GPIO_OUTPUT_PUMP)

// Aux I/O Assignments
gpio_num_t aux_io_pin[] = {0, 32};
#if 0
#define GPIO_AUX0          0
#define GPIO_AUX0_PIN_SEL  (1<<GPIO_AUX0)

#define GPIO_AUX1          32
#define GPIO_AUX1_PIN_SEL  (1<<GPIO_AUX1)
#endif

// ADC Parameters
#define V_REF   1185
#define ADC_VBUS_CHANNEL (ADC1_CHANNEL_6)      // GPIO 34

// Init ADC and Characteristics
//esp_adc_cal_characteristics_t characteristics;

//*****************************************************************************
// GpioInit
//
//*****************************************************************************
void GpioInit(void)
{
    gpio_config_t io_conf;

    // Pins default to input with internal pull-up enabled.
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1 << aux_io_pin[0]) | (1 << aux_io_pin[1]);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

//*****************************************************************************
// GpioLevelSet
// Sets a GPIO pin high or low. Automatically changes pin to output.
//
//*****************************************************************************
void GpioLevelSet(uint8_t pin, bool level)
{
    gpio_config_t io_conf;

    // Change pin to output
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    io_conf.pin_bit_mask = 1 << aux_io_pin[pin];
    gpio_config(&io_conf);

    // Set level
    gpio_set_level(aux_io_pin[pin], level);
}

//*****************************************************************************
// GpioLevelGet
//
//
//*****************************************************************************
bool GpioLevelGet(uint8_t pin)
{
    gpio_config_t io_conf;

    // Change pin to input
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    io_conf.pin_bit_mask = 1 << aux_io_pin[pin];
    gpio_config(&io_conf);

    // Read and return the level
    return ((bool)gpio_get_level(aux_io_pin[pin]));
}

//*****************************************************************************
// PumpControlSet
//
//*****************************************************************************
void PumpControlSet(bool on)
{
    if (on)
    {
        gpio_set_level(GPIO_OUTPUT_PUMP, on);
    }
    else
    {
        gpio_set_level(GPIO_OUTPUT_PUMP, on);
    }
}

//*****************************************************************************
// PumpInit
//
//*****************************************************************************
void PumpInit(void)
{
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

//*****************************************************************************
// AnalogVoltageRead
// Returns voltage in millivolts
// 200k with 7.68k = 27.04:1 or 29.75V span
// Counts are 0..4095, so LSB = 7.26mV
//
//*****************************************************************************
uint32_t AnalogVoltageRead(void)
{
    uint32_t raw_mv;

    // Attenuator on Vbat monitor is 39.3:1
    raw_mv = adc1_get_raw(ADC_VBUS_CHANNEL);
    //raw_mv = adc1_to_voltage(ADC_VBUS_CHANNEL, &characteristics);
    raw_mv *= 726;
    raw_mv /= 100;
    return(raw_mv);
}

//*****************************************************************************
// AnalogMeasInit
//
//*****************************************************************************
void AnalogMeasInit(void)
{
    // With 0dB, span is roughly 1.1V
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC_VBUS_CHANNEL, ADC_ATTEN_0db);
    //esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_0db, ADC_WIDTH_12Bit, &characteristics);
}

