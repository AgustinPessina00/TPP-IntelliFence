#include "FreeRTOS.h"
#include "task.h"
#include "threads/dispatcherTask.h"
#include "cmsis_os.h"
#include "messages.h"

extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;
extern osMessageQueueId_t gpsQueueHandle;
extern osMessageQueueId_t loraTxQueueHandle;
extern osMessageQueueId_t loraRxQueueHandle;
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t distanceToLimitQueueHandle;
extern osMessageQueueId_t fenceUpdateQueueHandle;

void dispatcherTask(void *argument) {
    Message* msg;

    while(1) {
        if (osMessageQueueGet(dispatcherQueueHandle, &msg, NULL, osWaitForever) == osOK) {
            switch (msg->receiver) {
                case ModuleId_t::SENSOR_ACQ:
                    osMessageQueuePut(sensorAcqQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::STIMULUS:
                    osMessageQueuePut(stimulusQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::GPS:
                    osMessageQueuePut(gpsQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::LORA_TX:
                    osMessageQueuePut(loraTxQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::LORA_RX:
                    osMessageQueuePut(loraRxQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::FSM:
                    osMessageQueuePut(fsmQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::DISTANCE:
                    osMessageQueuePut(distanceToLimitQueueHandle, msg, 0, 0);
                    break;
                case ModuleId_t::FENCE_UPDATE:
                    osMessageQueuePut(fenceUpdateQueueHandle, msg, 0, 0);
                    break;
                default:
                    break;
            }
        }
    }
}
