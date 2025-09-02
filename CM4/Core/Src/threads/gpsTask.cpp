
#include "threads/gpsTask.h"

extern osMessageQueueId_t gpsQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void gpsTask(void *argument) {
	gpsAcqTaskParams *gpsParams = static_cast<gpsAcqTaskParams *>(argument);

    while (1) {
        Message* msgReceived = nullptr;
        Message* msgToSend = nullptr;
        gpsRateSpeed rateGPS;

        if (osMessageQueueGet(gpsQueueHandle, &msgReceived, NULL, 0) == osOK) {
            switch (msgReceived->id) {
                case MSG_ID_GPS_REQUEST_CONFIG:
                    memcpy(&rateGPS, msgReceived->payload, sizeof(gpsRateSpeed));

                    if(gpsParams->gps->set_new_acq_time(rateGPS)) {
                        msgToSend = new Message(MSG_ID_GPS_CONFIG_RESPONSE, ModuleId_t::GPS, ModuleId_t::FSM, 0);
                    }
                    break;

                default:
                    break;
            }

            delete msgReceived;
	    }

        if(msgToSend)
            osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

    }

    vTaskDelay(pdMS_TO_TICKS(1000));
}
