
#include "FreeRTOS.h"
#include "task.h"
#include "threads/loraRxTask.h"

extern osMessageQueueId_t loraRxQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void loraRxTask(void *argument) {
	//Message* msgReceived;

	while (1) {

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
