#include "FreeRTOS.h"
#include "task.h"
#include "threads/loraTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <string.h>

#ifdef ENABLE_TEST_MODE
#include "Test/test_data.h"
#endif

// Declaraciones externas de las colas
extern osMessageQueueId_t loraTxQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void loraTask(void *argument) {
    (void)argument; // Unused parameter
    
    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    
    RTOS_LOG_INFO("[LORA] Task initialized successfully\n");
    
    while(1) {
        // Esperar mensajes en la cola de LoRa TX
        if (osMessageQueueGet(loraTxQueueHandle, &msgReceived, NULL, osWaitForever) == osOK) {
            RTOS_LOG_DEBUG("[LORA] Received message ID:%d from module:%d\n", 
                          msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_LORA_SEND_POSITION:
                    // Procesar el mensaje de posición
                    if (msgReceived->length == 2 * sizeof(double)) {
                        double latitude, longitude;
                        memcpy(&latitude, msgReceived->payload, sizeof(double));
                        memcpy(&longitude, msgReceived->payload + sizeof(double), sizeof(double));
                        
                        RTOS_LOG_INFO("[LORA] Sending position: lat=%.6f, lon=%.6f\n", 
                                     latitude, longitude);
                        
                        // TODO: Aquí iría la lógica real de envío por LoRa
                        // Por ahora simulamos un envío exitoso
                        
                        // Enviar feedback a FSM
                        msgToSend = MessagePool_Allocate();
                        if (msgToSend != NULL) {
                            EmbeddedMessage_Create(msgToSend, 
                                                  MSG_ID_LORA_SEND_POSITION_FEEDBACK,
                                                  MODULE_LORA_TX, 
                                                  MODULE_FSM);
                            
                            osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                                 &msgToSend, 0, 100);
                            if (status == osOK) {
                                RTOS_LOG_DEBUG("[LORA] Feedback sent to FSM\n");
                            } else {
                                RTOS_LOG_ERROR("[LORA] Failed to send feedback (status=%d)\n", status);
                                MessagePool_Free(msgToSend);
                            }
                            msgToSend = NULL;
                        } else {
                            RTOS_LOG_ERROR("[LORA] Failed to allocate feedback message\n");
                        }
                        
                        // Enviar fence de test_data en fragmentos a FSM
#ifdef ENABLE_TEST_MODE
                        // Calcular fragmentación: 3 bytes header + N*16 bytes vertices
                        // Con payload=35: 35-3=32 bytes -> 2 vertices por fragmento
                        const uint8_t VERTEX_SIZE = 2 * sizeof(double);  // 16 bytes
                        const uint8_t HEADER_SIZE = 3;  // fragment_num, total_fragments, vertices_count
                        const uint8_t MAX_VERTICES_PER_MSG = (MAX_MESSAGE_PAYLOAD_SIZE - HEADER_SIZE) / VERTEX_SIZE;
                        
                        uint8_t totalFragments = (TEST_FENCE_VERTEX_COUNT + MAX_VERTICES_PER_MSG - 1) / MAX_VERTICES_PER_MSG;
                        
                        RTOS_LOG_INFO("[LORA] Sending fence in %d fragments (%d vertices, %d per msg)\n", 
                                     totalFragments, TEST_FENCE_VERTEX_COUNT, MAX_VERTICES_PER_MSG);
                        
                        for (uint8_t fragment = 0; fragment < totalFragments; fragment++) {
                            msgToSend = MessagePool_Allocate();
                            if (msgToSend != NULL) {
                                uint8_t startVertex = fragment * MAX_VERTICES_PER_MSG;
                                uint8_t verticesInFragment = MAX_VERTICES_PER_MSG;
                                
                                // Último fragmento puede tener menos vértices
                                if (startVertex + verticesInFragment > TEST_FENCE_VERTEX_COUNT) {
                                    verticesInFragment = TEST_FENCE_VERTEX_COUNT - startVertex;
                                }
                                
                                // Construir payload: [fragment_num][total_fragments][vertices_count][vertex_data...]
                                uint8_t payloadOffset = 0;
                                msgToSend->payload[payloadOffset++] = fragment;
                                msgToSend->payload[payloadOffset++] = totalFragments;
                                msgToSend->payload[payloadOffset++] = verticesInFragment;
                                
                                // Copiar vértices de este fragmento
                                for (uint8_t i = 0; i < verticesInFragment; i++) {
                                    uint8_t vertexIdx = startVertex + i;
                                    memcpy(&msgToSend->payload[payloadOffset], 
                                           &TEST_FENCE_VERTICES[vertexIdx], 
                                           VERTEX_SIZE);
                                    payloadOffset += VERTEX_SIZE;
                                }
                                
                                msgToSend->id = MSG_ID_LORA_VERTEXES_RECEIVED;
                                msgToSend->sender = MODULE_LORA_TX;
                                msgToSend->receiver = MODULE_FSM;
                                msgToSend->length = payloadOffset;
                                
                                osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, 
                                                                     &msgToSend, 0, 100);
                                if (status == osOK) {
                                    RTOS_LOG_DEBUG("[LORA] Fragment %d/%d sent (%d vertices)\n", 
                                                 fragment + 1, totalFragments, verticesInFragment);
                                } else {
                                    RTOS_LOG_ERROR("[LORA] Failed to send fragment %d\n", fragment);
                                    MessagePool_Free(msgToSend);
                                }
                                msgToSend = NULL;
                                
                                // Pequeño delay entre fragmentos para no saturar la cola
                                osDelay(50);
                            } else {
                                RTOS_LOG_ERROR("[LORA] Failed to allocate message for fragment %d\n", fragment);
                                break;
                            }
                        }
#endif
                    } else {
                        RTOS_LOG_WARN("[LORA] Invalid position payload length: %d\n", 
                                     msgReceived->length);
                    }
                    break;
                    
                default:
                    RTOS_LOG_WARN("[LORA] Unhandled message ID:%d\n", msgReceived->id);
                    break;
            }
            
            // Liberar el mensaje recibido
            MessagePool_Free(msgReceived);
            msgReceived = NULL;
        }
        
        // Pequeño delay para evitar monopolizar la CPU
        osDelay(1000);
    }
}