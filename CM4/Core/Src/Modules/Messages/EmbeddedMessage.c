#include "EmbeddedMessage.h"
#include "FreeRTOS.h"
#include "task.h"

// Pool global de mensajes
static MessagePool_t g_message_pool;
static uint8_t g_pool_initialized = 0;

// ====== Implementación del Pool ======

MessageResult_t MessagePool_Init(void) {
    if (g_pool_initialized) {
        return MSG_RESULT_OK; // Ya inicializado
    }
    
    // Limpiar toda la estructura
    memset(&g_message_pool, 0, sizeof(MessagePool_t));
    
    // Inicializar todos los mensajes como no usados
    for (uint32_t i = 0; i < MESSAGE_POOL_SIZE; i++) {
        g_message_pool.messages[i].in_use = 0;
    }
    
    g_message_pool.next_free_index = 0;
    g_message_pool.allocated_count = 0;
    g_message_pool.max_allocated = 0;
    
    // Crear mutex para thread safety
    const osMutexAttr_t mutex_attr = {
        .name = "MessagePoolMutex"
    };
    g_message_pool.pool_mutex = osMutexNew(&mutex_attr);
    
    if (g_message_pool.pool_mutex == NULL) {
        return MSG_RESULT_ERROR_INVALID_PARAM;
    }
    
    g_pool_initialized = 1;
    return MSG_RESULT_OK;
}

EmbeddedMessage_t* MessagePool_Allocate(void) {
    if (!g_pool_initialized) {
        return NULL;
    }
    
    // Thread-safe allocation
    if (osMutexAcquire(g_message_pool.pool_mutex, osWaitForever) != osOK) {
        return NULL;
    }
    
    EmbeddedMessage_t* allocated_msg = NULL;
    
    // Buscar el siguiente mensaje libre
    for (uint32_t i = 0; i < MESSAGE_POOL_SIZE; i++) {
        uint32_t check_index = (g_message_pool.next_free_index + i) % MESSAGE_POOL_SIZE;
        
        if (g_message_pool.messages[check_index].in_use == 0) {
            allocated_msg = &g_message_pool.messages[check_index];
            allocated_msg->in_use = 1;
            
            // Limpiar el mensaje
            memset(allocated_msg, 0, sizeof(EmbeddedMessage_t));
            allocated_msg->in_use = 1; // Restaurar flag después del memset
            allocated_msg->timestamp = osKernelGetTickCount();
            
            // Actualizar estadísticas
            g_message_pool.allocated_count++;
            if (g_message_pool.allocated_count > g_message_pool.max_allocated) {
                g_message_pool.max_allocated = g_message_pool.allocated_count;
            }
            
            // Actualizar índice para próxima búsqueda
            g_message_pool.next_free_index = (check_index + 1) % MESSAGE_POOL_SIZE;
            break;
        }
    }
    
    osMutexRelease(g_message_pool.pool_mutex);
    return allocated_msg;
}

MessageResult_t MessagePool_Free(EmbeddedMessage_t* msg) {
    if (!g_pool_initialized || msg == NULL) {
        return MSG_RESULT_ERROR_MESSAGE_NULL;
    }
    
    // Verificar que el mensaje pertenece al pool
    if (msg < &g_message_pool.messages[0] || 
        msg > &g_message_pool.messages[MESSAGE_POOL_SIZE - 1]) {
        return MSG_RESULT_ERROR_INVALID_PARAM;
    }
    
    // Thread-safe deallocation
    if (osMutexAcquire(g_message_pool.pool_mutex, osWaitForever) != osOK) {
        return MSG_RESULT_ERROR_INVALID_PARAM;
    }
    
    if (msg->in_use == 1) {
        msg->in_use = 0;
        g_message_pool.allocated_count--;
        
        // Limpiar datos sensibles (opcional, para seguridad)
        memset(msg->payload, 0, MAX_MESSAGE_PAYLOAD_SIZE);
    }
    
    osMutexRelease(g_message_pool.pool_mutex);
    return MSG_RESULT_OK;
}

// ====== API de Mensajes ======

MessageResult_t EmbeddedMessage_Create(EmbeddedMessage_t* msg, 
                                     uint8_t msg_id, 
                                     ModuleId_t sender, 
                                     ModuleId_t receiver) {
    if (msg == NULL) {
        return MSG_RESULT_ERROR_MESSAGE_NULL;
    }
    
    msg->id = msg_id;
    msg->sender = sender;
    msg->receiver = receiver;
    msg->length = 0;
    msg->timestamp = osKernelGetTickCount();
    
    return MSG_RESULT_OK;
}

MessageResult_t EmbeddedMessage_CreateWithPayload(EmbeddedMessage_t* msg,
                                                 uint8_t msg_id,
                                                 ModuleId_t sender,
                                                 ModuleId_t receiver,
                                                 const uint8_t* data,
                                                 uint8_t length) {
    if (msg == NULL) {
        return MSG_RESULT_ERROR_MESSAGE_NULL;
    }
    
    if (length > MAX_MESSAGE_PAYLOAD_SIZE) {
        return MSG_RESULT_ERROR_PAYLOAD_TOO_LARGE;
    }
    
    msg->id = msg_id;
    msg->sender = sender;
    msg->receiver = receiver;
    msg->length = length;
    msg->timestamp = osKernelGetTickCount();
    
    if (data != NULL && length > 0) {
        memcpy(msg->payload, data, length);
    }
    
    return MSG_RESULT_OK;
}

MessageResult_t EmbeddedMessage_Copy(EmbeddedMessage_t* dest, const EmbeddedMessage_t* src) {
    if (dest == NULL || src == NULL) {
        return MSG_RESULT_ERROR_MESSAGE_NULL;
    }
    
    // Preservar el flag in_use del destino
    uint8_t dest_in_use = dest->in_use;
    
    // Copiar toda la estructura
    memcpy(dest, src, sizeof(EmbeddedMessage_t));
    
    // Restaurar el flag in_use original del destino
    dest->in_use = dest_in_use;
    
    return MSG_RESULT_OK;
}

MessageResult_t EmbeddedMessage_SetPayload(EmbeddedMessage_t* msg, const uint8_t* data, uint8_t length) {
    if (msg == NULL) {
        return MSG_RESULT_ERROR_MESSAGE_NULL;
    }
    
    if (length > MAX_MESSAGE_PAYLOAD_SIZE) {
        return MSG_RESULT_ERROR_PAYLOAD_TOO_LARGE;
    }
    
    msg->length = length;
    if (data != NULL && length > 0) {
        memcpy(msg->payload, data, length);
    }
    
    return MSG_RESULT_OK;
}

// ====== Utilidades y Debugging ======

void MessagePool_GetStats(uint32_t* total_messages, uint32_t* allocated_count, uint32_t* max_allocated) {
    if (!g_pool_initialized) {
        if (total_messages) *total_messages = 0;
        if (allocated_count) *allocated_count = 0;
        if (max_allocated) *max_allocated = 0;
        return;
    }
    
    if (osMutexAcquire(g_message_pool.pool_mutex, osWaitForever) == osOK) {
        if (total_messages) *total_messages = MESSAGE_POOL_SIZE;
        if (allocated_count) *allocated_count = g_message_pool.allocated_count;
        if (max_allocated) *max_allocated = g_message_pool.max_allocated;
        osMutexRelease(g_message_pool.pool_mutex);
    }
}

void MessagePool_Reset(void) {
    if (!g_pool_initialized) {
        return;
    }
    
    if (osMutexAcquire(g_message_pool.pool_mutex, osWaitForever) == osOK) {
        // Limpiar todos los mensajes
        for (uint32_t i = 0; i < MESSAGE_POOL_SIZE; i++) {
            g_message_pool.messages[i].in_use = 0;
        }
        
        g_message_pool.next_free_index = 0;
        g_message_pool.allocated_count = 0;
        
        osMutexRelease(g_message_pool.pool_mutex);
    }
}