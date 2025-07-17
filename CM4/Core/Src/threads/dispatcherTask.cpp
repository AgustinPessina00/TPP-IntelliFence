#include "FreeRTOS.h"
#include "task.h"
#include "dispatcherTask.h"

void dispatcherTask(void *argument) {
    for (;;) {
        // TODO: implementar l�gica de la tarea
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
