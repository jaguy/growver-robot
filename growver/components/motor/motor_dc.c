//*****************************************************************************
//
// motor_dc.c - Complete motor DC driver for Growver robot.
//
// License: GPL-3.0-or-later
// Copyright 2014 Revely Microsystems LLC.
//
//*****************************************************************************
#include "pwm_bdc.h"
#include "motor_dc.h"
#include "esp_log.h"
#include "driver/timer.h"


#define PWM_UNIT MCPWM_UNIT_0

// Map motor number to timer number
 mcpwm_timer_t motor_timer_num[MOTORS_IN_SYSTEM] =
    {MCPWM_TIMER_0, MCPWM_TIMER_1};

// Store speed and direction;
static uint16_t mc_speed[MOTORS_IN_SYSTEM];
static uint16_t mc_direction[MOTORS_IN_SYSTEM];

//*****************************************************************************
// MotorDCInit
//
//*****************************************************************************
int MotorDCInit(void)
{
	// Set up PWM and IO control
    mcpwm_initialize();

	// Start with both motors stopped (control signals high)
	brushed_motor_stop(PWM_UNIT, motor_timer_num[0]);
	brushed_motor_stop(PWM_UNIT, motor_timer_num[1]);
    return 0;
}

//*****************************************************************************
// MotorDCSetSpeed
// Sets the speed and direction of a DC motor. Speed is in percent if open loop
// otherwise in RPM.
//
//*****************************************************************************
void MotorDCSetSpeed(uint8_t motor, uint16_t speed, uint8_t direction)
{
	if (motor >= MOTORS_IN_SYSTEM)
		return;

	if (speed > 100)
	{
		speed = 100;
	}

	mc_speed[motor] = speed;

	// If speed is zero then hard-stop
	if (speed == 0)
	{
        brushed_motor_stop(PWM_UNIT, motor_timer_num[motor]);
		return;
	}

	// In open loop mode, update PWM directly. Max speed is 100% PWM
	if (direction)
	{
		brushed_motor_forward(PWM_UNIT, motor_timer_num[motor], (float)speed);
		mc_direction[motor] = MOTOR_FORWARD;
	}
	else
	{
		brushed_motor_backward(PWM_UNIT, motor_timer_num[motor], (float)speed);
		mc_direction[motor] = MOTOR_REVERSE;
	}
}

//*****************************************************************************
// MotorDCGetSpeed
// Gets the speed of a DC motor.
//
//*****************************************************************************
uint16_t MotorDCGetSpeed(uint8_t motor)
{
	if (motor >= MOTORS_IN_SYSTEM)
		return 0;

	return (mc_speed[motor]);
}

//*****************************************************************************
// MotorDCGetDirection
// Gets the direction of a DC motor.
//
//*****************************************************************************
uint8_t MotorDCGetDirection(uint8_t motor)
{
	return (mc_direction[motor]);
}