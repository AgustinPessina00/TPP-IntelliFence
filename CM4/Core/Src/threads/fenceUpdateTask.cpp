#include "FreeRTOS.h"
#include "task.h"
#include "fenceUpdateTask.h"

void fenceUpdateTask(void *argument) {
    for (;;) {
        // TODO: implementar lógica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
