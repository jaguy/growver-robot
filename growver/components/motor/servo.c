//*****************************************************************************
// servo.c - Servo control for Growver 2020
//
// Controls RC servo
//
// License: GPL-3.0-or-later
// Copyright 2017 Revely Microsystems LLC.
//
//*****************************************************************************

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"


#define SERVO_MIN_PULSEWIDTH 750 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2250 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 180 //Maximum angle in degree upto which servo can rotate

static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

void ServoSetAngle(uint32_t angle)
{
    uint32_t count = servo_per_degree_init(angle);
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, count);
}

void ServoInit(void)
{
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, 21);

    // PWM set to 50Hz
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);

    // Set initial position to mid point.
    ServoSetAngle(SERVO_MAX_DEGREE / 2);
}
