#ifndef EMBEDDED_MESSAGE_H
#define EMBEDDED_MESSAGE_H

#include <stdint.h>
#include <string.h>
#include "messages_id.h"
#include "cmsis_os2.h"

// Configuración del sistema de mensajes
#define MAX_MESSAGE_PAYLOAD_SIZE   35      // Tamaño máximo de payload (aumentado para fragmentación)
#define MESSAGE_POOL_SIZE          16      // Pool de mensajes estáticos
#define MAX_MESSAGE_QUEUES         10      // Máximo número de colas

// Enum para módulos (mantenemos compatibilidad)
typedef enum {
    MODULE_SENSOR_ACQ   = 1,
    MODULE_STIMULUS     = 2,
    MODULE_GPS          = 3,
    MODULE_LORA_TX      = 4,
    MODULE_LORA_RX      = 5,
    MODULE_FSM          = 6,
    MODULE_DISTANCE     = 7,
    MODULE_FENCE_UPDATE = 8,
    MODULE_DISPATCHER   = 9
} ModuleId_t;

// Estructura de mensaje embedded-friendly
typedef struct {
    uint8_t id;                                    // Message ID
    ModuleId_t sender;                            // Sender module
    ModuleId_t receiver;                          // Receiver module
    uint8_t length;                               // Payload length
    uint8_t payload[MAX_MESSAGE_PAYLOAD_SIZE];    // Static payload buffer
    uint8_t in_use;                              // Pool management flag
} EmbeddedMessage_t;

// Resultado de operaciones
typedef enum {
    MSG_RESULT_OK = 0,
    MSG_RESULT_ERROR_POOL_FULL,
    MSG_RESULT_ERROR_INVALID_PARAM,
    MSG_RESULT_ERROR_PAYLOAD_TOO_LARGE,
    MSG_RESULT_ERROR_MESSAGE_NULL
} MessageResult_t;

// Pool de mensajes estático
typedef struct {
    EmbeddedMessage_t messages[MESSAGE_POOL_SIZE];
    uint8_t next_free_index;
    osMutexId_t pool_mutex;
    uint32_t allocated_count;
    uint32_t max_allocated;  // Para estadísticas
} MessagePool_t;

#ifdef __cplusplus
extern "C" {
#endif

// ====== API del Sistema de Mensajes ======

// Inicialización del pool de mensajes
MessageResult_t MessagePool_Init(void);

// Obtener un mensaje del pool
EmbeddedMessage_t* MessagePool_Allocate(void);

// Liberar un mensaje al pool
MessageResult_t MessagePool_Free(EmbeddedMessage_t* msg);

// Crear mensaje básico
MessageResult_t EmbeddedMessage_Create(EmbeddedMessage_t* msg, 
                                     uint8_t msg_id, 
                                     ModuleId_t sender, 
                                     ModuleId_t receiver);

// Crear mensaje con payload
MessageResult_t EmbeddedMessage_CreateWithPayload(EmbeddedMessage_t* msg,
                                                 uint8_t msg_id,
                                                 ModuleId_t sender,
                                                 ModuleId_t receiver,
                                                 const uint8_t* data,
                                                 uint8_t length);

// Copiar mensaje (sin dynamic allocation)
MessageResult_t EmbeddedMessage_Copy(EmbeddedMessage_t* dest, const EmbeddedMessage_t* src);

// Configurar payload
MessageResult_t EmbeddedMessage_SetPayload(EmbeddedMessage_t* msg, const uint8_t* data, uint8_t length);

// Obtener estadísticas del pool
void MessagePool_GetStats(uint32_t* total_messages, uint32_t* allocated_count, uint32_t* max_allocated);

// Reset del pool (para debugging)
void MessagePool_Reset(void);

#ifdef __cplusplus
}
#endif

#endif // EMBEDDED_MESSAGE_H