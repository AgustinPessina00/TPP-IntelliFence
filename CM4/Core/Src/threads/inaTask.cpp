#include "FreeRTOS.h"
#include "ina226.h"
#include "task.h"
#include "inaTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t inaQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void inaTask(void *argument) {
    // CRITICAL: Use static objects to avoid stack overflow
    static Ina226 inaGps;
    static Ina226 inaImu;
    static Ina226 inaMcu;
    
    // Explicit initialization - must be called once at task startup
    if (!inaGps.init(configs[gpsIndex].address, configs[gpsIndex].rShunt, 
                     configs[gpsIndex].currentLSB, Ina226Averaging::AVG_64,
             Ina226BusConvTime::CT_1_1MS,
             Ina226ShuntConvTime::CT_1_1MS,
             Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[INA_TASK] Failed to initialize INA226 GPS\r\n");
    }
    
    if (!inaImu.init(configs[imuIndex].address, configs[imuIndex].rShunt, 
                     configs[imuIndex].currentLSB, Ina226Averaging::AVG_64,
            Ina226BusConvTime::CT_1_1MS,
            Ina226ShuntConvTime::CT_1_1MS,
            Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[INA_TASK] Failed to initialize INA226 IMU\r\n");
    }
    
    if (!inaMcu.init(configs[mcuIndex].address, configs[mcuIndex].rShunt, 
                     configs[mcuIndex].currentLSB, Ina226Averaging::AVG_128, 
                     Ina226BusConvTime::CT_1_1MS, Ina226ShuntConvTime::CT_1_1MS, 
                     Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[INA_TASK] Failed to initialize INA226 MCU\r\n");
    }

    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    
    RTOS_LOG_INFO("[INA_TASK] Task initialized successfully\r\n");
    
    static uint32_t stackMonitorCounter = 0;
    
    while(1) {
        // Monitorear stack cada ~10 segundos
        // if (++stackMonitorCounter >= 10) {
        //     UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        //     RTOS_LOG_INFO("[INA_TASK] Stack libre: %u words (%u bytes)\r\n", 
        //                  stackLeft, stackLeft * 4);
        //     stackMonitorCounter = 0;
        // }

        RTOS_LOG_DEBUG("------------------------------------------------------------\r\n");
        inaGps.readCurrent_mA();
        RTOS_LOG_DEBUG("[SENSOR_ACQ] INA GPS current read: %.3f mA\r\n", inaGps.current);
        inaImu.readCurrent_mA();
        RTOS_LOG_DEBUG("[SENSOR_ACQ] INA IMU current read: %.3f mA\r\n", inaImu.current);
        inaMcu.readCurrent_mA();
        RTOS_LOG_DEBUG("[SENSOR_ACQ] INA MCU current read: %.3f mA\r\n", inaMcu.current);
        RTOS_LOG_DEBUG("------------------------------------------------------------\r\n");

        // Verificar si hay mensajes de solicitud
        if (osMessageQueueGet(inaQueueHandle, &msgReceived, NULL, 0) == osOK) {
            RTOS_LOG_DEBUG("[INA_TASK] Received message ID:%d from module:%d\r\n", msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_REQUEST_INA_MCU:
                    // Leer INA MCU solo cuando se solicita
                    inaMcu.readCurrent_mA();
                    RTOS_LOG_DEBUG("[INA_TASK] INA MCU read on request: %.3f mA\r\n", inaMcu.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_MCU, MODULE_INA, MODULE_FSM, (uint8_t*)&(inaMcu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[INA_TASK] Sent INA MCU data to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;
                
                case MSG_ID_REQUEST_INA_GPS:
                    // Leer INA GPS solo cuando se solicita
                    inaGps.readCurrent_mA();
                    RTOS_LOG_DEBUG("[INA_TASK] INA GPS read on request: %.3f mA\r\n", inaGps.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_GPS, MODULE_INA, MODULE_FSM, (uint8_t*)&(inaGps.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[INA_TASK] Sent INA GPS data to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_REQUEST_INA_IMU:
                    // Leer INA IMU solo cuando se solicita
                    inaImu.readCurrent_mA();
                    RTOS_LOG_DEBUG("[INA_TASK] INA IMU read on request: %.3f mA\r\n", inaImu.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_IMU, MODULE_INA, MODULE_FSM, (uint8_t*)&(inaImu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[INA_TASK] Sent INA IMU data to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_INA_GPS:
                    inaGps.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_GPS_DATA, 
                                                         MODULE_INA, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaGps.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[INA_TASK] Sent INA GPS data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_INA_IMU:
                    inaImu.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_IMU_DATA, 
                                                         MODULE_INA, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaImu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[INA_TASK] Sent INA IMU data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_INA_MCU:
                    inaMcu.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_MCU_DATA, 
                                                         MODULE_INA, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaMcu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[INA_TASK] Sent INA MCU data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
        
                default:
                    RTOS_LOG_WARN("[INA_TASK] Unknown message ID: %d\r\n", msgReceived->id);
                    break;
            }
      
            MessagePool_Free(msgReceived);
            msgReceived = NULL;
        }

        osDelay(5000);
    }
}
