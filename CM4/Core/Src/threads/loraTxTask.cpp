
#include "FreeRTOS.h"
#include "task.h"
#include "stm32wlxx_hal.h"
#include "threads/loraTxTask.h"
#include "LmHandler.h"
#include "region.h"

#include <stdint.h>
#include <string.h>

extern osMessageQueueId_t loraTxQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

HAL_StatusTypeDef SendCoordinates(double latitude, double longitude);

void loraTxTask(void *argument) {
	Message* msgReceived;


	while (1) {
		msgReceived = nullptr;  // se reinicia el puntero antes de recibir algo

		Message *msgToSend = nullptr;

		double latitude;
		double longitude;

		if (osMessageQueueGet(loraTxQueueHandle, &msgReceived, NULL, 0) == osOK) {
		  switch (msgReceived->id) {
			case MSG_ID_LORA_SEND_POSITION:
				memcpy(&latitude, msgReceived->payload, sizeof(double));
				memcpy(&longitude, msgReceived->payload + sizeof(double), sizeof(double));

				if(SendCoordinates(latitude, longitude) == HAL_OK) {
					msgToSend = new Message(MSG_ID_LORA_SEND_POSITION_FEEDBACK, ModuleId_t::LORA_TX, ModuleId_t::FSM, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
					osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
				}
				break;
		  }
		  delete msgReceived;
		}
        vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

HAL_StatusTypeDef SendCoordinates(double latitude, double longitude) {
	HAL_StatusTypeDef status = HAL_ERROR;

	uint8_t payload[16];   // 2 doubles de 8 bytes cada uno

	memcpy(&payload[0],  &latitude,  sizeof(double));
	memcpy(&payload[8],  &longitude, sizeof(double));

	// Preparar la estructura de datos
	LmHandlerAppData_t appData;
	appData.Port = 2; // Puerto LoRaWAN (usa uno que tengas permitido)
	appData.Buffer = payload;
	appData.BufferSize = sizeof(payload); // 16 bytes

	if (LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG, false) == LORAMAC_HANDLER_SUCCESS) {
		status = HAL_OK;
	}

    return status;
}
