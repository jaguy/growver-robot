// Dc Motor channel assignments

#define MOTOR_L			0
#define MOTOR_R			1

// Number of DC motors
#define MOTORS_IN_SYSTEM    2


#define MOTOR_FORWARD	0
#define MOTOR_REVERSE   1

int MotorDCInit(void);
void MotorDCSetSpeed(uint8_t motor, uint16_t speed, uint8_t direction);
uint16_t MotorDCGetSpeed(uint8_t motor);
uint8_t MotorDCGetDirection(uint8_t motor);
