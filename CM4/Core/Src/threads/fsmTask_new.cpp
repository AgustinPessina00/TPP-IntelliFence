#include "FreeRTOS.h"
#include "task.h"
#include "threads/fsmTask_new.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"

// Declaraciones externas de las colas
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void fsmTask(void *argument) {
    EmbeddedMessage_t *msg = NULL;
    static uint32_t test_counter = 0;
    
    RTOS_LOG_INFO("[FSM] Task initialized successfully\n");
    
    while(1) {
        // Verificar si hay mensajes
        if (osMessageQueueGet(fsmQueueHandle, &msg, NULL, 100) == osOK) {
            RTOS_LOG_DEBUG("[FSM] Received message ID:%d from module:%d\n", msg->id, msg->sender);
            
            // TODO: Implementar lógica de máquina de estados
            switch (msg->id) {
                case MSG_ID_SEND_GPS: {
                    if (msg->length >= sizeof(double) * 2) {
                        double *gps_data = (double*)msg->payload;
                        RTOS_LOG_INFO("[FSM] Received GPS data - Lat:%.6f, Lon:%.6f\n", 
                                     gps_data[0], gps_data[1]);
                        
                        // TODO: Procesar coordenadas GPS (calcular zona, distancia al límite, etc.)
                    } else {
                        RTOS_LOG_WARN("[FSM] Invalid GPS data length: %d bytes\n", msg->length);
                    }
                    break;
                }
                case MSG_ID_SEND_IMU:
                    RTOS_LOG_DEBUG("[FSM] Processing IMU data\n");
                    break;
                default:
                    RTOS_LOG_WARN("[FSM] Unknown message ID: %d\n", msg->id);
                    break;
            }
            
            // Liberar el mensaje recibido
            MessagePool_Free(msg);
        }
        
        // Cada 5 segundos, enviar un mensaje de prueba al SENSOR_ACQ
        test_counter++;
        if (test_counter >= 50) { // 50 * 100ms = 5000ms = 5s
            test_counter = 0;
            
            // Crear mensaje de prueba
            EmbeddedMessage_t *test_msg = MessagePool_Allocate();
            if (test_msg != NULL) {
                test_msg->id = MSG_ID_REQUEST_GPS;
                test_msg->sender = MODULE_FSM;
                test_msg->receiver = MODULE_SENSOR_ACQ;
                test_msg->length = 0;
                
                RTOS_LOG_DEBUG("[FSM] Sending test GPS request to SENSOR_ACQ\n");
                
                // Enviar a través del dispatcher
                if (osMessageQueuePut(dispatcherQueueHandle, &test_msg, 0, 100) != osOK) {
                    RTOS_LOG_ERROR("[FSM] Failed to send test message\n");
                    MessagePool_Free(test_msg);
                }
            } else {
                RTOS_LOG_ERROR("[FSM] Failed to allocate message for test\n");
            }
        }
        
        osDelay(100);
    }
}