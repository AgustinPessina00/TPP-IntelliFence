#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void consoleTask(void *argument);

#ifdef __cplusplus
}
#endif

// Console device codes (for UART protocol)
#define CONSOLE_DEV_GPS      41
#define CONSOLE_DEV_IMU      42
#define CONSOLE_DEV_INA_GPS  43
#define CONSOLE_DEV_INA_IMU  44
#define CONSOLE_DEV_INA_MCU  45

// Operation codes
#define OP_READ   0x00
#define OP_WRITE  0x01

// Buffer sizes
#define UART_RX_BUFFER_SIZE 256
#define COMMAND_LINE_SIZE   128
#define RESPONSE_SIZE       256
#define MAX_DATA_LEN        16

// Command structure
typedef struct {
    uint8_t module_code;
    uint8_t operation_code;
    uint8_t data[MAX_DATA_LEN];
    uint8_t data_length;
    bool valid;
} ConsoleCommand_t;

#endif // CONSOLE_H
