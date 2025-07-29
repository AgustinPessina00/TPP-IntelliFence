
#include "fsmTask.h"

extern fsmQueueHandle;
extern dispatcherQueueHandle;

// TODO: VER COMO MANEJAMOS EL TEMA DE PASAR COMO PARÁMETROS COW Y FENCE PARA ESTA TASK. LO NECESITAN MÁS TASKS?

void enterLowPowerSleep(void) {
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

CowState classifyMotion(Acceleration imu) {
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

/*zone_t getZoneForDistance(distance_t dist, Fence fence) {
    for (zone_t i = GREEN_ZONE; i < BLACK_ZONE; i++)
    {
      if (dist < fence.getThresholds[i])
        return static_cast<zone_t>(i - 1);  // zona anterior
    }

    return BLACK_ZONE;
}*/


void fsmTask(void *argument) {
    Message* msgReceived;
    
    while (1) {
    
      msgReceived = nullptr;  // se reinicia el puntero antes de recibir algo

      // === Solicitar Mensaje de GPS ===
      Message* msg = new Message(MSG_ID_REQUEST_GPS, ModuleId_t::FSM, ModuleId_t::SENSOR_ACQ, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
      osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

      if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
        switch (msgReceived->id) {
          case MSG_ID_SEND_GPS: {
            Position pos;
            if (msgReceived->length == 2 * sizeof(double) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
              std::memcpy(&pos.latitude, msgReceived->payload, sizeof(double));
              std::memcpy(&pos.longitude, msgReceived->payload + sizeof(double), sizeof(double));

              // printf("FSM recibió GPS: lat=%.5f, lon=%.5f\r\n", latitude, longitude);

              cow.updatePosition(pos);

            } else {
              // Manejo de mensaje corrupto o malformado
              printf("FSM recibió GPS con payload inválido.\r\n");
            }
            
            break;
          }

          // Otros casos futuros...
        }

        // Importante: liberar memoria del mensaje recibido
        delete msgReceived;
      }

      // === Solicitar Mensaje de Distancia al Cerco ===
      Message* msg = new Message(MSG_ID_REQUEST_DISTANCE_TO_FENCE, ModuleId_t::FSM, ModuleId_t::DISTANCE, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
      osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

      if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
        switch (msgReceived->id) {
          case MSG_ID_SEND_DISTANCE_TO_FENCE: {
            if (msgReceived->length == 3 * sizeof(zone_t) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
              zone_t zone;
              std::memcpy(&zone, msgReceived->payload, sizeof(zone_t));
              
              // printf("FSM recibió ZONA: zone=%d\r\n", zone);

              switch () {
                case constant expression:
                  /* code */
                  break;
              
                default:
                  break;
              }
              
            } else {
              // Manejo de mensaje corrupto o malformado
              printf("FSM recibió ZONE con payload inválido.\r\n");
            }
            
            break;
          }

          // Otros casos futuros...
        }

        // Importante: liberar memoria del mensaje recibido
        delete msgReceived;
      }



      // === Solicitar Mensaje de IMU ===
      Message* msg = new Message(MSG_ID_REQUEST_IMU, ModuleId_t::FSM, ModuleId_t::SENSOR_ACQ, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
      osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

      if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
        switch (msgReceived->id) {
          case MSG_ID_SEND_IMU: {
            if (msgReceived->length == 3 * sizeof(float) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
              Acceleration acc;
              std::memcpy(&acc.ax, msgReceived->payload, sizeof(float));
              std::memcpy(&acc.ay, msgReceived->payload, sizeof(float));
              std::memcpy(&acc.az, msgReceived->payload, sizeof(float));
              
              // printf("FSM recibió IMU: ax=%.5f, ay=%.5f, az=%.5f\r\n", ax, ay, az);

              cow.updateAcceleration(acc);

            } else {
              // Manejo de mensaje corrupto o malformado
              printf("FSM recibió IMU con payload inválido.\r\n");
            }
            
            break;
          }

          // Otros casos futuros...
        }

        // Importante: liberar memoria del mensaje recibido
        delete msgReceived;
      }

      vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/*
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
