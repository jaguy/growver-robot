//******************************************************************************
//
// commandline.h
//
//******************************************************************************
//#define SOURCE_UART 0

#define ASCII_LF    10
#define ASCII_CR    13

// This command not recognized
#define CMDLINE_BAD_CMD				-1

// Wrong number of args for this command
#define CMDLINE_BAD_ARG_COUNT   	-2

// Some other argument issue
#define CMDLINE_INVALID_ARG   		-3

// Problem with this command
#define CMDLINE_EXEC_ERROR  		-4

// FIrmware CRC mismatch
#define CMDLINE_FW_CRC_ERROR        -5

// Firmware download complete
#define CMDLINE_FW_LOAD_COMPLETE    10

// Globals
extern bool g_uart_echo;

// Typedef for standard command line function
typedef int (*pCmdLine)(int argc, char *argv[]);

// Prototypes
void CmdLineRespond(char *response);
//bool CmdLineBinaryPacket(void);
//uint16_t CmdRxCRCGet(void);
//int CmdLineProcess(char *pCommand);
//unsigned char VerboseIsOn(void);

void uart_init();
//void HandleUartCommand(void);

// end of commandline.h

