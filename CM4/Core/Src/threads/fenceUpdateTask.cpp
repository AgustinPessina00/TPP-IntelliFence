#include "FreeRTOS.h"
#include "task.h"
#include "threads/fenceUpdateTask.h"

void fenceUpdateTask(void *argument) {
    for (;;) {
        // TODO: implementar l�gica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
