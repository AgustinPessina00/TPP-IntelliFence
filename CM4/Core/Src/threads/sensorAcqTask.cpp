#include "FreeRTOS.h"
#include "ina226.h"
#include "lsm6dso.h"
#include "sam_m10q.h"
#include "task.h"
#include "sensorAcqTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void sensorAcqTask(void *argument) {
    // CRITICAL: Use static objects to avoid stack overflow
    // These objects must have static lifetime and be initialized explicitly
    static SamM10q gps;
    static Lsm6dso imu;
    static Ina226 inaGps;
    static Ina226 inaImu;
    static Ina226 inaMcu;
    
    // Explicit initialization - must be called once at task startup
    if (!gps.init(gpsAddress)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize GPS\n");
        // Retry or handle error appropriately
    }
    
    if (!imu.init(imuAddress)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize IMU\n");
        // Retry or handle error appropriately
    }
    
    if (!inaGps.init(configs[gpsIndex].address, configs[gpsIndex].rShunt, 
                     configs[gpsIndex].currentLSB, Ina226Averaging::AVG_128, 
                     Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, 
                     Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize INA226 GPS\n");
    }
    
    if (!inaImu.init(configs[imuIndex].address, configs[imuIndex].rShunt, 
                     configs[imuIndex].currentLSB, Ina226Averaging::AVG_128, 
                     Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, 
                     Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize INA226 IMU\n");
    }
    
    if (!inaMcu.init(configs[mcuIndex].address, configs[mcuIndex].rShunt, 
                     configs[mcuIndex].currentLSB, Ina226Averaging::AVG_128, 
                     Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, 
                     Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize INA226 MCU\n");
    }

    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    gpsRateSpeed rateGPS;
    RTOS_LOG_INFO("[SENSOR_ACQ] Task initialized successfully\n");
    float gpsData[2] = {0.0, 0.0};
    float imuData[3] = {0.0, 0.0, 0.0};

    static uint32_t stackMonitorCounter = 0;
    while(1) {
        // Monitorear stack cada ~10 segundos
        if (++stackMonitorCounter >= 10) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            RTOS_LOG_INFO("[SENSOR_ACQ] Stack libre: %u words (%u bytes)\n", 
                         stackLeft, stackLeft * 4);
            stackMonitorCounter = 0;
        }
        
        // // Leer todos los sensores periódicamente
        //     gps.read_gps_position();
        //     gpsData[0] = gps.latitude;
        //     gpsData[1] = gps.longitude;
        //     RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS read: lat %.6f, lon %.6f\n", gps.latitude, gps.longitude);

        //     imu.readAcceleration();
        //     imuData[0] = imu.ax;
        //     imuData[1] = imu.ay;
        //     imuData[2] = imu.az;
        //     RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU read: (%.6f g, %.6f g, %.6f g)\n", imu.ax/1000, imu.ay/1000, imu.az/1000);

        //     inaGps.readCurrent_mA();
        //     RTOS_LOG_DEBUG("[SENSOR_ACQ] INA GPS current read: %.3f mA\n", inaGps.current);
        //     inaImu.readCurrent_mA();
        //     RTOS_LOG_DEBUG("[SENSOR_ACQ] INA IMU current read: %.3f mA\n", inaImu.current);
        //     inaMcu.readCurrent_mA();
        //     RTOS_LOG_DEBUG("[SENSOR_ACQ] INA MCU current read: %.3f mA\n", inaMcu.current);
        // // Verificar si hay mensajes de solicitud
        if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, 0) == osOK) {
            RTOS_LOG_DEBUG("[SENSOR_ACQ] Received message ID:%d from module:%d\n", msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_REQUEST_GPS:
                    // Leer GPS solo cuando se solicita
                    gps.read_gps_position();
                    gpsData[0] = gps.latitude;
                    gpsData[1] = gps.longitude;
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS read on request: lat %.6f, lon %.6f\n", gps.latitude, gps.longitude);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_GPS, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)gpsData, sizeof(float) * 2);
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent GPS data to FSM\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_REQUEST_IMU:
                    // Leer IMU solo cuando se solicita
                    imu.readAcceleration();
                    imuData[0] = imu.ax;
                    imuData[1] = imu.ay;
                    imuData[2] = imu.az;
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU read on request: (%.3f, %.3f, %.3f) mg\n", imu.ax, imu.ay, imu.az);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_IMU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)imuData, sizeof(float) * 3);
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent IMU data to FSM\n");
                        msgToSend = NULL;
                    }
                    break;
        
                case MSG_ID_REQUEST_INA_MCU:
                    // Leer INA MCU solo cuando se solicita
                    inaMcu.readCurrent_mA();
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA MCU read on request: %.3f mA\n", inaMcu.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_MCU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaMcu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA MCU data to FSM\n");
                        msgToSend = NULL;
                    }
                    break;
                
                case MSG_ID_REQUEST_INA_GPS:
                    // Leer INA GPS solo cuando se solicita
                    inaGps.readCurrent_mA();
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA GPS read on request: %.3f mA\n", inaGps.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_GPS, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaGps.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA GPS data to FSM\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_REQUEST_INA_IMU:
                    // Leer INA IMU solo cuando se solicita
                    inaImu.readCurrent_mA();
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA IMU read on request: %.3f mA\n", inaImu.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_IMU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaImu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA IMU data to FSM\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_GPS_REQUEST_CONFIG:
                    rateGPS = static_cast<gpsRateSpeed>(msgReceived->payload[0]);
                    if (gps.set_new_acq_time(rateGPS)) {
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS acquisition time set to %d\n", static_cast<int>(rateGPS));
                    }
                    else {
                        RTOS_LOG_WARN("[SENSOR_ACQ] Failed to set GPS acquisition time\n");
                    }
                    break;

                // Console UART requests - leer sensores on-demand
                case MSG_ID_CONSOLE_READ_GPS:
                    gps.read_gps_position();
                    gpsData[0] = gps.latitude;
                    gpsData[1] = gps.longitude;
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_GPS_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)gpsData, 2 * sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent GPS data to console\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_IMU:
                    imu.readAcceleration();
                    imuData[0] = imu.ax;
                    imuData[1] = imu.ay;
                    imuData[2] = imu.az;
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_IMU_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)imuData, 3 * sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent IMU data to console\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_INA_GPS:
                    inaGps.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_GPS_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaGps.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA GPS data to console\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_INA_IMU:
                    inaImu.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_IMU_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaImu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA IMU data to console\n");
                        msgToSend = NULL;
                    }
                    break;

                case MSG_ID_CONSOLE_READ_INA_MCU:
                    inaMcu.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_MCU_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaMcu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA MCU data to console\n");
                        msgToSend = NULL;
                    }
                    break;
        
                default:
                    RTOS_LOG_WARN("[SENSOR_ACQ] Unknown message ID: %d\n", msgReceived->id);
                    break;
            }
      
            MessagePool_Free(msgReceived);
            msgReceived = NULL;
        }

        osDelay(1000);
    }
}