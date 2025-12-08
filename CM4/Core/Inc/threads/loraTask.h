#ifndef LORATASK_H
#define LORATASK_H

#include "EmbeddedMessage.h"
#include "stm32wlxx_hal.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

// Estructura para datos GPS a enviar por LoRa
// SOLO latitud y longitud (16 bytes totales)
// El dispatcher redirige estos datos desde fsmTask a loraTxQueue
typedef struct {
    double latitude;   // 8 bytes
    double longitude;  // 8 bytes
} LoraGpsData_t;

// Función principal de la tarea LoRa
void loraTask(void *argument);

// Función para enviar datos GPS por LoRa (llamada desde fsmTask u otros módulos)
HAL_StatusTypeDef sendGpsDataToLora(LoraGpsData_t *gpsData);

// Función para solicitar envío de datos de forma manual
void loraTriggerSend(void);

// Callback llamada cuando se completa el JOIN a la red LoRaWAN
void loraTaskOnJoinSuccess(void);

// Obtiene los últimos datos GPS para enviar
uint8_t loraTaskGetPayload(uint8_t *buffer, uint8_t maxSize);

// Verifica si hay datos nuevos listos para enviar
bool loraTaskHasNewData(void);

#ifdef __cplusplus
}
#endif

#endif // LORATASK_H
