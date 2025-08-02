
#include "fsmTask.h"

extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;
extern osMessageQueueId_t distanceToLimitQueueHandle;

// TODO: VER COMO MANEJAMOS EL TEMA DE PASAR COMO PARÁMETROS COW Y FENCE PARA ESTA TASK. LO NECESITAN MÁS TASKS?


FSM::FSM() {
  this->mainFSM = MainFSM_t::STARTUP_ROUTINE;
  this->normalOpFSM = NormalOpFSM_t::INITIALIZE;
  
  this->startupRoutineState = STARTUP_ROUTINE_BEGIN;
  
  this->initializeState = INITIALIZE_BEGIN;
  this->greenZoneState = GREEN_ZONE_BEGIN;
  this->stimulusZoneState = STIMULUS_ZONE_BEGIN;

  this->fenceTransitionState = FENCE_TRANSITION_BEGIN;

  this->tries = 0; // TODO: Podemos definir varios "tries" para cada caso (i.e.: gps, lora, zone, etc)
}

void FSM::runStartupRoutineFSM() {

  switch (this->startupRoutineState) {
    case STARTUP_ROUTINE_BEGIN:
      tries = 0;
      this->startupRoutineState = STARTUP_ROUTINE_REQUEST_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_POSITION:
      if (requestPosition() == HAL_OK) {
        this->startupRoutineState = STARTUP_ROUTINE_WAIT_POSITION;
        tries = 0;
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case STARTUP_ROUTINE_WAIT_POSITION:
      if (recievedPosition() == HAL_OK) {
        updatePosition();
        this->startupRoutineState = STARTUP_ROUTINE_SEND_POSITION_LORA;
      }
      break;

    case STARTUP_ROUTINE_SEND_POSITION_LORA:
      if (sendLoRaPosition() == ACK){
        this->startupRoutineState = STARTUP_ROUTINE_WAIT_FENCE;
        tries = 0;
      } else
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
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
      if (requestPosition() == HAL_OK) {
        this->startupRoutineState = STARTUP_ROUTINE_WAIT_NEW_POSITION;
        tries = 0;
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case STARTUP_ROUTINE_WAIT_NEW_POSITION:
      if (recievedPosition() == HAL_OK){
        updatePosition();
        this->startupRoutineState = STARTUP_ROUTINE_END;
      }
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

  switch (this->initializeState) {
    case INITIALIZE_BEGIN:
      tries = 0;
      this->initializeState = INITIALIZE_REQUEST_POSITION;
      break;

    case INITIALIZE_REQUEST_POSITION:
      if (readGPS() == HAL_OK) {
        this->initializeState = INITIALIZE_WAIT_POSITION;
        tries = 0;
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case INITIALIZE_WAIT_POSITION:
      tries = 0;
      updatePosition();
      this->initializeState = INITIALIZE_REQUEST_ZONE;
      break;

    case INITIALIZE_REQUEST_ZONE:
      if (requestZone() == ACK){
        this->startupRoutineState = INITIALIZE_WAIT_ZONE;
        tries = 0;
      } else
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      break;

    case INITIALIZE_WAIT_ZONE:
      if (receivedZone() == HAL_OK)
        updateZone();
        updateDistance();
        this->initializeState = INITIALIZE_EVALUATE_ZONE;
      break;
    
    case INITIALIZE_EVALUATE_ZONE:
      evaluateZone();
      this->initializeState = INITIALIZE_END;
      break;

    case INITIALIZE_END:
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
      tries = 0;
      this->greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
      break;

    case GREEN_ZONE_REQUEST_ACCELERATION:
      if (requestAcceleration() == HAL_OK) {
        this->greenZoneState = GREEN_ZONE_WAIT_ACCELERATION;
        tries = 0;
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      }
      break;
    
    case GREEN_ZONE_WAIT_ACCELERATION:
      if (recievedAcceleration() == HAL_OK) {
        updateAcceleration();
        this->greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
      }
        break;

    case GREEN_ZONE_EVALUATE_COWSTATE:
      updateState();
      if (cow.getState() == CowState::GRAZING)
        this->greenZoneState = GREEN_ZONE_GRAZING; 
      else if (cow.getState() == CowState::SLEEP) 
        this->greenZoneState = GREEN_ZONE_SLEEP;
      else if (cow.getState() == CowState::MOVEMENT)
        this->greenZoneState = GREEN_ZONE_MOVEMENT;
      break;

    case GREEN_ZONE_GRAZING:
      if (updateGpsAdqTime(GpsRate::SLOW) == HAL_OK)
        this->greenZoneState = GREEN_ZONE_END;
      break;

    case GREEN_ZONE_SLEEP:
      if (updateGpsAdqTime(GpsRate::STOP) == HAL_OK)
        enterLowPowerSleep();
        // TODO: Cuando llega la interrupción de la IMU se despierta y sigue acá??
        // O tengo que poner un estado intermedio como WAKE_UP para verificar o algo así?
        this->greenZoneState = GREEN_ZONE_END;
      break;

    case GREEN_ZONE_MOVEMENT:
      // updateDistance(); Ya se hace cuando se recibe la zona.
      if (cow.getDistanceToLimit() <= NEAR_LIMIT)
        this->greenZoneState = GREEN_ZONE_NEAR_LIMIT; 
      else  
        this->greenZoneState = GREEN_ZONE_FAR_LIMIT;
      break;

    case GREEN_ZONE_NEAR_LIMIT:
      if (updateGpsAdqTime(GpsRate::FAST) == HAL_OK)
        this->greenZoneState = GREEN_ZONE_END;
      break;

    case GREEN_ZONE_FAR_LIMIT:
      if (updateGpsAdqTime(GpsRate::MEDIUM) == HAL_OK)
        this->greenZoneState = GREEN_ZONE_END;
      break;

    case GREEN_ZONE_END:
      this->greenZoneState = GREEN_ZONE_BEGIN;
      this->normalOpFSM = NormalOpFSM_t::INITIALIZE;  // Reinicia NORMAL_OPERATION FSM
    break;
  }
}

void FSM::runStimulusZoneFSM() {
  switch (this->stimulusZoneState) {
    case STIMULUS_ZONE_BEGIN:
      if (cow.getCurrentZone() == zone_t::LIGHT_BLUE_ZONE)
        this->stimulusZoneState = STIMULUS_ZONE_LIGHT_BLUE; 
      else if (cow.getCurrentZone() == zone_t::BLUE_ZONE)
        this->stimulusZoneState = STIMULUS_ZONE_BLUE;
      else if (cow.getCurrentZone() == zone_t::DARK_BLUE_ZONE)
        this->stimulusZoneState = STIMULUS_ZONE_DARK_BLUE;
      else if (cow.getCurrentZone() == zone_t::YELLOW_ZONE)
        this->stimulusZoneState = STIMULUS_ZONE_YELLOW;
      else if (cow.getCurrentZone() == zone_t::RED_ZONE)
        this->stimulusZoneState = STIMULUS_ZONE_RED;
      break;

    case STIMULUS_ZONE_LIGHT_BLUE:
      if (sendStimulus() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_WAIT_LIGHT_BLUE_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_LIGHT_BLUE_RESPONSE:
      if (recievedStimulusResponse() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_END;
      break;

    case STIMULUS_ZONE_BLUE:
      if (sendStimulus() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_WAIT_BLUE_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_BLUE_RESPONSE:
      if (recievedStimulusResponse() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_END;
      break;

    case STIMULUS_ZONE_DARK_BLUE:
      if (sendStimulus() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_WAIT_DARK_BLUE_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_DARK_BLUE_RESPONSE:
      if (recievedStimulusResponse() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_END;
      break;

    case STIMULUS_ZONE_YELLOW:
      if (sendStimulus() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_WAIT_YELLOW_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_YELLOW_RESPONSE:
      if (recievedStimulusResponse() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_END;
      break;

    case STIMULUS_ZONE_RED:
      if (sendStimulus() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_WAIT_RED_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_RED_RESPONSE:
      if (recievedStimulusResponse() == HAL_OK)
        this->stimulusZoneState = STIMULUS_ZONE_END;
      break;

    case STIMULUS_ZONE_END:
      this->stimulusZoneState = STIMULUS_ZONE_BEGIN;
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
          this->mainFSM = MainFSM_t::FENCE_TRANSITION; // TODO: VER BIEN DONDE PONER LA TRANSICIÓN ENTRE FSM. PARA MI ESTÁ BIEN ACÁ.
        }
        break;

      case MainFSM_t::FENCE_TRANSITION:
        runFenceTransition();
        if (fenceTransitionFinished()) {
          this->mainFSM = MainFSM_t::NORMAL_OPERATION;
        }
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  } 
}

void FSM::enterLowPowerSleep() {
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

CowState FSM::classifyMotion(Acceleration acc) {
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