
#include "FreeRTOS.h"
#include "task.h"
#include "threads/loraRxTask.h"
#include "fence.h"

#include <string.h>
#include <stdio.h>

extern osMessageQueueId_t loraRxQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void loraRxTask(void *argument) {
	Message* msgReceived;

	while (1) {
		msgReceived = nullptr;  // se reinicia el puntero antes de recibir algo
		Message* msgToSend = nullptr;

		if (osMessageQueueGet(dispatcherQueueHandle, &msgReceived, NULL, osWaitForever) == osOK) {
			switch (msgReceived->id) {
				case MSG_ID_LORA_RX:
					msgToSend = new Message(MSG_ID_LORA_VERTEXES_RECEIVED, ModuleId_t::LORA_RX, ModuleId_t::FSM, msgReceived->length);
					osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
					break;
			}
			delete msgReceived;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

