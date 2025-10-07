
#include "threads/sensorAcqTask.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

static int x = 0;

void sensorAcqTask(void *argument) {
  sensorAcqTaskParams *sensorParams = static_cast<sensorAcqTaskParams *>(argument);

  Message* msgReceived;

  int error_count = 0;

  while (1){
	//printf("[SENSORACQ] sensorAcqTask running...\n");
    msgReceived = nullptr;  // se reinicia el puntero antes de recibir algo

    x++;
    if(x == 100){
    	int i;
    	x=0;
    	i = 1;
    }
    // === Leer GPS ===
    if (sensorParams->gps->read_gps_position() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      //continue;  // volver a intentar luego
    }

    // === Leer IMU ===
    if (sensorParams->imu->readAcceleration() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      //continue;  // volver a intentar luego
    }

    // === Leer INA_MCU ===
    if (sensorParams->inaMcu->readCurrent_mA() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      //continue;  // volver a intentar luego
    }

    // === Leer INA_GPS ===
    if (sensorParams->inaGps->readCurrent_mA() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      //continue;  // volver a intentar luego
    }

    // === Leer INA_IMU ===
    if (sensorParams->inaImu->readCurrent_mA() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      //continue;  // volver a intentar luego
    }

    // RECIBE FLAGS DE LA COLA QUE LE MANDA fsmTask A sensorAcqTask
    Message *msg = nullptr;
    if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, 0) == osOK) {
      switch (msgReceived->id) {
        case MSG_ID_REQUEST_GPS:
		  //printf("[SENSORACQ] Recibi mensaje con ID %d desde %d\n", msgReceived->id, msgReceived->sender);
          Position gpsPos;
          gpsPos.latitude = sensorParams->gps->latitude;
          gpsPos.longitude = sensorParams->gps->longitude;
		  //msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(double));  // [latitud, longitud] = 2 doubles
          //memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(double));
          //memcpy(msg->payload + sizeof(double), &(sensorParams->gps->longitude), sizeof(double));
          msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM,(uint8_t*)&gpsPos,sizeof(Position));
          osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
          //printf("[SENSORACQ] Ubicacion mandada al FSM\n");
          break;

        case MSG_ID_REQUEST_IMU:
          msg = new Message(MSG_ID_SEND_IMU, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 3 * sizeof(float));  // [accel_x, accel_y, accel_z] = 3 floats
          memcpy(msg->payload, &(sensorParams->imu->ax), sizeof(float));
          memcpy(msg->payload + sizeof(float), &(sensorParams->imu->ay), sizeof(float));
          memcpy(msg->payload + 2 * sizeof(float), &(sensorParams->imu->az), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
          break;
        
        case MSG_ID_REQUEST_INA_MCU:
          msg = new Message(MSG_ID_SEND_INA_MCU, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, sizeof(float));  // [current] = float
          memcpy(msg->payload, &(sensorParams->inaMcu->current), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
          break;
        
        case MSG_ID_REQUEST_INA_GPS:
          msg = new Message(MSG_ID_SEND_INA_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, sizeof(float));  // [current] = float
          memcpy(msg->payload, &(sensorParams->inaGps->current), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;

        case MSG_ID_REQUEST_INA_IMU:
          msg = new Message(MSG_ID_SEND_INA_IMU, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, sizeof(float));  // [current] = float
          memcpy(msg->payload, &(sensorParams->inaImu->current), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        
        default:
        // TODO: printf si queremos debuggear.
          break;
      }
      
      delete msgReceived;
    }
    
    // Delay de adquisión de muestras.
    //osDelay(pdMS_TO_TICKS(SAMPLE_RATE));
    //vTaskDelay(pdMS_TO_TICKS(10));
    osDelay(100);
  }
}
