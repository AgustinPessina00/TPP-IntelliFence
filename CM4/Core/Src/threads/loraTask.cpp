#include "threads/loraTask.h"
#include "cmsis_os.h"
#include "lora_app.h"
#include "app_lorawan.h"
#include "LmHandler.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <string.h>

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Cola externa para comunicación con loraTask (definida en app_freertos.c)
extern osMessageQueueId_t loraTxQueueHandle;

// Estado interno de loraTask
// Guarda la ÚLTIMA posición válida recibida para reenviar cada 10 seg
static LoraGpsData_t s_lastValidGpsData = {0, 0}; // Última posición GPS válida
static bool s_hasValidData = false;            // Flag: tiene al menos una posición válida
static bool s_isJoined = false;                // Flag: JOIN exitoso

// Semáforo para sincronización con el thread de LoRa
extern osThreadId_t Thd_LoraSendProcessId;

// ============================================================================
// IMPLEMENTACIÓN DE loraTask
// ============================================================================

/**
 * @brief Tarea principal de LoRa - maneja la recepción de datos GPS y envío por LoRaWAN
 * @param argument Argumento no utilizado
 * 
 * Esta tarea:
 * 1. Inicializa el stack LoRaWAN
 * 2. Espera datos GPS en la cola loraTxQueueHandle (LoraGpsData_t con lat/lon)
 * 3. Cuando hay datos nuevos, dispara el envío por LoRaWAN al gateway
 */
void loraTask(void *argument) {
    (void)argument;
    
    // Inicializar el middleware LoRaWAN (incluye SystemApp_Init que arranca CM0PLUS)
    // NOTA: No usar RTOS_LOG antes de MX_LoRaWAN_Init() para evitar race conditions
    // con la inicialización del mutex de rtos_printf
    MX_LoRaWAN_Init();
    
    RTOS_LOG_INFO("[LORA_TASK] LoRaWAN iniciado, esperando JOIN...\n");
    RTOS_LOG_INFO("[LORA_TASK] Flujo: fsmTask -> dispatcher -> loraTxQueue -> loraTask -> gateway\n");
    
    LoraGpsData_t receivedData;
    
    while (1) {
        // Esperar datos GPS desde dispatcher (timeout de 100ms)
        // Si NO hay datos nuevos, el timer de LoRa reenviará la última posición válida
        if (osMessageQueueGet(loraTxQueueHandle, &receivedData, NULL, 100) == osOK) {
            // Guardar como última posición válida (persistente)
            s_lastValidGpsData = receivedData;
            s_hasValidData = true;
            
            RTOS_LOG_INFO("[LORA_TASK] Nueva posición GPS: lat=%.6f, lon=%.6f\n",
                         receivedData.latitude, receivedData.longitude);
            
            // Si ya estamos conectados a la red, disparar envío inmediato
            if (s_isJoined) {
                loraTriggerSend();
            }
        }
        
        // NOTA: Si no hay datos nuevos, s_lastValidGpsData mantiene la última posición
        // El timer de LoRaWAN (APP_TX_DUTYCYCLE = 10 seg) llamará a loraTaskGetPayload()
        // y enviará la última posición conocida
        
        // Pequeño delay para no saturar el CPU
        osDelay(100);
    }
}

/**
 * @brief Envía datos GPS por LoRa (puede ser llamada desde otros módulos)
 * @param gpsData Puntero a estructura con datos GPS
 * @return HAL_OK si se encola correctamente, HAL_ERROR si falla
 */
HAL_StatusTypeDef sendGpsDataToLora(LoraGpsData_t *gpsData) {
    if (gpsData == NULL) {
        return HAL_ERROR;
    }
    
    // Encolar los datos en la cola de LoRa
    if (osMessageQueuePut(loraTxQueueHandle, gpsData, 0, 100) == osOK) {
        return HAL_OK;
    }
    
    RTOS_LOG_ERROR("[LORA_TASK] Error al encolar datos GPS\n");
    return HAL_ERROR;
}

/**
 * @brief Dispara el envío de datos por LoRa (activa el thread de envío)
 */
void loraTriggerSend(void) {
    if (Thd_LoraSendProcessId != NULL) {
        osThreadFlagsSet(Thd_LoraSendProcessId, 1);
        RTOS_LOG_DEBUG("[LORA_TASK] Envío LoRa disparado\n");
    }
}

// ============================================================================
// FUNCIONES EXPUESTAS A C (extern "C")
// ============================================================================

extern "C" {

/**
 * @brief Callback llamada cuando se completa el JOIN a la red LoRaWAN
 * Esta función debe ser llamada desde lora_app.c en OnJoinRequest
 */
void loraTaskOnJoinSuccess(void) {
    s_isJoined = true;
    RTOS_LOG_INFO("[LORA_TASK] JOIN exitoso! Red LoRaWAN conectada\n");
    
    // Si tenemos una posición válida, enviarla ahora
    if (s_hasValidData) {
        loraTriggerSend();
    }
}

/**
 * @brief Obtiene los últimos datos GPS para enviar
 * Esta función debe ser llamada desde lora_app.c en SendTxData (cada 10 seg)
 * @param buffer Buffer donde se copiarán los datos formateados
 * @param maxSize Tamaño máximo del buffer
 * @return Cantidad de bytes escritos en el buffer
 * 
 * IMPORTANTE: Siempre envía la ÚLTIMA posición válida recibida, incluso si no hay
 * datos nuevos en la cola. Esto permite tracking continuo cada 10 segundos.
 */
uint8_t loraTaskGetPayload(uint8_t *buffer, uint8_t maxSize) {
    if (buffer == NULL || maxSize < 16 || !s_hasValidData) {
        return 0;  // No hay ninguna posición válida todavía
    }
    
    // Formato del payload: [lat:8 bytes][lon:8 bytes]
    // Total: 16 bytes (NO zone, NO distance)
    uint8_t index = 0;
    
    // Copiar latitud (8 bytes - double)
    memcpy(&buffer[index], &s_lastValidGpsData.latitude, sizeof(double));
    index += sizeof(double);
    
    // Copiar longitud (8 bytes - double)
    memcpy(&buffer[index], &s_lastValidGpsData.longitude, sizeof(double));
    index += sizeof(double);
    
    RTOS_LOG_DEBUG("[LORA_TASK] Payload preparado: lat=%.6f, lon=%.6f (%d bytes)\n",
                   s_lastValidGpsData.latitude, s_lastValidGpsData.longitude, index);
    
    // NO limpiamos s_lastValidGpsData para que se reenvíe la próxima vez
    // si no hay datos nuevos (tracking continuo cada 10 seg)
    
    return index;
}

/**
 * @brief Verifica si hay al menos una posición válida para enviar
 * @return true si hay datos válidos, false si no
 */
bool loraTaskHasNewData(void) {
    return s_hasValidData;
}

} // extern "C"
