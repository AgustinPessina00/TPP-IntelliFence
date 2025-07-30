
#include "fsmTask.h"

extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;

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

CowState classifyMotion(Acceleration acc) {
  float abs_ax = fabsf(acc.ax);
  float abs_ay = fabsf(acc.ay);
  float abs_az = fabsf(acc.az);

  if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az < 0.05f)
    return CowState::SLEEP;
  else if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az > 0.1f)
    return CowState::GRAZING;
  else
    return CowState::MOVEMENT;
}

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
    zone_t zone;
    Message* msg = new Message(MSG_ID_REQUEST_DISTANCE_TO_FENCE, ModuleId_t::FSM, ModuleId_t::DISTANCE, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
    osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

    if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
      switch (msgReceived->id) {
        case MSG_ID_SEND_DISTANCE_TO_FENCE: {
          if (msgReceived->length == sizeof(zone_t) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
            std::memcpy(&zone, msgReceived->payload, sizeof(zone_t));
            
            // printf("FSM recibió ZONA: zone=%d\r\n", zone);
              
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

    // === Si no está en GREEN_ZONE, aplicar estímulo ===
    if (zone != GREEN_ZONE) {
      // TODO: Switch Case de los distintos mensajes a mandar a stimulus.
    }
    else {
      // === Solicitar Mensaje de IMU ===
      Message* msg = new Message(MSG_ID_REQUEST_IMU, ModuleId_t::FSM, ModuleId_t::SENSOR_ACQ, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
      osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

      if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
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
        
        // Importante: liberar memoria del mensaje recibido
        delete msgReceived;
      }

      cow.updateState(classifyMotion(cow.getAcceleration())); // GRAZING, SLEEP, MOVEMENT

      switch (cow.getState()) {
        case CowState::SLEEP:
          Message* msg = new Message(MSG_ID_GPS_CONFIG, ModuleId_t::FSM, ModuleId_t::GPS, sizeof(GpsRate));  // [latitud, longitud] = 2 doubles
          std::memcpy(msg->payload, &(GpsRate::VERY_SLOW), sizeof(GpsRate));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          enterLowPowerSleep(); // El micro se duerme, IMU genera WAKE_UP
          break;

        case CowState::GRAZING:
          Message* msg = new Message(MSG_ID_GPS_CONFIG, ModuleId_t::FSM, ModuleId_t::GPS, sizeof(GpsRate));  // [latitud, longitud] = 2 doubles
          std::memcpy(msg->payload, &(GpsRate::SLOW), sizeof(GpsRate));
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          break;

        case CowState::MOVEMENT:
        // TODO: Obtener dist de distanceTask.
          if (dist < NEAR_LIMIT) {
            Message* msg = new Message(MSG_ID_GPS_CONFIG, ModuleId_t::FSM, ModuleId_t::GPS, sizeof(GpsRate));  // [latitud, longitud] = 2 doubles
            std::memcpy(msg->payload, &(GpsRate::MEDIUM), sizeof(GpsRate));
            osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          }
          else {
            Message* msg = new Message(MSG_ID_GPS_CONFIG, ModuleId_t::FSM, ModuleId_t::GPS, sizeof(GpsRate));  // [latitud, longitud] = 2 doubles
            std::memcpy(msg->payload, &(GpsRate::FAST), sizeof(GpsRate));
            osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
          
          }
        
          break;
      
        default:
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  } 
}
