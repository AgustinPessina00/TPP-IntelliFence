#include "FreeRTOS.h"
#include "task.h"
#include "loraRxTask.h"

void loraRxTask(void *argument) {
    for (;;) {
        // TODO: implementar l�gica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
