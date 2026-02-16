#include "FreeRTOS.h"
#include "task.h"
#include "threads/dispatcherTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"

// Declaraciones externas de las colas que deberían estar definidas en main.c
extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t sensorAcqQueueHandle;  // LEGACY
extern osMessageQueueId_t imuQueueHandle;
extern osMessageQueueId_t gpsQueueHandle;
extern osMessageQueueId_t inaQueueHandle;
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;
extern osMessageQueueId_t loraTxQueueHandle;

void dispatcherTask(void *argument) {
    EmbeddedMessage_t *msg = NULL;
    
    
    // Ahora SÍ podemos usar printf thread-safe
    RTOS_LOG_INFO("[DISPATCHER] Task initialized successfully\r\n");

    static uint32_t stackMonitorCounter = 0;
    while(1) {
        // Monitorear stack cada ~10 segundos (cada 10 iteraciones × 1000ms delay)
        if (++stackMonitorCounter >= 10) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            RTOS_LOG_INFO("[DISPATCHER] Stack libre: %u words (%u bytes)\r\n", 
                         stackLeft, stackLeft * 4);
            stackMonitorCounter = 0;
        }
        
        // Monitorear colas cada 5 segundos (cada 10 iteraciones × 500ms delay)
        // if (++queueMonitorCounter >= 10) {
        //     RTOS_LOG_INFO("[QUEUE STATUS] =============================\r\n");
        //     RTOS_LOG_INFO("  Dispatcher  - Msgs: %u / Spaces: %u\r\n", 
        //                  osMessageQueueGetCount(dispatcherQueueHandle),
        //                  osMessageQueueGetSpace(dispatcherQueueHandle));
        //     RTOS_LOG_INFO("  SensorAcq   - Msgs: %u / Spaces: %u\r\n",
        //                  osMessageQueueGetCount(sensorAcqQueueHandle),
        //                  osMessageQueueGetSpace(sensorAcqQueueHandle));
        //     RTOS_LOG_INFO("  FSM         - Msgs: %u / Spaces: %u\r\n",
        //                  osMessageQueueGetCount(fsmQueueHandle),
        //                  osMessageQueueGetSpace(fsmQueueHandle));
        //     RTOS_LOG_INFO("  Stimulus    - Msgs: %u / Spaces: %u\r\n",
        //                  osMessageQueueGetCount(stimulusQueueHandle),
        //                  osMessageQueueGetSpace(stimulusQueueHandle));
        //     RTOS_LOG_INFO("  LoraTx      - Msgs: %u / Spaces: %u\r\n",
        //                  osMessageQueueGetCount(loraTxQueueHandle),
        //                  osMessageQueueGetSpace(loraTxQueueHandle));
        //     RTOS_LOG_INFO("============================================\r\n");
        //     queueMonitorCounter = 0;
        // }
        
        if (osMessageQueueGet(dispatcherQueueHandle, &msg, NULL, 0) == osOK) {
            // Ahora podemos hacer logging thread-safe
            RTOS_LOG_DEBUG("[DISPATCHER] Routing msg ID:%d from:%d to:%d\r\n", 
                          msg->id, msg->sender, msg->receiver);
            
            // Ruteo inteligente basado en MSG_ID (prioridad sobre receiver)
            // Esto permite que mensajes de sensores vayan a tareas especializadas
            switch (msg->id) {
                // === IMU Messages ===
                case MSG_ID_REQUEST_IMU:
                case MSG_ID_CONSOLE_READ_IMU:
                    if (osMessageQueuePut(imuQueueHandle, &msg, 0, 0) != osOK) {
                        RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to IMU\r\n");
                        MessagePool_Free(msg);
                    }
                    break;
                    
                // === GPS Messages ===
                case MSG_ID_REQUEST_GPS:
                case MSG_ID_GPS_REQUEST_CONFIG:
                case MSG_ID_CONSOLE_READ_GPS:
                    if (osMessageQueuePut(gpsQueueHandle, &msg, 0, 0) != osOK) {
                        RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to GPS\r\n");
                        MessagePool_Free(msg);
                    }
                    break;
                    
                // === INA Messages ===
                case MSG_ID_REQUEST_INA_MCU:
                case MSG_ID_REQUEST_INA_GPS:
                case MSG_ID_REQUEST_INA_IMU:
                case MSG_ID_CONSOLE_READ_INA_GPS:
                case MSG_ID_CONSOLE_READ_INA_IMU:
                case MSG_ID_CONSOLE_READ_INA_MCU:
                    if (osMessageQueuePut(inaQueueHandle, &msg, 0, 0) != osOK) {
                        RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to INA\r\n");
                        MessagePool_Free(msg);
                    }
                    break;
                    
                // === Fallback: Enrutar por MODULE_RECEIVER ===
                default:
                    switch (msg->receiver) {
                        case MODULE_SENSOR_ACQ:  // LEGACY - deprecado
                            if (osMessageQueuePut(sensorAcqQueueHandle, &msg, 0, 0) != osOK) {
                                RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to SENSOR_ACQ (legacy)\r\n");
                                MessagePool_Free(msg);
                            }
                            break;
                        case MODULE_FSM:
                            if (osMessageQueuePut(fsmQueueHandle, &msg, 0, 0) != osOK) {
                                RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to FSM\r\n");
                                MessagePool_Free(msg);
                            }
                            break;
                        case MODULE_STIMULUS:
                            if (osMessageQueuePut(stimulusQueueHandle, &msg, 0, 0) != osOK) {
                                    RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to STIMULUS\r\n");
                                    MessagePool_Free(msg);
                            }
                            break;
                        case MODULE_LORA_TX:
                            if (osMessageQueuePut(loraTxQueueHandle, &msg, 0, 0) != osOK) {
                                RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to LORA_TX\r\n");
                                MessagePool_Free(msg);
                            }
                            break;
                        case MODULE_GPS:
                        case MODULE_LORA_RX:
                        case MODULE_DISTANCE:
                        case MODULE_FENCE_UPDATE:
                        default:
                            RTOS_LOG_WARN("[DISPATCHER] Unknown receiver module: %d (msg_id=%d)\r\n", msg->receiver, msg->id);
                            MessagePool_Free(msg);
                            break;
                    }
                    break;
            }
        }
        
        osDelay(500);
    }
}
