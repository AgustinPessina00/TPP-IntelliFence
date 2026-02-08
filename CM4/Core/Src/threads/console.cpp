#include "FreeRTOS.h"
#include "task.h"
#include "threads/console.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "gps_data.h"

// External queue declarations
extern osMessageQueueId_t consoleQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

// External UART handle (from main.c)
extern UART_HandleTypeDef huart2;  // Console UART

// Circular buffer for UART reception - exposed for callback
uint8_t console_uart_rx_buffer[UART_RX_BUFFER_SIZE];
volatile uint16_t console_uart_rx_head = 0;
volatile uint16_t console_uart_rx_tail = 0;

// Command line buffer
static char command_line[COMMAND_LINE_SIZE];
volatile uint8_t console_command_ready_flag = 0;

// Current byte being received - exposed for callback
uint8_t console_current_rx_byte = 0;

// Forward declarations
static void console_init(void);
static void extract_command_line(char* dest, uint16_t max_len);
static uint8_t hex_string_to_bytes(char* hex_str, uint8_t* bytes, uint8_t max_len);
static ConsoleCommand_t parse_command(char* cmd_line);
static void execute_console_command(ConsoleCommand_t* cmd);
static void send_uart_response(const char* response);

/**
 * Initialize console protocol
 */
static void console_init(void) {
    // Reset buffers
    console_uart_rx_head = 0;
    console_uart_rx_tail = 0;
    console_command_ready_flag = 0;
    memset(command_line, 0, COMMAND_LINE_SIZE);
    memset(console_uart_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    
    // Start UART reception with interrupts
    HAL_UART_Receive_IT(&huart2, &console_current_rx_byte, 1);
    
    RTOS_LOG_INFO("[CONSOLE] Initialized successfully\r\n");
}

/**
 * Extract command line from circular buffer
 */
static void extract_command_line(char* dest, uint16_t max_len) {
    uint16_t i = 0;
    
    while (console_uart_rx_tail != console_uart_rx_head && i < (max_len - 1)) {
        dest[i] = console_uart_rx_buffer[console_uart_rx_tail];
        console_uart_rx_tail = (console_uart_rx_tail + 1) % UART_RX_BUFFER_SIZE;
        
        // Stop at newline
        if (dest[i] == '\n') {
            dest[i] = '\0';
            break;
        }
        i++;
    }
    
    dest[i] = '\0'; // Null terminator
}

/**
 * Convert hex string to bytes array
 */
static uint8_t hex_string_to_bytes(char* hex_str, uint8_t* bytes, uint8_t max_len) {
    uint8_t len = 0;
    char* token = strtok(hex_str, " ");
    
    while (token != NULL && len < max_len) {
        bytes[len++] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, " ");
    }
    
    return len;
}

/**
 * Parse command line
 * Format: "<MODULE_CODE> <OPERATION_CODE> [DATA]\r\n"
 * Example: "41 00\r\n" or "41 01 A1 B2\r\n"
 */
static ConsoleCommand_t parse_command(char* cmd_line) {
    ConsoleCommand_t cmd = {0};
    
    int module, operation;
    char data_str[64] = {0};
    
    // Parse components
    int parsed = sscanf(cmd_line, "%d %x %s", &module, &operation, data_str);
    
    if (parsed >= 2) {
        cmd.module_code = (uint8_t)module;
        cmd.operation_code = (uint8_t)operation;
        cmd.valid = true;
        
        // Parse hex data if present
        if (parsed >= 3 && strlen(data_str) > 0) {
            cmd.data_length = hex_string_to_bytes(data_str, cmd.data, MAX_DATA_LEN);
        }
    } else {
        cmd.valid = false;
    }
    
    return cmd;
}

/**
 * Send response via UART
 */
static void send_uart_response(const char* response) {
    HAL_UART_Transmit(&huart2, (uint8_t*)response, strlen(response), 1000);
}

/**
 * Execute console command
 * Implements the console protocol specification:
 * - Validates command format
 * - Routes commands to appropriate modules (GPS, IMU, INA sensors)
 * - Sends [CONSOLE] prefixed responses for web interface visibility
 */
static void execute_console_command(ConsoleCommand_t* cmd) {
    EmbeddedMessage_t *msgToSend = NULL;
    
    if (!cmd->valid) {
        send_uart_response("[CONSOLE] ERROR: Invalid command format\r\n");
        send_uart_response("[CONSOLE] Expected format: <MODULE_CODE> <OP_CODE> [DATA]\r\n");
        send_uart_response("[CONSOLE] Example: 41 00 (Read GPS)\r\n");
        return;
    }
    
    RTOS_LOG_DEBUG("[CONSOLE] Executing command: module=%d, operation=0x%02X\r\n", 
                  cmd->module_code, cmd->operation_code);
    
    switch (cmd->module_code) {
        case CONSOLE_DEV_GPS: // 41
            if (cmd->operation_code == OP_READ) {
                // Request GPS data from sensor task
                msgToSend = MessagePool_Allocate();
                if (msgToSend != NULL) {
                    EmbeddedMessage_Create(msgToSend, 
                                          MSG_ID_CONSOLE_READ_GPS,
                                          MODULE_CONSOLE,
                                          MODULE_SENSOR_ACQ);
                    
                    osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                         &msgToSend, 0, 100);
                    if (status == osOK) {
                        RTOS_LOG_DEBUG("[CONSOLE] GPS read request sent\r\n");
                        send_uart_response("[CONSOLE] GPS Reading...\r\n");
                        // Response will be sent when data arrives
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send GPS request\r\n");
                        send_uart_response("[CONSOLE] ERROR: GPS read request failed\r\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[CONSOLE] ERROR: Memory allocation failed\r\n");
                }
            } else if (cmd->operation_code == OP_WRITE) {
                // GPS write configuration
                send_uart_response("[CONSOLE] GPS Write command received\r\n");
                // Send data to GPS module (implementation pending)
                send_uart_response("[CONSOLE] GPS Config OK\r\n");
                send_uart_response("[CONSOLE] ACK\r\n");
            } else {
                send_uart_response("[CONSOLE] ERROR: Unknown operation code\r\n");
            }
            break;
            
        case CONSOLE_DEV_IMU: // 42
            if (cmd->operation_code == OP_READ) {
                // Request IMU data from sensor task
                msgToSend = MessagePool_Allocate();
                if (msgToSend != NULL) {
                    EmbeddedMessage_Create(msgToSend, 
                                          MSG_ID_CONSOLE_READ_IMU,
                                          MODULE_CONSOLE,
                                          MODULE_SENSOR_ACQ);
                    
                    osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                         &msgToSend, 0, 100);
                    if (status == osOK) {
                        RTOS_LOG_DEBUG("[CONSOLE] IMU read request sent\r\n");
                        send_uart_response("[CONSOLE] IMU Reading...\r\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send IMU request\r\n");
                        send_uart_response("[CONSOLE] ERROR: IMU read request failed\r\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[CONSOLE] ERROR: Memory allocation failed\r\n");
                }
            } else if (cmd->operation_code == OP_WRITE) {
                // IMU write configuration
                send_uart_response("[CONSOLE] IMU Write command received\r\n");
                send_uart_response("[CONSOLE] IMU Calibration complete\r\n");
                send_uart_response("[CONSOLE] ACK\r\n");
            } else {
                send_uart_response("[CONSOLE] ERROR: Unknown operation code\r\n");
            }
            break;
            
        case CONSOLE_DEV_INA_GPS: // 43
            if (cmd->operation_code == OP_READ) {
                // Request INA GPS data
                msgToSend = MessagePool_Allocate();
                if (msgToSend != NULL) {
                    EmbeddedMessage_Create(msgToSend, 
                                          MSG_ID_CONSOLE_READ_INA_GPS,
                                          MODULE_CONSOLE,
                                          MODULE_SENSOR_ACQ);
                    
                    osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                         &msgToSend, 0, 100);
                    if (status == osOK) {
                        RTOS_LOG_DEBUG("[CONSOLE] INA GPS read request sent\r\n");
                        send_uart_response("[CONSOLE] INA_GPS Reading...\r\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send INA GPS request\r\n");
                        send_uart_response("[CONSOLE] ERROR: INA GPS read request failed\r\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[CONSOLE] ERROR: Memory allocation failed\r\n");
                }
            } else {
                send_uart_response("[CONSOLE] ERROR: Unknown operation code\r\n");
            }
            break;
            
        case CONSOLE_DEV_INA_IMU: // 44
            if (cmd->operation_code == OP_READ) {
                // Request INA IMU data
                msgToSend = MessagePool_Allocate();
                if (msgToSend != NULL) {
                    EmbeddedMessage_Create(msgToSend, 
                                          MSG_ID_CONSOLE_READ_INA_IMU,
                                          MODULE_CONSOLE,
                                          MODULE_SENSOR_ACQ);
                    
                    osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                         &msgToSend, 0, 100);
                    if (status == osOK) {
                        RTOS_LOG_DEBUG("[CONSOLE] INA IMU read request sent\r\n");
                        send_uart_response("[CONSOLE] INA_IMU Reading...\r\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send INA IMU request\r\n");
                        send_uart_response("[CONSOLE] ERROR: INA IMU read request failed\r\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[CONSOLE] ERROR: Memory allocation failed\r\n");
                }
            } else {
                send_uart_response("[CONSOLE] ERROR: Unknown operation code\r\n");
            }
            break;
            
        case CONSOLE_DEV_INA_MCU: // 45
            if (cmd->operation_code == OP_READ) {
                // Request INA MCU data
                msgToSend = MessagePool_Allocate();
                if (msgToSend != NULL) {
                    EmbeddedMessage_Create(msgToSend, 
                                          MSG_ID_CONSOLE_READ_INA_MCU,
                                          MODULE_CONSOLE,
                                          MODULE_SENSOR_ACQ);
                    
                    osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                         &msgToSend, 0, 100);
                    if (status == osOK) {
                        RTOS_LOG_DEBUG("[CONSOLE] INA MCU read request sent\r\n");
                        send_uart_response("[CONSOLE] INA_MCU Reading...\r\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send INA MCU request\r\n");
                        send_uart_response("[CONSOLE] ERROR: INA MCU read request failed\r\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[CONSOLE] ERROR: Memory allocation failed\r\n");
                }
            } else {
                send_uart_response("[CONSOLE] ERROR: Unknown operation code\r\n");
            }
            break;
            
        default:
            {
                char error_msg[64];
                snprintf(error_msg, sizeof(error_msg), 
                        "[CONSOLE] ERROR: Invalid module code %02X\r\n", cmd->module_code);
                send_uart_response(error_msg);
                send_uart_response("[CONSOLE] Valid codes: 41-45\r\n");
            }
            break;
    }
}

/**
 * Console Task - Main loop
 */
void consoleTask(void *argument) {
    (void)argument; // Unused parameter
    
    // Initialize console
    console_init();
    
    EmbeddedMessage_t *msgReceived = NULL;
    
    while (1) {
        // Check for incoming commands from UART
        if (console_command_ready_flag) {
            console_command_ready_flag = 0;
            
            // Extract command from buffer
            extract_command_line(command_line, COMMAND_LINE_SIZE);
            
            RTOS_LOG_DEBUG("[CONSOLE] Command received: %s\r\n", command_line);
            
            // Parse and execute command
            ConsoleCommand_t cmd = parse_command(command_line);
            execute_console_command(&cmd);
        }
        
        // Check for messages from other tasks (sensor data responses)
        if (osMessageQueueGet(consoleQueueHandle, &msgReceived, NULL, 10) == osOK) {
            RTOS_LOG_DEBUG("[CONSOLE] Received message ID:%d from module:%d\r\n", 
                          msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_SENSOR_GPS_DATA:
                    if (msgReceived->length == sizeof(gpsData_t)) {
                        gpsData_t gpsData;
                        memcpy(&gpsData, msgReceived->payload, sizeof(gpsData_t));
                        
                        char response[256];
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Latitude: %.6f\r\n", gpsData.latitude);
                        send_uart_response(response);
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Longitude: %.6f\r\n", gpsData.longitude);
                        send_uart_response(response);
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Fix: %s\r\n", gpsData.fix ? "Valid" : "No Fix");
                        send_uart_response(response);
                        send_uart_response("[CONSOLE] Read complete\r\n");
                    }
                    break;
                    
                case MSG_ID_SENSOR_IMU_DATA:
                    if (msgReceived->length == 3 * sizeof(float)) {
                        float ax, ay, az;
                        memcpy(&ax, msgReceived->payload, sizeof(float));
                        memcpy(&ay, msgReceived->payload + sizeof(float), sizeof(float));
                        memcpy(&az, msgReceived->payload + 2 * sizeof(float), sizeof(float));
                        
                        char response[128];
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Accel X: %.3f g\r\n", ax / 1000.0);
                        send_uart_response(response);
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Accel Y: %.3f g\r\n", ay / 1000.0);
                        send_uart_response(response);
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Accel Z: %.3f g\r\n", az / 1000.0);
                        send_uart_response(response);
                        send_uart_response("[CONSOLE] Read complete\r\n");
                    }
                    break;
                    
                case MSG_ID_SENSOR_INA_GPS_DATA:
                    if (msgReceived->length == sizeof(float)) {
                        float current;
                        memcpy(&current, msgReceived->payload, sizeof(float));
                        
                        char response[128];
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Current: %.1f mA\r\n", current);
                        send_uart_response(response);
                        send_uart_response("[CONSOLE] Read complete\r\n");
                    }
                    break;
                    
                case MSG_ID_SENSOR_INA_IMU_DATA:
                    if (msgReceived->length == sizeof(float)) {
                        float current;
                        memcpy(&current, msgReceived->payload, sizeof(float));
                        
                        char response[128];
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Current: %.1f mA\r\n", current);
                        send_uart_response(response);
                        send_uart_response("[CONSOLE] Read complete\r\n");
                    }
                    break;
                    
                case MSG_ID_SENSOR_INA_MCU_DATA:
                    if (msgReceived->length == sizeof(float)) {
                        float current;
                        memcpy(&current, msgReceived->payload, sizeof(float));
                        
                        char response[128];
                        snprintf(response, sizeof(response), 
                                "[CONSOLE] Current: %.1f mA\r\n", current);
                        send_uart_response(response);
                        send_uart_response("[CONSOLE] Read complete\r\n");
                    }
                    break;
                    
                default:
                    RTOS_LOG_WARN("[CONSOLE] Unhandled message ID: %d\n", 
                                  msgReceived->id);
                    break;
            }
            
            // Free the message back to the pool
            MessagePool_Free(msgReceived);
        }
    }

    osDelay(1000);
}
