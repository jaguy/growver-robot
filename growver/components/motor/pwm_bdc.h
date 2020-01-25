#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

void mcpwm_initialize(void);
void mcpwm_example_brushed_motor_control(void *arg);
void brushed_motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle);
void brushed_motor_backward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle);
void brushed_motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num);

