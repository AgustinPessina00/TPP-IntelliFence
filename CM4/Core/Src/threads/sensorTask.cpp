#include "FreeRTOS.h"
#include "task.h"
#include "sensorTask.h"

void sensorTask(void *argument) {
    for (;;) {
        // TODO: implementar l�gica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
