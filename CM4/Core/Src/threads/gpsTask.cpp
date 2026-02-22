#include "FreeRTOS.h"
#include "sam_m10q.h"
#include "task.h"
#include "gpsTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "gps_data.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t gpsQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void gpsTask(void *argument) {
    // CRITICAL: Use static objects to avoid stack overflow
    static SamM10q gps;
    
    // Explicit initialization - must be called once at task startup
    if (!gps.init(gpsAddress)) {
        RTOS_LOG_ERROR("[GPS_TASK] Failed to initialize GPS\r\n");
        // Retry or handle error appropriately
    }

    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    gpsRateSpeed rateGPS;
    gpsData_t gpsData = {0.0, 0.0, 0};
    
    RTOS_LOG_INFO("[GPS_TASK] Task initialized successfully\r\n");
      
    static uint32_t stackMonitorCounter = 0;
    
    while(1) {
        // Monitorear stack cada ~10 segundos
        // if (++stackMonitorCounter >= 10) {
        //     UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        //     RTOS_LOG_INFO("[GPS_TASK] Stack libre: %u words (%u bytes)\r\n", 
        //                  stackLeft, stackLeft * 4);
        //     stackMonitorCounter = 0;
        // }
        // Monitorear stack cada ~10 segundos
        // if (++stackMonitorCounter >= 10) {
        //     UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        //     RTOS_LOG_INFO("[GPS_TASK] Stack libre: %u words (%u bytes)\r\n", 
        //                  stackLeft, stackLeft * 4);
        //     stackMonitorCounter = 0;
        // }

        // Verificar si hay mensajes de solicitud
        if (osMessageQueueGet(gpsQueueHandle, &msgReceived, NULL, osWaitForever) == osOK) {
            RTOS_LOG_DEBUG("[GPS_TASK] Received message ID:%d from module:%d\r\n", msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_REQUEST_GPS:
                    // Leer GPS solo cuando se solicita
                    gps.read_gps_position();
                    gpsData.latitude = gps.latitude;
                    gpsData.longitude = gps.longitude;
                    gpsData.fix = gps.flags & 0x01; // Bit 0 indica si hay fix
                    RTOS_LOG_DEBUG("[GPS_TASK] GPS read: lat %.6f, lon %.6f, fix %d\r\n", gps.latitude, gps.longitude, gpsData.fix);
                    
                    if(gpsData.fix) {
                        msgToSend = MessagePool_Allocate();
                        if (msgToSend != NULL) {
                            RTOS_LOG_DEBUG("[GPS_TASK] alocado\r\n");
                            EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_GPS, MODULE_GPS, MODULE_FSM, (uint8_t*)&gpsData, sizeof(gpsData_t));
                            osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                            RTOS_LOG_DEBUG("[GPS_TASK] Sent GPS data to FSM\r\n");
                            msgToSend = NULL;
                        }
                        else {
                            RTOS_LOG_DEBUG("[GPS_TASK] se lleno la pile\r\n");
                        }
                    }
                    break;

                case MSG_ID_GPS_REQUEST_CONFIG:
                    rateGPS = static_cast<gpsRateSpeed>(msgReceived->payload[0]);
                    if (gps.set_new_acq_time(rateGPS)) {
                        RTOS_LOG_DEBUG("[GPS_TASK] GPS acquisition time set to %d\r\n", static_cast<int>(rateGPS));
                    }
                    else {
                        RTOS_LOG_WARN("[GPS_TASK] Failed to set GPS acquisition time\r\n");
                    }
                    break;

                case MSG_ID_REQUEST_GPS_CONFIGURATION_PSM:
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        gps.configure_gps(40, M10Q_NUM_DATA_ELEMENTS);
                        EmbeddedMessage_Create(msgToSend, MSG_ID_SEND_GPS_CONFIGURATION_PSM, MODULE_SENSOR_ACQ, MODULE_FSM);
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent GPS Confirmation PSMOO to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_GPS:
                    gps.read_gps_position();
                    gpsData.latitude = gps.latitude;
                    gpsData.longitude = gps.longitude;
                    gpsData.fix = gps.flags & 0x01;
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_GPS_DATA, 
                                                         MODULE_GPS, MODULE_CONSOLE, 
                                                         (uint8_t*)&gpsData, sizeof(gpsData_t));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[GPS_TASK] Sent GPS data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
        
                default:
                    RTOS_LOG_WARN("[GPS_TASK] Unknown message ID: %d\r\n", msgReceived->id);
                    break;
            }
      
            MessagePool_Free(msgReceived);
            msgReceived = NULL;
        }

        //osDelay(1500);
    }
}
