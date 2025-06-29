#include "FreeRTOS.h"
#include "task.h"
#include "distanceTask.h"

void distanceTask(void *argument) {
    for (;;) {
        // TODO: implementar lógica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
