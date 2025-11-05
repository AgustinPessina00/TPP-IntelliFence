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

// Variables globales para debugging (visibles en debugger)
volatile uint32_t dispatcher_status = 0;        // 0=init, 1=running, 2=error
volatile uint32_t dispatcher_iterations = 0;
volatile uint32_t dispatcher_messages_received = 0;
volatile uint32_t dispatcher_last_message_id = 0;
volatile uint32_t dispatcher_last_sender = 0;
volatile uint32_t dispatcher_last_receiver = 0;

void dispatcherTask(void *argument) {
    EmbeddedMessage_t *msg = NULL;
    
    // Marcar que el task se inicializó correctamente
    dispatcher_status = 1; // running
    
    // Ahora SÍ podemos usar printf thread-safe
    RTOS_LOG_INFO("[DISPATCHER] Task initialized successfully\n");

    while(1) {
        dispatcher_iterations++;
        
        if (osMessageQueueGet(dispatcherQueueHandle, &msg, NULL, 0) == osOK) {
            dispatcher_messages_received++;
            
            // Guardar información del mensaje para debugging
            dispatcher_last_message_id = msg->id;
            dispatcher_last_sender = msg->sender;
            dispatcher_last_receiver = msg->receiver;
            
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
                case MODULE_GPS:
                case MODULE_LORA_TX:
                case MODULE_LORA_RX:
                case MODULE_DISTANCE:
                case MODULE_FENCE_UPDATE:
                default:
                    RTOS_LOG_WARN("[DISPATCHER] Unknown receiver module: %d\n", msg->receiver);
                    MessagePool_Free(msg);
                    break;
            }
        }
        
        // Log periódico de estadísticas cada ~10 segundos
        if (dispatcher_iterations % 1000 == 0) {
            RTOS_LOG_INFO("[DISPATCHER] Stats - Iterations: %lu, Messages: %lu\n",
                         dispatcher_iterations, dispatcher_messages_received);
        }
        
        osDelay(100);
    }
}
