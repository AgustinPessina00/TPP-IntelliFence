#include "FreeRTOS.h"
#include "task.h"
#include "threads/sensorAcqTask_new.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void sensorAcqTask(void *argument) {
    EmbeddedMessage_t *msg = NULL;
    
    RTOS_LOG_INFO("[SENSOR_ACQ] Task initialized successfully\n");
    
    while(1) {
        // Verificar si hay mensajes de solicitud
        if (osMessageQueueGet(sensorAcqQueueHandle, &msg, NULL, 1000) == osOK) {
            RTOS_LOG_DEBUG("[SENSOR_ACQ] Received message ID:%d from module:%d\n", 
                          msg->id, msg->sender);
            
            // TODO: Procesar solicitudes de datos de sensores según msg->id
            switch (msg->id) {
                case MSG_ID_REQUEST_GPS: {
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] Processing GPS request - reading sensor\n");
                    
                    // Simular lectura de GPS (latitud, longitud)
                    double fake_gps[2] = {-34.6118, -58.3960}; // Buenos Aires coordinates
                    
                    // Crear mensaje de respuesta con datos GPS
                    EmbeddedMessage_t *response_msg = MessagePool_Allocate();
                    if (response_msg != NULL) {
                        response_msg->id = MSG_ID_SEND_GPS;
                        response_msg->sender = MODULE_SENSOR_ACQ;
                        response_msg->receiver = msg->sender; // Responder al que envió el request
                        response_msg->length = sizeof(fake_gps);
                        
                        // Copiar datos GPS al payload
                        memcpy(response_msg->payload, fake_gps, sizeof(fake_gps));
                        
                        RTOS_LOG_INFO("[SENSOR_ACQ] Sending GPS response (lat:%.6f, lon:%.6f)\n", 
                                     fake_gps[0], fake_gps[1]);
                        
                        // Enviar a través del dispatcher
                        if (osMessageQueuePut(dispatcherQueueHandle, &response_msg, 0, 100) != osOK) {
                            RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to send GPS response\n");
                            MessagePool_Free(response_msg);
                        }
                    } else {
                        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to allocate response message\n");
                    }
                    break;
                }
                case MSG_ID_REQUEST_IMU:
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] Processing IMU request\n");
                    // TODO: Implementar respuesta IMU similar a GPS
                    break;
                default:
                    RTOS_LOG_WARN("[SENSOR_ACQ] Unknown request ID: %d\n", msg->id);
                    break;
            }
            
            // Liberar el mensaje recibido
            MessagePool_Free(msg);
        }
        
        osDelay(100);
    }
}