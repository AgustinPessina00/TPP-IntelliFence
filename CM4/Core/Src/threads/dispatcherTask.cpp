#include "FreeRTOS.h"
#include "task.h"
#include "threads/dispatcherTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"

// Declaraciones externas de las colas que deberían estar definidas en main.c
extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;
extern osMessageQueueId_t loraTxQueueHandle;

void dispatcherTask(void *argument) {
    EmbeddedMessage_t *msg = NULL;
    
    
    // Ahora SÍ podemos usar printf thread-safe
    RTOS_LOG_INFO("[DISPATCHER] Task initialized successfully\n");

    static uint32_t stackMonitorCounter = 0;
    while(1) {
        // Monitorear stack cada ~10 segundos (cada 100 iteraciones × 100ms delay)
        if (++stackMonitorCounter >= 100) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            RTOS_LOG_INFO("[DISPATCHER] Stack libre: %u words (%u bytes)\n", 
                         stackLeft, stackLeft * 4);
            stackMonitorCounter = 0;
        }
        
        if (osMessageQueueGet(dispatcherQueueHandle, &msg, NULL, 0) == osOK) {
            // Ahora podemos hacer logging thread-safe
            RTOS_LOG_DEBUG("[DISPATCHER] Routing msg ID:%d from:%d to:%d\n", 
                          msg->id, msg->sender, msg->receiver);
            
            switch (msg->receiver) {
                case MODULE_SENSOR_ACQ:
                    if (osMessageQueuePut(sensorAcqQueueHandle, &msg, 0, 0) != osOK) {
                        RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to SENSOR_ACQ\n");
                        MessagePool_Free(msg);
                    }
                    break;
                case MODULE_FSM:
                    if (osMessageQueuePut(fsmQueueHandle, &msg, 0, 0) != osOK) {
                        RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to FSM\n");
                        MessagePool_Free(msg);
                    }
                    break;
                case MODULE_STIMULUS:
                    if (osMessageQueuePut(stimulusQueueHandle, &msg, 0, 0) != osOK) {
                            RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to STIMULUS\n");
                            MessagePool_Free(msg);
                    }
                    break;
                case MODULE_LORA_TX:
                    if (osMessageQueuePut(loraTxQueueHandle, &msg, 0, 0) != osOK) {
                        RTOS_LOG_ERROR("[DISPATCHER] Failed to route message to LORA_TX\n");
                        MessagePool_Free(msg);
                    }
                    break;
                case MODULE_GPS:
                case MODULE_LORA_RX:
                case MODULE_DISTANCE:
                case MODULE_FENCE_UPDATE:
                default:
                    RTOS_LOG_WARN("[DISPATCHER] Unknown receiver module: %d\n", msg->receiver);
                    MessagePool_Free(msg);
                    break;
            }
        }
        
        osDelay(100);
    }
}
