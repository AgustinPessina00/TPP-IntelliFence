#include "FreeRTOS.h"
#include "task.h"
#include "threads/fenceUpdateTask.h"

void fenceUpdateTask(void *argument) {
    for (;;) {
        // TODO: Eliminar esta tarea porque ya lo hace FSM::FENCE_TRANSITION_STATE
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
