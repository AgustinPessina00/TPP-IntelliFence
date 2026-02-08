/**
 * @file console.h
 * @brief Console Protocol for UART2 Communication with STM32
 * 
 * PROTOCOL SPECIFICATION:
 * ======================
 * 
 * COMMAND FORMAT (PC → MCU):
 *   <MODULE_CODE> <OPERATION_CODE> [DATA]\n
 *   
 *   Example: "41 00\n"  → Read GPS
 *            "42 00\n"  → Read IMU
 *            "41 01 A1 B2\n" → Write to GPS with data A1 B2
 * 
 * RESPONSE FORMAT (MCU → PC):
 *   [CONSOLE] <message>\n
 *   
 *   All messages visible in web console MUST have [CONSOLE] prefix.
 *   The prefix will be removed by backend before displaying to user.
 *   
 *   Example responses:
 *   - "[CONSOLE] GPS Reading...\r\n"
 *   - "[CONSOLE] Latitude: -34.603722\r\n"
 *   - "[CONSOLE] Longitude: -58.381592\r\n"
 *   - "[CONSOLE] Read complete\r\n"
 *   
 * MODULE CODES:
 *   41 = GPS
 *   42 = IMU
 *   43 = INA_GPS (Current sensor)
 *   44 = INA_IMU (Current sensor)
 *   45 = INA_MCU (Current sensor)
 *   
 * OPERATION CODES:
 *   00 = READ  - Request sensor data
 *   01 = WRITE - Configure/Write to sensor
 */

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
#define CONSOLE_DEV_GPS      41  // GPS Module
#define CONSOLE_DEV_IMU      42  // IMU/Accelerometer
#define CONSOLE_DEV_INA_GPS  43  // GPS Current Sensor
#define CONSOLE_DEV_INA_IMU  44  // IMU Current Sensor
#define CONSOLE_DEV_INA_MCU  45  // MCU Current Sensor

// Operation codes
#define OP_READ   0x00
#define OP_WRITE  0x01

// Buffer sizes
#define UART_RX_BUFFER_SIZE 128  // Reduced from 256 to save RAM
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
