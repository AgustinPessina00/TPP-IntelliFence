#ifndef LORATASK_H
#define LORATASK_H

#include "EmbeddedMessage.h"
#include "stm32wlxx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

// Estructura para datos GPS a enviar por LoRa
typedef struct {
    double latitude;
    double longitude;
    uint8_t zone;      // 0=GREEN, 1=YELLOW, 2=ORANGE, 3=RED, 4=BLACK
    float distance;    // distancia al límite del fence
} LoraGpsData_t;

// Cola externa para comunicación con loraTask (definida en app_freertos.c)
extern osMessageQueueId_t loraTxQueueHandle;

// Función principal de la tarea LoRa
void loraTask(void *argument);

// Función para enviar datos GPS por LoRa (llamada desde fsmTask u otros módulos)
HAL_StatusTypeDef sendGpsDataToLora(LoraGpsData_t *gpsData);

// Función para solicitar envío de datos de forma manual
void loraTriggerSend(void);

#ifdef __cplusplus
}
#endif

#endif // LORATASK_H
