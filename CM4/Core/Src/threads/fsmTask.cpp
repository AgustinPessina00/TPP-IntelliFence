#include "FreeRTOS.h"
#include "task.h"
#include "fsmTask.h"

void fsmTask(void *argument) {
    for (;;) {
        // TODO: implementar lógica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
