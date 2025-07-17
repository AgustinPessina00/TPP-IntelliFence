#include "FreeRTOS.h"
#include "task.h"
#include "sensorAcqTask.h"

void sensorAcqTask(void *argument) {
    for (;;) {
        // TODO: implementar l�gica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
