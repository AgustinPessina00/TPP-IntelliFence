#include "FreeRTOS.h"
#include "task.h"
#include "loraTxTask.h"

void loraTxTask(void *argument) {
    for (;;) {
        // TODO: implementar lógica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
