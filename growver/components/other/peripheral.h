// Header file for peripheral.c

void GpioInit(void);
void GpioLevelSet(uint8_t pin, bool level);
bool GpioLevelGet(uint8_t pin);
void PumpControlSet(bool on);
void PumpInit(void);
uint32_t AnalogMotorCurrentRead(uint8_t motor);
uint32_t AnalogVoltageRead(void);
void AnalogMeasInit(void);
