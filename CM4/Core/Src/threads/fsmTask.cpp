
#include "fsmTask.h"

extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;

// TODO: VER COMO MANEJAMOS EL TEMA DE PASAR COMO PARÁMETROS COW Y FENCE PARA ESTA TASK. LO NECESITAN MÁS TASKS?


FSM::FSM() {
  this->mainFSM = FSM_t::STARTUP_ROUTINE;
  this->normalOpFSM = NormalOperationSubFSM_t::INITIALIZE;
  
  this->startupRoutineState = STARTUP_ROUTINE_BEGIN;
  
  this->initializeState = INITIALIZE_BEGIN;
  this->greenZoneState = GREEN_ZONE_BEGIN;
  this->stimulusZoneState = STIMULUS_ZONE_BEGIN;

  this->fenceTransitionState = FENCE_TRANSITION_BEGIN;
}

void FSM::runStartupRoutineFSM() {
  static uint8_t gpsTries = 0;
  static const uint8_t MAX_TRIES = 10;

  switch (this->startupRoutineState) {
    case STARTUP_ROUTINE_BEGIN:
      gpsTries = 0;
      this->startupRoutineState = STARTUP_ROUTINE_REQUEST_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_POSITION:
      if (readGPS() == HAL_OK) {
        this->startupRoutineState = STARTUP_ROUTINE_WAIT_POSITION;
        gpsTries = 0;
      } else {
        gpsTries++;
        if (gpsTries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case STARTUP_ROUTINE_WAIT_POSITION:
      updatePosition();
      this->startupRoutineState = STARTUP_ROUTINE_SEND_POSITION_LORA;
      break;

    case STARTUP_ROUTINE_SEND_POSITION_LORA:
      if (sendLoRaPosition() == ACK){
        this->startupRoutineState = STARTUP_ROUTINE_WAIT_FENCE;
        gpsTries = 0;
      } else
        gpsTries++;
      break;

    case STARTUP_ROUTINE_WAIT_FENCE:
      if (receivedFence()) {
        this->startupRoutineState = STARTUP_ROUTINE_SAVE_FENCE;
      }
      break;

    case STARTUP_ROUTINE_SAVE_FENCE:
      saveToMemory();
      this->startupRoutineState = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_NEW_POSITION:
      if (readGPS() == HAL_OK) {
        this->startupRoutineState = STARTUP_ROUTINE_WAIT_NEW_POSITION;
        gpsTries = 0;
      } else {
        gpsTries++;
        if (gpsTries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case STARTUP_ROUTINE_WAIT_NEW_POSITION:
      updatePosition();
      this->startupRoutineState = STARTUP_ROUTINE_END;
      break;

    case STARTUP_ROUTINE_END:
      this->startupRoutineState = STARTUP_ROUTINE_BEGIN;
      if (isInFence())
        this->mainFSM = MainFSM_t::NORMAL_OPERATION;
      else
        this->mainFSM = MainFSM_t::FENCE_TRANSITION;
      break;
  }
}

void FSM::runNormalOperationFSM() {

  switch (this->normalOpFSM) {
    case NormalOpFSM_t::INITIALIZE:
      runInitializeFSM();
      break;

    case NormalOpFSM_t::GREEN_ZONE:
      runGreenZoneFSM();
      break;

    case NormalOpFSM_t::STIMULUS_ZONE:
      runStimulusZoneFSM();
      break;
  }
}

void FSM::runInitializeFSM() {
  static uint8_t gpsTries = 0;
  static const uint8_t MAX_TRIES = 10;

  switch (this->initializeState) {
    case INITIALIZE_BEGIN:
      gpsTries = 0;
      this->initializeState = INITIALIZE_REQUEST_POSITION;
      break;

    case INITIALIZE_REQUEST_POSITION:
      if (readGPS() == HAL_OK) {
        this->initializeState = INITIALIZE_WAIT_POSITION;
        gpsTries = 0;
      } else {
        gpsTries++;
        if (gpsTries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case INITIALIZE_WAIT_POSITION:
      updatePosition();
      this->initializeState = INITIALIZE_REQUEST_ZONE;
      break;

    case INITIALIZE_REQUEST_ZONE:
      requestZone();
      break;

    case INITIALIZE_WAIT_ZONE:
      if (receivedZone() == HAL_OK)
        this->initializeState = INITIALIZE_EVALUATE_ZONE;
      break;
    
    case INITIALIZE_EVALUATE_ZONE:
      evaluateZone();
      this->initializeState = INITIALIZE_END;
      break;

    case INTIALIZE_END:
      this->initializeState = INITIALIZE_BEGIN;
      if (isInFence())
        this->normalOpFSM = NormalOpFSM_t::GREEN_ZONE;
      else
        this->normalOpFSM = NormalOpFSM_t::STIMULUS_ZONE;
      break;
  }
}

void FSM::runGreenZoneFSM() {

  switch (this->greenZoneState) {
    case GREEN_ZONE_BEGIN:
      gpsTries = 0;
      this->greenZoneState = STARTUP_ROUTINE_REQUEST_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_POSITION:
      if (readGPS() == HAL_OK) {
        this->greenZoneState = STARTUP_ROUTINE_WAIT_POSITION;
        gpsTries = 0;
      } else {
        gpsTries++;
        if (gpsTries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case STARTUP_ROUTINE_WAIT_POSITION:
      gpsTries = 0;
      this->greenZoneState = STARTUP_ROUTINE_SEND_POSITION_LORA;
      break;

    case STARTUP_ROUTINE_SEND_POSITION_LORA:
      if (sendLoRaPosition() == ACK){
        this->greenZoneState = STARTUP_ROUTINE_WAIT_FENCE;
        gpsTries = 0;
      } else
        gpsTries++;
      break;

    case STARTUP_ROUTINE_WAIT_FENCE:
      if (receivedFence()) {
        this->greenZoneState = STARTUP_ROUTINE_SAVE_FENCE;
      }
      break;

    case STARTUP_ROUTINE_SAVE_FENCE:
      saveToMemory();
      this->greenZoneState = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_NEW_POSITION:
      if (readGPS() == HAL_OK) {
        this->greenZoneState = STARTUP_ROUTINE_WAIT_NEW_POSITION;
        gpsTries = 0;
      } else {
        gpsTries++;
        if (gpsTries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case STARTUP_ROUTINE_WAIT_NEW_POSITION:
      gpsTries = 0;
      this->greenZoneState = STARTUP_ROUTINE_END;
      break;

    case STARTUP_ROUTINE_END:
      this->greenZoneState = GREEN_ZONE_BEGIN;
      this->normalOpFSM = NormalOpFSM_t::INITIALIZE;  // Reinicia NORMAL_OPERATION FSM
    break;
  }
}



void FSM::fsmTask(void *argument) {

  while (1) {
    switch (this->mainFSM) {
      case MainFSM_t::STARTUP_ROUTINE:
        runStartupRoutineFSM();
        break;

      case MainFSM_t::NORMAL_OPERATION:
        runNormalOperationFSM();
        if (shouldEnterFenceTransition()) {
          this->mainFSM = FSMType::FENCE_TRANSITION; // TODO: VER BIEN DONDE PONER LA TRANSICIÓN ENTRE FSM. PARA MI ESTÁ BIEN ACÁ.
        }
        break;

      case MainFSM_t::FENCE_TRANSITION:
        runFenceTransition();
        if (fenceTransitionFinished()) {
          this->mainFSM = FSMType::NORMAL_OPERATION;
        }
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  } 
}







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















/*


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
        default:
          break;
      }

      // Importante: liberar memoria del mensaje recibido
      delete msgReceived;
    }

    // === Solicitar Mensaje de Distancia al Cerco ===
    zone_t zone;
    Message* msg = new Message(MSG_ID_REQUEST_ZONE_TO_FENCE, ModuleId_t::FSM, ModuleId_t::DISTANCE, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
    osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

    if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
      switch (msgReceived->id) {
        case MSG_ID_SEND_ZONE_TO_FENCE: {
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
          
        default:
          break;
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
          
          default:
            break;
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
          // === Solicitar DISTANCIA ===
          Message* msg = new Message(MSG_ID_SEND_DISTANCE_TO_FENCE, ModuleId_t::FSM, ModuleId_t::DISTANCE, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
          osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);

          if (osMessageQueueGet(fsmQueueHandle, &msgReceived, NULL, 0) == osOK) {
            switch (msgReceived->id) {
              case MSG_ID_SEND_DISTANCE_TO_FENCE: {
                if (msgReceived->length == sizeof(float) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
                  distance_t dist;
                  std::memcpy(&dist, msgReceived->payload, sizeof(float));

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
                } else {
                  // Manejo de mensaje corrupto o malformado
                  printf("FSM recibió DISTANCE con payload inválido.\r\n");
                }
            
                break;
              } 

              // Otros casos futuros...
              default:
                break;
            }
        
            // Importante: liberar memoria del mensaje recibido
            delete msgReceived;
          }

          break;
      
        default:
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  } */