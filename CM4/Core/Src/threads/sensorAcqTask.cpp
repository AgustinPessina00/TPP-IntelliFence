#include "FreeRTOS.h"
#include "ina226.h"
#include "lsm6dso.h"
#include "sam_m10q.h"
#include "task.h"
#include "threads/sensorAcqTask_new.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

static SamM10q gps(gpsAddress);  // Dirección I2C 7-bit del GPS SAM-M10Q
static Lsm6dso imu(imuAddress); // Dirección I2C 7-bit del LSM6DSO
static Ina226 inaGps(configs[gpsIndex].address, configs[gpsIndex].rShunt, configs[gpsIndex].currentLSB, Ina226Averaging::AVG_128, Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, Ina226Mode::SHUNT_BUS_CONTINUOUS);
static Ina226 inaImu(configs[imuIndex].address, configs[imuIndex].rShunt, configs[imuIndex].currentLSB, Ina226Averaging::AVG_128, Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, Ina226Mode::SHUNT_BUS_CONTINUOUS);               
static Ina226 inaMcu(configs[mcuIndex].address, configs[mcuIndex].rShunt, configs[mcuIndex].currentLSB, Ina226Averaging::AVG_128, Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, Ina226Mode::SHUNT_BUS_CONTINUOUS);

void acquireDataFromSensors(double* gpsData, double* imuData, float& inaGpsCurrent, float& inaImuCurrent, float& inaMcuCurrent);

void sensorAcqTask(void *argument) {
    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    RTOS_LOG_INFO("[SENSOR_ACQ] Task initialized successfully\n");
    double gpsData[2] = {0.0, 0.0};
    double imuData[3] = {0.0, 0.0, 0.0};
    float inaGpsCurrent = 0.0f;
    float inaImuCurrent = 0.0f;
    float inaMcuCurrent = 0.0f;

    while(1) {
        // Leer todos los sensores periódicamente
        acquireDataFromSensors(gpsData, imuData, inaGpsCurrent, inaImuCurrent, inaMcuCurrent);
        // Verificar si hay mensajes de solicitud
        if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, 1000) == osOK) {
            RTOS_LOG_DEBUG("[SENSOR_ACQ] Received message ID:%d from module:%d\n", msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_REQUEST_GPS:
                    msgToSend = MessagePool_Allocate();
                    EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_GPS, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)gpsData, sizeof(double) * 2);
                    osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                    RTOS_LOG_DEBUG("[SENSOR_ACQ]] Sent GPS data to FSM\n");
                    msgToSend = NULL;
                    break;

                case MSG_ID_REQUEST_IMU:
                    msgToSend = MessagePool_Allocate();
                    EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_IMU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)imuData, sizeof(double) * 3);
                    osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                    RTOS_LOG_DEBUG("[SENSOR_ACQ]] Sent IMU data to FSM\n");
                    msgToSend = NULL;
                    break;
        
                case MSG_ID_REQUEST_INA_MCU:
                    msgToSend = MessagePool_Allocate();
                    EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_MCU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaMcu.current), sizeof(float));
                    osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                    RTOS_LOG_DEBUG("[SENSOR_ACQ]] Sent INA MCU data to FSM\n");
                    msgToSend = NULL;
                    break;
                
                case MSG_ID_REQUEST_INA_GPS:
                    msgToSend = MessagePool_Allocate();
                    EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_GPS, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaGps.current), sizeof(float));
                    osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                    RTOS_LOG_DEBUG("[SENSOR_ACQ]] Sent INA GPS data to FSM\n");
                    msgToSend = NULL;
                    break;

                case MSG_ID_REQUEST_INA_IMU:
                    msgToSend = MessagePool_Allocate();
                    EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_IMU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaImu.current), sizeof(float));
                    osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                    RTOS_LOG_DEBUG("[SENSOR_ACQ]] Sent INA IMU data to FSM\n");
                    msgToSend = NULL;
                    break;
        
                default:
                    break;
            }
      
        MessagePool_Free(msgReceived);
        msgReceived = NULL;
        }
        osDelay(100);
    }
}

void acquireDataFromSensors(double* gpsData, double* imuData, float& inaGpsCurrent, float& inaImuCurrent, float& inaMcuCurrent) {
    gps.read_gps_position();
    
    gpsData[0] = gps.latitude;
    gpsData[1] = gps.longitude;
    RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS read: lat %.6f, lon %.6f\n", gps.latitude, gps.longitude);

    imu.readAcceleration();
    imuData[0] = imu.ax;
    imuData[1] = imu.ay;
    imuData[2] = imu.az;
    RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU read: ax %.2f, ay %.2f, az %.2f\n", imu.ax, imu.ay, imu.az);

    inaGps.readCurrent_mA();
    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA GPS current read: %.2f mA\n", inaGps.current);
    inaImu.readCurrent_mA();
    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA IMU current read: %.2f mA\n", inaImu.current);
    inaMcu.readCurrent_mA();
    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA MCU current read: %.2f mA\n", inaMcu.current);
}