
#include "sensorAcqTask.h"

extern sensorAcqQueueHandle;
extern dispatcherQueueHandle;

void sensorAcqTask(void *argument) {
  sensorAcqTaskParams *sensorParams = static_cast<sensorAcqTaskParams *>(argument);

  Message* msgReceived;
  
  while (1){

    msgReceived = nullptr;  // se reinicia el puntero antes de recibir algo

    // === Leer GPS ===
    if (sensorParams->gps->read_nmea_stream() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      continue;  // volver a intentar luego
    }
    else {
      sensorParams->gps->update_location_and_time();
    }

    // === Leer IMU ===
    if (sensorParams->imu->readAcceleration() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      continue;  // volver a intentar luego
    }

    // === Leer INA_MCU ===
    if (sensorParams->inaMcu->readCurrent_mA() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      continue;  // volver a intentar luego
    }
    
    // === Leer INA_GPS ===
    if (sensorParams->inaGps->readCurrent_mA() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      continue;  // volver a intentar luego
    }
    
    // === Leer INA_IMU ===
    if (sensorParams->inaImu->readCurrent_mA() != HAL_OK) {
      error_count++;
      // TODO: Manejo de errores.
      continue;  // volver a intentar luego
    }

    // RECIBE FLAGS DE LA COLA QUE LE MANDA fsmTask A sensorAcqTask

    if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, 0) == osOK) {
      switch (msgReceived->id) {
        case MSG_ID_REQUEST_GPS:
          Message* msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(double));  // [latitud, longitud] = 2 doubles
          std::memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(double));
          std::memcpy(msg->payload + sizeof(double), &(sensorParams->gps->longitude), sizeof(double));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;

        case MSG_ID_REQUEST_IMU:
          Message* msg = new Message(MSG_ID_SEND_IMU, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 3 * sizeof(float));  // [accel_x, accel_y, accel_z] = 3 floats
          std::memcpy(msg->payload, &(sensorParams->imu->ax), sizeof(float));
          std::memcpy(msg->payload + sizeof(float), &(sensorParams->imu->ay), sizeof(float));
          std::memcpy(msg->payload + 2 * sizeof(float), &(sensorParams->imu->az), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        
        case MSG_ID_REQUEST_INA_MCU:
          Message* msg = new Message(MSG_ID_SEND_INA_MCU, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, sizeof(float));  // [current] = float
          std::memcpy(msg->payload, &(sensorParams->inaMcu->current), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        
        case MSG_ID_REQUEST_INA_GPS:
          Message* msg = new Message(MSG_ID_SEND_INA_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, sizeof(float));  // [current] = float
          std::memcpy(msg->payload, &(sensorParams->inaGps->current), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;

        case MSG_ID_REQUEST_INA_IMU:
          Message* msg = new Message(MSG_ID_SEND_INA_IMU, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, sizeof(float));  // [current] = float
          std::memcpy(msg->payload, &(sensorParams->inaImu->current), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        
        default:
        // TODO: printf si queremos debuggear.
          break;
      }
    }
    
    delete msgReceived;
    
    // Delay de adquisión de muestras.
    osDelay(pdMS_TO_TICKS(SAMPLE_RATE)); 
  }
}
