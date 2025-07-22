
#include "sensorAcqTask.h"
extern sensorAcqQueueHandle;
extern dispatcherQueueHandle;
void enterLowPowerSleep(void)
{
  // Asegurarse de limpiar interrupciones previas
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  // Deshabilitar SysTick para que no interrumpa durante el stop
  HAL_SuspendTick();

  // Habilitar wake-up por EXTI (ya debe estar configurado en NVIC / GPIO)
  // No es necesario hacerlo explícitamente si CubeMX lo generó bien

  // Entrar a modo STOP2
  //HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

  // Entrar en modo SLEEP. El micro se detiene hasta que ocurra una interrupción.
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

  // (El micro ahora está detenido. Al despertar vuelve desde aquí)

  // Reanudar SysTick para que FreeRTOS vuelva a funcionar
  HAL_ResumeTick();

  // Reconfigurar el reloj si hace falta
  SystemClock_Config();  // Necesario si usás HSE/HSEBYP/HSE+PLL
}

CowState classifyMotion(Acceleration imu)
{
  float abs_ax = fabsf(imu.ax);
  float abs_ay = fabsf(imu.ay);
  float abs_az = fabsf(imu.az);

  if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az < 0.05f)
    return CowState::SLEEP;
  else if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az > 0.1f)
    return CowState::GRAZING;
  else
    return CowState::MOVEMENT;
}

zone_t getZoneForDistance(distance_t dist, Fence fence)
{
    for (zone_t i = GREEN_ZONE; i < BLACK_ZONE; i++)
    {
      if (dist < fence.getThresholds[i])
        return static_cast<zone_t>(i - 1);  // zona anterior
    }

    return BLACK_ZONE;
}

void startSensorAcqTask(void *argument) {
  sensorAcqTaskParams *sensorParams = static_cast<sensorAcqTaskParams *>(argument);

  float currentMcu;
  float currentGps;
  float currentImu;
  
  for (;;) {
    // === Leer GPS ===
    if (sensorParams->gps->read_nmea_stream() != HAL_OK) {
      error_count++;
      continue;  // volver a intentar luego
    }
    else {
      sensorParams->gps->update_location_and_time();
    }

    // === Leer IMU ===
    if (sensorParams->imu->readAcceleration(&acc) =! HAL_OK) {
      error_count++;
      continue;  // volver a intentar luego
    }

    // === Leer INA_MCU ===
    if (sensorParams->inaMcu->readCurrent_mA(&currentMcu) =! HAL_OK) {
      error_count++;
      continue;  // volver a intentar luego
    }
    
    // === Leer INA_GPS ===
    if (sensorParams->inaGps->readCurrent_mA(&currentGps) =! HAL_OK) {
      error_count++;
      continue;  // volver a intentar luego
    }
    
    // === Leer INA_IMU ===
    if (sensorParams->inaImu->readCurrent_mA(&currentImu) =! HAL_OK) {
      error_count++;
      continue;  // volver a intentar luego
    }

    // TODO: Armar los mensaje_t y cambiarlo en "&zone"
    // RECIBE FLAGS DE LA COLA QUE LE MANDA fsmTask A sensorAcqTask
    if (osMessageQueueGet(sensorAcqQueueHandle, &msg, NULL, 0) == osOK) {
      switch (msg.id) {
        case SEND_GPS_DATA:
          Message* msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(float));  //[latitud, longitud]
          std::memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(float));
          std::memcpy(msg->payload + sizeof(float), &(sensorParams->gps->longitude), sizeof(float));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        case SEND_IMU_DATA:
          // TODO: COMPLETAR CON EL MENSAJE DE LA IMU
          // Message* msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(float));  //[latitud, longitud]
          // std::memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(float));
          // std::memcpy(msg->payload + sizeof(float), &(sensorParams->gps->longitude), sizeof(float));
          // osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        case SEND_INA_MCU_DATA:
          // TODO: COMPLETAR CON EL MENSAJE DEL INA
          // Message* msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(float));  //[latitud, longitud]
          // std::memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(float));
          // std::memcpy(msg->payload + sizeof(float), &(sensorParams->gps->longitude), sizeof(float));
          // osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        case SEND_INA_GPS_DATA:
          // TODO: COMPLETAR CON EL MENSAJE DEL INA
          // Message* msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(float));  //[latitud, longitud]
          // std::memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(float));
          // std::memcpy(msg->payload + sizeof(float), &(sensorParams->gps->longitude), sizeof(float));
          // osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        case SEND_INA_IMU_DATA:
          // TODO: COMPLETAR CON EL MENSAJE DEL INA
          // Message* msg = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM, 2 * sizeof(float));  //[latitud, longitud]
          // std::memcpy(msg->payload, &(sensorParams->gps->latitude), sizeof(float));
          // std::memcpy(msg->payload + sizeof(float), &(sensorParams->gps->longitude), sizeof(float));
          // osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;
        default:
        // TODO: printf si queremos debuggear.
          break;
      }
    }
    
  }
}


/*
    // TODO: pasar esto a fsmTask
      if (sensorParams->imu->readAcceleration(&acc) == HAL_OK)
      {
        cow.updateAcceleration(imu);
        
        state = classifyMotion(imu);  // GRAZING, SLEEP, MOVEMENT

        // Mostrar IMU Read por UART
        printf("IMU: ax=%.2f, ay=%.2f, az=%.2f -> State: %d\r\n", imu.ax, imu.ay, imu.az, state);

        cow.updateState(state);

        switch (state)
        {
          case CowState::SLEEP:
            GPS_SetAcquisitionRate(GpsRate::VERY_SLOW); // 1 muestra/hora
            enterLowPowerSleep(); // El micro se duerme, IMU genera WAKE_UP
            break;

          case CowState::GRAZING:
            GPS_SetAcquisitionRate(GpsRate::SLOW); // p.ej. 1 muestra/30 min
            break;

          case CowState::MOVEMENT:
            GPS_SetAcquisitionRate((dist < NEAR_LIMIT) ? GpsRate::MEDIUM : GpsRate::FAST);
            break;
        }
      }

    cow.updatePosition(gps);

    // Mostrar GPS Read por UART
    printf("GPS read: lat=%.5f, lon=%.5f\r\n", gps.latitude, gps.longitude);

    // === 2. Determinar distancia a cerca virtual y zona ===
    dist = calculateDistanceToLimit(cow.getPosition(), fence.getLimits()); // TODO: Implementar esta función.
    zone = getZoneFromDistance(dist, fence); // GREEN, BLUE, YELLOW, RED, BLACK

    // Mostrar Zona por UART
    printf("Zone: %d, Distance: %.2f m\r\n", zone, dist);

    // === 3. Si no está en GREEN_ZONE, mandar estímulo ===
    if (zone != GREEN_ZONE)
    {
      // TODO: definir StimulusQueueHandle - ioc?
      xQueueSend(StimulusQueueHandle, &zone, 0); // Despierta StimulusTask
    }
    else
    {
      // === 4. Leer IMU ===
      if (imu.readAcceleration(&acc) == HAL_OK)
      {
        cow.updateAcceleration(imu);
        
        state = classifyMotion(imu);  // GRAZING, SLEEP, MOVEMENT

        // Mostrar IMU Read por UART
        printf("IMU: ax=%.2f, ay=%.2f, az=%.2f -> State: %d\r\n", imu.ax, imu.ay, imu.az, state);

        cow.updateState(state);

        switch (state)
        {
          case CowState::SLEEP:
            GPS_SetAcquisitionRate(GpsRate::VERY_SLOW); // 1 muestra/hora
            enterLowPowerSleep(); // El micro se duerme, IMU genera WAKE_UP
            break;

          case CowState::GRAZING:
            GPS_SetAcquisitionRate(GpsRate::SLOW); // p.ej. 1 muestra/30 min
            break;

          case CowState::MOVEMENT:
            GPS_SetAcquisitionRate((dist < NEAR_LIMIT) ? GpsRate::MEDIUM : GpsRate::FAST);
            break;
        }
      }
    }

    // TODO: REVISAR, queremos que esto varíe?? 
    osDelay(GPS_SAMPLE_RATE);  // p.ej. 1 vez por minuto o lo que hayas configurado
*/

