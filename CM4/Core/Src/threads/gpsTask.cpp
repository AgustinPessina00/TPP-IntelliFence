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
#include "gpio.h"

// Declaraciones externas de las colas
extern osMessageQueueId_t gpsQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

// Variable global para medir tiempo desde adquisición GPS hasta aplicación de estímulo
static uint32_t gpsAcquisitionTimestamp = 0;

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
    gpsData_t gpsData = {0.0, 0.0, 0, 0};
    
    RTOS_LOG_INFO("[GPS_TASK] Task initialized successfully\r\n");
      
    static uint32_t stackMonitorCounter = 0;
    static uint32_t iTow = 0; // Variable para monitorear iTOW y detectar reinicios del GPS
    bool gpsConfigured = false;
    
    while(1) {
        //RTOS_LOG_DEBUG("[GPS_TASK] ENTER GPS_TASK\r\n");
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
                case MSG_ID_REQUEST_GPS: {
                    // Leer GPS solo cuando se solicita
                    gps.read_gps_position();
                    // Guardar timestamp de adquisición para medir latencia
                    gpsAcquisitionTimestamp = osKernelGetTickCount();
                    gpsData.latitude = gps.latitude;
                    gpsData.longitude = gps.longitude;
                    gpsData.fix = gps.flags & 0x01; // Bit 0 indica si hay fix
                    gpsData.psmStateActive = gps.psmStateActive; // PSM State (0 = INACTIVE, 1 = ACTIVE)

                    // // Valores Hardcodeados para pruebas de integración - reemplazar con gps.latitude, gps.longitude, gps.flags
                    // gpsData.latitude = -34.57050809152076;
                    // gpsData.longitude = -58.44418995925609;
                    // gpsData.fix = 1; // Bit 0 indica si hay fix
                    // gpsData.psmStateActive = gps.psmStateActive; // PSM State (0 = INACTIVE, 1 = ACTIVE)
                    if (gpsData.psmStateActive)
                        RTOS_LOG_DEBUG("[GPS_TASK] GPS read: lat %.6f, lon %.6f, GPS ON,  fix %d\r\n", gpsData.latitude, gpsData.longitude, gpsData.fix);
                    else
                        RTOS_LOG_DEBUG("[GPS_TASK] GPS read: lat %.6f, lon %.6f, GPS OFF, fix %d\r\n", gpsData.latitude, gpsData.longitude, gpsData.fix);
                    if(gpsData.fix && gps.iTow != iTow) { // Solo enviar si hay fix y iTOW ha cambiado (nuevo dato) 
                    //if(gpsData.fix) { // Elimino el iTow para test. 
                        msgToSend = MessagePool_Allocate();
                        if (msgToSend != NULL) {
                            RTOS_LOG_DEBUG("[GPS_TASK] alocado\r\n");
                            // Almacenar timestamp en los primeros 4 bytes del payload
                            uint8_t payloadWithTimestamp[sizeof(gpsData_t) + sizeof(uint32_t)];
                            memcpy(payloadWithTimestamp, &gpsAcquisitionTimestamp, sizeof(uint32_t));
                            memcpy(payloadWithTimestamp + sizeof(uint32_t), &gpsData, sizeof(gpsData_t));
                            EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_GPS, MODULE_GPS, MODULE_FSM, payloadWithTimestamp, sizeof(payloadWithTimestamp));
                            osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                            RTOS_LOG_DEBUG("[GPS_TASK] Sent GPS data to FSM\r\n");
                            msgToSend = NULL;
                        }
                        else {
                            RTOS_LOG_DEBUG("[GPS_TASK] se lleno la pile\r\n");
                        }
                        iTow = gps.iTow; // Actualizar iTOW
                    }
                    
                    break;
                }

                case MSG_ID_GPS_REQUEST_CONFIG:
                    gpsConfigured = false; // Reiniciar flag de configuración para forzar reconfiguración en cada solicitud
                    // gpsData.latitude = -34.57050809152076;
                    // gpsData.longitude = -58.44418995925609;
                    // gpsData.fix = 1; // Bit 0 indica si hay fix
                    // gpsData.psmStateActive = gps.psmStateActive; // PSM State (0 = INACTIVE, 1 = ACTIVE)
                    // while(!gpsConfigured) {
                    //     gps.read_gps_position();
                    //     gpsData.latitude = gps.latitude;
                    //     gpsData.longitude = gps.longitude;
                    //     gpsData.fix = gps.flags & 0x01; // Bit 0 indica si hay fix
                    //     gpsData.psmStateActive = gps.psmStateActive; // PSM State (0 = INACTIVE, 1 = ACTIVE)
                    //     if(gpsData.psmStateActive) {
                    //         RTOS_LOG_WARN("[GPS_TASK] GPS is in INACTIVE PSM state\r\n");
                    //         // gps.set_new_acq_time(gpsRateSpeed::CONTINUOUS); // Intentar configurar tiempo de adquisición cercano al límite para activar PSMOO
                    //         // HAL_GPIO_WritePin(GPS_EXTINT_GPIO_Port, GPS_EXTINT_Pin, GPIO_PIN_RESET);
                    //         // HAL_Delay(50);
                    //         rateGPS = static_cast<gpsRateSpeed>(msgReceived->payload[0]);
                            
                    //         if (gps.set_new_acq_time(rateGPS)) {
                    //             RTOS_LOG_DEBUG("[GPS_TASK] GPS acquisition time set to %d\r\n", static_cast<int>(rateGPS));
                    //             msgToSend = MessagePool_Allocate();
                    //             if (msgToSend != NULL) {
                    //                 EmbeddedMessage_Create(msgToSend, MSG_ID_GPS_CONFIG_RESPONSE, MODULE_GPS, MODULE_FSM);
                    //                 osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                    //                 RTOS_LOG_DEBUG("[GPS_TASK] Sent GPS Confirmation NEW Config to FSM\r\n");
                    //                 msgToSend = NULL;
                    //                 gpsConfigured = true;
                    //             }
                    //         }
                    //         else {
                    //             RTOS_LOG_WARN("[GPS_TASK] Failed to set GPS acquisition time\r\n");
                    //         }
                    //     }
                    //     else {
                    //         RTOS_LOG_WARN("[GPS_TASK] Cannot set GPS acquisition time while in INACTIVE PSM state\r\n");
                    //         osDelay(500);
                    //     }

                    // }
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_Create(msgToSend, MSG_ID_GPS_CONFIG_RESPONSE, MODULE_GPS, MODULE_FSM);
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        msgToSend = NULL;
                    }
                    
                    break;

                case MSG_ID_REQUEST_GPS_CONFIGURATION_PSM:
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        //gps.configure_gps(M10Q_NUM_INIT_PSM_DATA_ELEMENTS, M10Q_NUM_DATA_ELEMENTS);
                        EmbeddedMessage_Create(msgToSend, MSG_ID_SEND_GPS_CONFIGURATION_PSM, MODULE_GPS, MODULE_FSM);
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        //RTOS_LOG_DEBUG("[GPS_TASK] Sent GPS Confirmation PSMOO to FSM\r\n");
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
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_GPS_DATA, MODULE_GPS, MODULE_CONSOLE, (uint8_t*)&gpsData, sizeof(gpsData_t));
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
