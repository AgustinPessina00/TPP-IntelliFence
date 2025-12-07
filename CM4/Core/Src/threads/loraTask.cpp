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

// Cola para recibir datos GPS desde otros módulos (declarada externa en loraTask.h)
// Inicializada en app_freertos.c

// Buffer para almacenar los últimos datos GPS recibidos
static LoraGpsData_t s_lastGpsData = {0};
static bool s_hasNewData = false;
static bool s_isJoined = false;

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
    
    LoraGpsData_t receivedData;
    
    while (1) {
        // Esperar datos GPS en la cola (timeout de 100ms)
        if (osMessageQueueGet(loraTxQueueHandle, &receivedData, NULL, 100) == osOK) {
            // Guardar los datos recibidos
            memcpy(&s_lastGpsData, &receivedData, sizeof(LoraGpsData_t));
            s_hasNewData = true;
            
            RTOS_LOG_INFO("[LORA_TASK] Datos GPS recibidos: lat=%.6f, lon=%.6f, zone=%d, dist=%.2f\n",
                         receivedData.latitude, receivedData.longitude, 
                         receivedData.zone, receivedData.distance);
            
            // Si ya estamos conectados a la red, disparar envío inmediato
            if (s_isJoined) {
                loraTriggerSend();
            }
        }
        
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
    
    // Si tenemos datos pendientes, enviarlos ahora
    if (s_hasNewData) {
        loraTriggerSend();
    }
}

/**
 * @brief Obtiene los últimos datos GPS para enviar
 * Esta función debe ser llamada desde lora_app.c en SendTxData
 * @param buffer Buffer donde se copiarán los datos formateados
 * @param maxSize Tamaño máximo del buffer
 * @return Cantidad de bytes escritos en el buffer
 */
uint8_t loraTaskGetPayload(uint8_t *buffer, uint8_t maxSize) {
    if (buffer == NULL || maxSize < 17 || !s_hasNewData) {
        return 0;
    }
    
    // Formato del payload: [lat:8 bytes][lon:8 bytes][zone:1 byte]
    // Total: 17 bytes
    uint8_t index = 0;
    
    // Copiar latitud (8 bytes - double)
    memcpy(&buffer[index], &s_lastGpsData.latitude, sizeof(double));
    index += sizeof(double);
    
    // Copiar longitud (8 bytes - double)
    memcpy(&buffer[index], &s_lastGpsData.longitude, sizeof(double));
    index += sizeof(double);
    
    // Copiar zona (1 byte)
    buffer[index] = s_lastGpsData.zone;
    index += 1;
    
    RTOS_LOG_DEBUG("[LORA_TASK] Payload preparado: %d bytes\n", index);
    
    // Marcar que ya se enviaron estos datos
    s_hasNewData = false;
    
    return index;
}

/**
 * @brief Verifica si hay datos nuevos listos para enviar
 * @return true si hay datos, false si no
 */
bool loraTaskHasNewData(void) {
    return s_hasNewData;
}

} // extern "C"
