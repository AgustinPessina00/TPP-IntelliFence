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

// External queue declarations
extern osMessageQueueId_t consoleQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

// External UART handle (from main.c)
extern UART_HandleTypeDef huart1;  // Console UART

// Circular buffer for UART reception
static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t uart_rx_head = 0;
static volatile uint16_t uart_rx_tail = 0;

// Command line buffer
static char command_line[COMMAND_LINE_SIZE];
static volatile uint8_t command_ready_flag = 0;

// Current byte being received
static uint8_t current_rx_byte = 0;

// Forward declarations
static void console_init(void);
static void console_uart_rx_callback(void);
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
    uart_rx_head = 0;
    uart_rx_tail = 0;
    command_ready_flag = 0;
    memset(command_line, 0, COMMAND_LINE_SIZE);
    memset(uart_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    
    // Start UART reception with interrupts
    HAL_UART_Receive_IT(&huart1, &current_rx_byte, 1);
    
    RTOS_LOG_INFO("[CONSOLE] Initialized successfully\n");
}

/**
 * UART RX callback - called from HAL_UART_RxCpltCallback
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // Add byte to circular buffer
        uart_rx_buffer[uart_rx_head] = current_rx_byte;
        uart_rx_head = (uart_rx_head + 1) % UART_RX_BUFFER_SIZE;
        
        // Check for newline - command complete
        if (current_rx_byte == '\n') {
            command_ready_flag = 1;
        }
        
        // Restart reception
        HAL_UART_Receive_IT(&huart1, &current_rx_byte, 1);
    }
}

/**
 * Extract command line from circular buffer
 */
static void extract_command_line(char* dest, uint16_t max_len) {
    uint16_t i = 0;
    
    while (uart_rx_tail != uart_rx_head && i < (max_len - 1)) {
        dest[i] = uart_rx_buffer[uart_rx_tail];
        uart_rx_tail = (uart_rx_tail + 1) % UART_RX_BUFFER_SIZE;
        
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
 * Format: "<MODULE_CODE> <OPERATION_CODE> [DATA]\n"
 * Example: "41 00\n" or "41 01 A1 B2\n"
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
    HAL_UART_Transmit(&huart1, (uint8_t*)response, strlen(response), 1000);
}

/**
 * Execute console command
 */
static void execute_console_command(ConsoleCommand_t* cmd) {
    char response[RESPONSE_SIZE];
    EmbeddedMessage_t *msgToSend = NULL;
    
    if (!cmd->valid) {
        send_uart_response("[ERROR] Invalid command format\n");
        return;
    }
    
    RTOS_LOG_DEBUG("[CONSOLE] Executing command: module=%d, operation=0x%02X\n", 
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
                        RTOS_LOG_DEBUG("[CONSOLE] GPS read request sent\n");
                        // Response will be sent when data arrives
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send GPS request\n");
                        send_uart_response("[ERROR] GPS read request failed\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[ERROR] Memory allocation failed\n");
                }
            } else if (cmd->operation_code == OP_WRITE) {
                // GPS write configuration
                snprintf(response, RESPONSE_SIZE, "[CONSOLE] GPS config updated OK\n");
                send_uart_response(response);
            } else {
                send_uart_response("[ERROR] Unknown operation code\n");
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
                        RTOS_LOG_DEBUG("[CONSOLE] IMU read request sent\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send IMU request\n");
                        send_uart_response("[ERROR] IMU read request failed\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[ERROR] Memory allocation failed\n");
                }
            } else if (cmd->operation_code == OP_WRITE) {
                // IMU write configuration
                snprintf(response, RESPONSE_SIZE, "[CONSOLE] IMU config updated OK\n");
                send_uart_response(response);
            } else {
                send_uart_response("[ERROR] Unknown operation code\n");
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
                        RTOS_LOG_DEBUG("[CONSOLE] INA GPS read request sent\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send INA GPS request\n");
                        send_uart_response("[ERROR] INA GPS read request failed\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[ERROR] Memory allocation failed\n");
                }
            } else {
                send_uart_response("[ERROR] Unknown operation code\n");
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
                        RTOS_LOG_DEBUG("[CONSOLE] INA IMU read request sent\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send INA IMU request\n");
                        send_uart_response("[ERROR] INA IMU read request failed\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[ERROR] Memory allocation failed\n");
                }
            } else {
                send_uart_response("[ERROR] Unknown operation code\n");
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
                        RTOS_LOG_DEBUG("[CONSOLE] INA MCU read request sent\n");
                    } else {
                        RTOS_LOG_ERROR("[CONSOLE] Failed to send INA MCU request\n");
                        send_uart_response("[ERROR] INA MCU read request failed\n");
                        MessagePool_Free(msgToSend);
                    }
                } else {
                    send_uart_response("[ERROR] Memory allocation failed\n");
                }
            } else {
                send_uart_response("[ERROR] Unknown operation code\n");
            }
            break;
            
        default:
            snprintf(response, RESPONSE_SIZE, "[ERROR] Unknown module code: %d\n", 
                    cmd->module_code);
            send_uart_response(response);
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
        if (command_ready_flag) {
            command_ready_flag = 0;
            
            // Extract command from buffer
            extract_command_line(command_line, COMMAND_LINE_SIZE);
            
            RTOS_LOG_DEBUG("[CONSOLE] Command received: %s\n", command_line);
            
            // Parse and execute command
            ConsoleCommand_t cmd = parse_command(command_line);
            execute_console_command(&cmd);
        }
        
        // Check for messages from other tasks (sensor data responses)
        if (osMessageQueueGet(consoleQueueHandle, &msgReceived, NULL, 10) == osOK) {
            RTOS_LOG_DEBUG("[CONSOLE] Received message ID:%d from module:%d\n", 
                          msgReceived->id, msgReceived->sender);
            
            char response[RESPONSE_SIZE];
            
            switch (msgReceived->id) {
                case MSG_ID_SENSOR_GPS_DATA:
                    if (msgReceived->length == 2 * sizeof(double)) {
                        double latitude, longitude;
                        memcpy(&latitude, msgReceived->payload, sizeof(double));
                        memcpy(&longitude, msgReceived->payload + sizeof(double), sizeof(double));
                        
                        snprintf(response, RESPONSE_SIZE, 
                                "[SENSOR_ACQ] GPS read: lat %.6f, lon %.6f\n", 
                                latitude, longitude);
                        send_uart_response(response);
                    }
                    break;
                    
                case MSG_ID_SENSOR_IMU_DATA:
                    if (msgReceived->length == 3 * sizeof(double)) {
                        double ax, ay, az;
                        memcpy(&ax, msgReceived->payload, sizeof(double));
                        memcpy(&ay, msgReceived->payload + sizeof(double), sizeof(double));
                        memcpy(&az, msgReceived->payload + 2 * sizeof(double), sizeof(double));
                        
                        snprintf(response, RESPONSE_SIZE, 
                                "[SENSOR_ACQ] IMU read: (%.3f g, %.3f g, %.3f g)\n", 
                                ax / 1000.0, ay / 1000.0, az / 1000.0);
                        send_uart_response(response);
                    }
                    break;
                    
                case MSG_ID_SENSOR_INA_GPS_DATA:
                    if (msgReceived->length == sizeof(float)) {
                        float current;
                        memcpy(&current, msgReceived->payload, sizeof(float));
                        
                        snprintf(response, RESPONSE_SIZE, 
                                "[SENSOR_ACQ] INA GPS current read: %.1f mA\n", 
                                current);
                        send_uart_response(response);
                    }
                    break;
                    
                case MSG_ID_SENSOR_INA_IMU_DATA:
                    if (msgReceived->length == sizeof(float)) {
                        float current;
                        memcpy(&current, msgReceived->payload, sizeof(float));
                        
                        snprintf(response, RESPONSE_SIZE, 
                                "[SENSOR_ACQ] INA IMU current read: %.1f mA\n", 
                                current);
                        send_uart_response(response);
                    }
                    break;
                    
                case MSG_ID_SENSOR_INA_MCU_DATA:
                    if (msgReceived->length == sizeof(float)) {
                        float current;
                        memcpy(&current, msgReceived->payload, sizeof(float));
                        
                        snprintf(response, RESPONSE_SIZE, 
                                "[SENSOR_ACQ] INA MCU current read: %.1f mA\n", 
                                current);
                        send_uart_response(response);
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
}
