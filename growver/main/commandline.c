 //*****************************************************************************
//
// commandline.c - Handles all terminal (UART) commands for Growver.
//
// Commands are typically issued from a terminal or from a host application
// running on a PC.  Enable verbose mode ("v") for terminal operation.
//
// License: GPL-3.0-or-later
// Copyright 2017 Revely Microsystems LLC.
//
//*****************************************************************************


#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "commandline.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "../components/motor/motor_dc.h"
#include "../components/motor/servo.h"
#include "../components/other/peripheral.h"
#include "tcpip_adapter.h"

#include <math.h>
#include <ctype.h>
#include "esp_log.h"

// Definitions
#define MAX_ARGUMENTS        4

// Verbose type commands
int CmdHelp(int argc, char *argv[]);
int CmdEcho(int argc, char *argv[]);
int CmdDriveForward(int argc, char *argv[]);
int CmdDriveReverse(int argc, char *argv[]);
int CmdSpinLeft(int argc, char *argv[]);
int CmdSpinRight(int argc, char *argv[]);
int CmdPumpControl(int argc, char *argv[]);
int CmdServoControl(int argc, char *argv[]);
int CmdSoftReset(int argc, char *argv[]);
int CmdMotorSpeed(int argc, char *argv[]);
int CmdIPAddress(int argc, char *argv[]);
int CmdBattRead(int argc, char *argv[]);

// GPIO Pin assignments for Growver 2020 module
#define CMD_UART_TX_PIN (GPIO_NUM_26)
#define CMD_UART_RX_PIN (GPIO_NUM_35)

// Set UART to use for serial commands
#define CMD_UART_NUM UART_NUM_1

// Buffers for incoming and outgoing messages
char serial_cmd_buff[100];
char response_buff[1024];

// Prototype for strtoull
//unsigned long long int strtoull(const char * restrict nptr,char ** restrict endptr,int base);

// Define a structure for the command table
typedef struct
{
    // Name
    const char *pName;
    // Function to call.
    pCmdLine pCmd;
    // Text for 'help'
    const char *pHelp;
}
tCmdLineEntry;

// Buffer sizes to use for TX and RX buffers in the UART driver
#define BUF_SIZE (256)
static QueueHandle_t uart0_queue;

// This table that holds the command names, a function pointer, and a
// description for the 'help' command. [B] in the decription indicates that the
// function is blocking.
tCmdLineEntry pCommandTable[] =
{
    { "help",  CmdHelp,   		"  : Display list of commands" },
    { "echo",  CmdEcho,   		"  : Set Echo characers (future)" },
	{ "df", CmdDriveForward,    "    : Drive forward at speed"},
	{ "dr", CmdDriveReverse,    "    : Drive reverse at speed"},
	{ "sl", CmdSpinLeft,  		"    : Spin left at speed"},
	{ "sr", CmdSpinRight,  		"    : Spin right at speed"},
	{ "pump", CmdPumpControl,   "  : Pump control 0..100"},
	{ "servo", CmdServoControl, " : Servo angle 0..180"},
    { "batt", CmdBattRead,   "  : Read battery voltage in mV"},
	{ "reset", CmdSoftReset,    " : Reset Growver"},
	{ "ms", CmdMotorSpeed,      "    : Set DC motor speed"},
	{ "ip", CmdIPAddress,       "    : Get IP address"},
    { 0, 0, 0 }
};

static const char *TAG = "CMD";

// Create an array of pointer to the arguments
static char *pArgv[MAX_ARGUMENTS + 1];

bool g_uart_echo;

//*****************************************************************************
// Cmd_help
// This function implements the "help" command.  It prints a simple list of the
// available commands with a brief description.
//
//*****************************************************************************
int CmdHelp(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    // Print some header text.
    CmdLineRespond("\nCOMMAND LIST");

    // Point at the beginning of the command table.
    pEntry = &pCommandTable[0];

    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    while(pEntry->pCmd)
    {
        // Print the command name and the brief description.
        sprintf(response_buff,"\n%s%s", pEntry->pName, pEntry->pHelp);
        CmdLineRespond(response_buff);
        pEntry++;
    }
    CmdLineRespond("\n");
    // Return success.
    return 0;
}

//*****************************************************************************
// Cmd_echo
// This function implements the "echo" command which turns on/off echoing of
// characters from the terminal.
//
//*****************************************************************************
int CmdEcho(int argc, char *argv[])
{
	uint8_t echo_on;

	// Must be 1 argument in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}
	echo_on = strtoul(argv[1], NULL, 10);
    g_uart_echo = (echo_on == 0) ? (false):(true);
	return 0;
}

//*****************************************************************************
// CmdDriveForward
// This function implements the "df" drive command which sets both drive motors
// running forward at the same speed. Speed is 0..100, which maps to
// DC_FULL_SPEED.
//
//*****************************************************************************
int CmdDriveForward(int argc, char *argv[])
{
	uint16_t speed;

	// Must be 1 argument in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get speed argument
	speed = strtoul(argv[1], NULL, 10);

	ESP_LOGI(TAG, "Forward %d\n", speed);

	// Set the speed for both motors
	MotorDCSetSpeed(MOTOR_R, speed, MOTOR_FORWARD);
	MotorDCSetSpeed(MOTOR_L, speed, MOTOR_FORWARD);

	return (0);
}

//*****************************************************************************
// CmdDriveReverse
// This function implements the "dr" drive command which sets both drive motors
// running in reverse at the same speed. Speed is 0..100, which maps to
// DC_FULL_SPEED.
//
//*****************************************************************************
int CmdDriveReverse(int argc, char *argv[])
{
	int32_t speed;

	// Must be 1 argument in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get speed argument
	speed = strtoul(argv[1], NULL, 10);

	ESP_LOGI(TAG, "Reverse %d\n", speed);

	// Set the speed for both motors
	MotorDCSetSpeed(MOTOR_R, speed, MOTOR_REVERSE);
	MotorDCSetSpeed(MOTOR_L, speed, MOTOR_REVERSE);

	return (0);
}

//*****************************************************************************
// CmdSpinLeft
// This function implements the "sl" drive command which executes a zero-radius
// turn in a CCW direction at specified speed.
//
//*****************************************************************************
int CmdSpinLeft(int argc, char *argv[])
{
	int32_t speed=0;

	// Must be 1 arguments in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get speed argument (0..100)
	speed = strtoul(argv[1], NULL, 10);

	if (speed > 100)
	{
		speed = 100;
	}

	// Set the speed for both motors
	MotorDCSetSpeed(MOTOR_R, speed, MOTOR_REVERSE);
	MotorDCSetSpeed(MOTOR_L, speed, MOTOR_FORWARD);

	return 0;
}

//*****************************************************************************
// CmdSpinRight
// This function implements the "sr" drive command which executes a zero-radius
// turn in a CW direction at specified speed.
//
//*****************************************************************************
int CmdSpinRight(int argc, char *argv[])
{
	int32_t speed=0;

	// Must be 1 arguments in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get speed argument (0..100)
	speed = strtoul(argv[1], NULL, 10);

	if (speed > 100)
	{
		speed = 100;
	}

	// Set the speed for both motors
	MotorDCSetSpeed(MOTOR_L, speed, MOTOR_REVERSE);
	MotorDCSetSpeed(MOTOR_R, speed, MOTOR_FORWARD);

	return 0;
}

//*****************************************************************************
// CmdPumpControl
// This function implements the "pump" control command which turns the pump
// on (>0) or off (=0)
//
//*****************************************************************************
int CmdPumpControl(int argc, char *argv[])
{
	int32_t speed=0;

	// Must be 1 arguments in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get speed argument (0..100)
	speed = strtoul(argv[1], NULL, 10);

	if (speed)
	{
		 PumpControlSet(1);
	}
	else
	{
		PumpControlSet(0);
	}

	return 0;
}

//*****************************************************************************
// CmdServoControl
// This function implements the "servo" control command which sets servo angle.
//
//*****************************************************************************
int CmdServoControl(int argc, char *argv[])
{
	int32_t angle=0;

	// Must be 1 arguments in addition to the command.
	if (argc != 2)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get angle argument (0..180)
	angle = strtoul(argv[1], NULL, 10);

	ServoSetAngle(angle);

	return 0;
}

//*****************************************************************************
// CmdHardReset
//
//
//*****************************************************************************
int CmdSoftReset(int argc, char *argv[])
{
	// Reset - never return
	esp_restart();

	return 0;
}

//*****************************************************************************
// CmdMotorSpeed
// This function implements the "ms" command which sets the speed of the
// specified motor. Speed is 0..100, which maps to
// DC_FULL_SPEED.
//
//*****************************************************************************
int CmdMotorSpeed(int argc, char *argv[])
{
	uint16_t speed;
	uint8_t motor, direction;

	// Must be 3 arguments in addition to the command.
	if (argc != 4)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Get motor argument
	motor = strtoul(argv[1], NULL, 10);

	// Get speed argument
	speed = strtoul(argv[2], NULL, 10);

	// Get direction argument
	direction = strtoul(argv[3], NULL, 10);

	//ESP_LOGI(TAG, "Forward %d\n", speed);

	// Set the speed
	MotorDCSetSpeed(motor, speed, direction);

	return (0);
}

//*****************************************************************************
// CmdIPAddress
// This function implements the "ip" command which gets and sets the IP
// address.
//
//*****************************************************************************
int CmdIPAddress(int argc, char *argv[])
{
    tcpip_adapter_ip_info_t ipInfo;

    // If no argument is provided then read values from parameter storage.
	if (argc == 1)
	{
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
		sprintf(response_buff,IPSTR "\n", IP2STR(&ipInfo.ip));
        CmdLineRespond(response_buff);
	}
	else
	{
		return (CMDLINE_INVALID_ARG);
	}
	return 0;
}

//*****************************************************************************
// CmdBattRead
// This function implements the "batt" command which reads the battery voltage,
// capacity and charge level (if known).
//
//*****************************************************************************
int CmdBattRead(int argc, char *argv[])
{
	// Must be 0 argument in addition to the command.
	if (argc != 1)
	{
		return (CMDLINE_INVALID_ARG);
	}

	// Read and print voltage in millivolts.
    sprintf(response_buff,"%u\n",AnalogVoltageRead());
    CmdLineRespond(response_buff);
	return 0;
}

//*****************************************************************************
// CmdLineProcess
//
// Accepts a command line, determines the action and arguments and executes
// them.
//
//*****************************************************************************
int CmdLineProcess(char *pCommand)
{
    char *pcChar;
    uint8_t argc;
    uint8_t lookForArg = 1;
    tCmdLineEntry *psCmdEntry;

    // Start at the beginning of command line table.
    argc = 0;
    pcChar = pCommand;

    // Until NULL
    while(*pcChar)
    {
        // Look for space to delimit arguments
        if(*pcChar == ' ')
        {
            *pcChar = 0;
            lookForArg = 1;
        }
        else
        {
            // Look for start of next arg?
            if(lookForArg)
            {
                // Save this argument in array and keep looking - unless max arguments.
                if(argc < MAX_ARGUMENTS)
                {
                    pArgv[argc] = pcChar;
                    argc++;
                    lookForArg = 0;
                }

                // Issue with argument count.
                else
                {
                    return(CMDLINE_BAD_ARG_COUNT);
                }
            }
        }

        pcChar++;
    }

    // If one or more arguments was found, then process the command.
    if(argc)
    {
        //
        // Start at the beginning of the command table, to look for a matching
        // command.
        //
        psCmdEntry = &pCommandTable[0];

        //
        // Search through the command table until a null command string is
        // found, which marks the end of the table.
        //
        while(psCmdEntry->pCmd)
        {
            // Is there a match? If so then call the corresponding function.
            if(!strcmp(pArgv[0], psCmdEntry->pName))
            {
				//ESP_LOGI(TAG, "Cmd:%s Args:%u\n", pArgv[0], argc - 1);
                return(psCmdEntry->pCmd(argc, pArgv));
            }

            psCmdEntry++;
        }
    }

    // No matches
    return CMDLINE_BAD_CMD;
}

//*****************************************************************************
// CmdLineRespond
//
//*****************************************************************************
void CmdLineRespond(char *response)
{
    uart_write_bytes(CMD_UART_NUM, (const char *) response, strlen(response));
}

//*****************************************************************************
// HandleUartCommand
//
//*****************************************************************************
void HandleUartCommand(void)
{
    int32_t len;

    // Read the buffer
    len = uart_read_bytes(CMD_UART_NUM, (uint8_t*)serial_cmd_buff, sizeof(serial_cmd_buff), 100 / portTICK_RATE_MS);

    // Change terminator to a null.
    if (len)
    {
        serial_cmd_buff[len - 1] = 0;
        //ESP_LOGI(TAG, "Got string [%s]", serial_cmd_buff);
        CmdLineProcess(serial_cmd_buff);
    }
    return;
}

//*****************************************************************************
// uart_event_task
//
//*****************************************************************************
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;

    // Wait for Uart events. Process them as they come
    while (1)
    {
        if (xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                    // UART Rx event. Do nothing as Rx is handled when <CR> is detected as a pattern.
                    break;
                case UART_FIFO_OVF:
                    // HW FIFO Overflow - flush the buffer.
                    ESP_LOGE(TAG, " HW fifo overflow");
                    uart_flush(CMD_UART_NUM);
                    break;
                case UART_BUFFER_FULL:
                    // Buffer full - flush
                    ESP_LOGE(TAG, "ring buffer full");
                    uart_flush(CMD_UART_NUM);
                    break;
                case UART_BREAK:
                    // Unused
                    break;
                case UART_PARITY_ERR:
                    // Unused - parity is disabled
                    ESP_LOGE(TAG, "parity error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGE(TAG, "frame error");
                    break;
                case UART_PATTERN_DET:
                    HandleUartCommand();
                    break;
                default:
                    ESP_LOGE(TAG, "no service for this event\n");
                    break;
            }
        }
    }
}

//*****************************************************************************
// UART Initialize
//
//*****************************************************************************
void uart_init()
{
    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config =
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(CMD_UART_NUM, &uart_config);
    // Set UART pins using UART0 default pins i.e. no changes
    uart_set_pin(CMD_UART_NUM, CMD_UART_TX_PIN, CMD_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(CMD_UART_NUM, BUF_SIZE, BUF_SIZE, 10, &uart0_queue, 0);

    // Set uart pattern detection function (13 = CR)
    uart_enable_pattern_det_intr(CMD_UART_NUM, ASCII_CR, 1, 10000, 10, 10);

    // Create a task to handle uart event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

    // Welcome message
    CmdLineRespond("\r\nGrowver Start-up\n");
}
