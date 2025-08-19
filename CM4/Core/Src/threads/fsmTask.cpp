
#include "threads/fsmTask.h"
#include "math.h"
#include "stdio.h"

extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;
extern osMessageQueueId_t stimulusQueueHandle;
extern osMessageQueueId_t distanceToLimitQueueHandle;
extern osMessageQueueId_t gpsQueueHandle;

// TODO: VER COMO MANEJAMOS EL TEMA DE PASAR COMO PARÁMETROS COW Y FENCE PARA ESTA TASK. LO NECESITAN MÁS TASKS?

/*
FSM::FSM() {
  this->mainFSM = MainFSM_t::STARTUP_ROUTINE;
  this->normalOpFSM = NormalOpFSM_t::INITIALIZE;
  
  this->startupRoutineState = STARTUP_ROUTINE_BEGIN;
  
  this->initializeState = INITIALIZE_BEGIN;
  this->greenZoneState = GREEN_ZONE_BEGIN;
  this->stimulusZoneState = STIMULUS_ZONE_BEGIN;

  this->fenceTransitionState = FENCE_TRANSITION_BEGIN;

  this->tries = 0; // TODO: Podemos definir varios "tries" para cada caso (i.e.: gps, lora, zone, etc)

  this->msgReceived = nullptr;  
}*/

void fsmTask(void *argument) {
  fsmTaskParams *fsmParams = static_cast<fsmTaskParams *> (argument);

  MainFSM_t mainFSM = MainFSM_t::STARTUP_ROUTINE;
  NormalOpFSM_t normalOpFSM = NormalOpFSM_t::INITIALIZE;

  StartupRoutineState_t startupRoutineState = STARTUP_ROUTINE_BEGIN;

  InitializeState_t initializeState = INITIALIZE_BEGIN;
  GreenZoneState_t greenZoneState = GREEN_ZONE_BEGIN;
  StimulusZone_t stimulusZoneState = STIMULUS_ZONE_BEGIN;

  FenceTransitionState_t fenceTransitionState = FENCE_TRANSITION_BEGIN;

  Message* msgReceived = nullptr;

  uint8_t startupTries = 0;

  while (1) {
    switch (mainFSM) {
      case MainFSM_t::STARTUP_ROUTINE:
        runStartupRoutineFSM(mainFSM, startupRoutineState, msgReceived, startupTries);
        break;

      case MainFSM_t::NORMAL_OPERATION:
        runNormalOperationFSM();
        if (shouldEnterFenceTransition()) {
          mainFSM = MainFSM_t::FENCE_TRANSITION; // TODO: VER BIEN DONDE PONER LA TRANSICIÓN ENTRE FSM. PARA MI ESTÁ BIEN ACÁ.
        }
        break;

      case MainFSM_t::FENCE_TRANSITION:
        runFenceTransitionFSM();
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void runStartupRoutineFSM(MainFSM_t mainFSM, StartupRoutineState_t startupRoutineState, Message* msgReceived, uint8_t startupTries) {

  switch (startupRoutineState) {
    case STARTUP_ROUTINE_BEGIN:
      startupTries = 0;
      startupRoutineState = STARTUP_ROUTINE_REQUEST_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_POSITION:
      sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request GPS Measurement.
      startupRoutineState = STARTUP_ROUTINE_WAIT_POSITION;
      break;
    
    case STARTUP_ROUTINE_WAIT_POSITION:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if(updatePosition(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_GPS dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          startupRoutineState = STARTUP_ROUTINE_SEND_POSITION_LORA;
          startupTries = 0;
        }
      } else {
    	  startupTries++;
        if (tries >= MAX_TRIES)
          startupRoutineState = STARTUP_ROUTINE_REQUEST_POSITION;
          //handleGPSFailure(); 
      }

      break;

    case STARTUP_ROUTINE_SEND_POSITION_LORA:
      sendPosition(MSG_ID_LORA_POSITION, ModuleId_t::LORA_TX);
      startupRoutineState = STARTUP_ROUTINE_WAIT_FENCE;
      break;

    case STARTUP_ROUTINE_WAIT_FENCE:
      // TODO: CREAR TIMEOUT PARA ESPERAR FENCE
      if (recievedFence(msgReceived) == HAL_OK) {
        startupRoutineState = STARTUP_ROUTINE_SAVE_FENCE;
      }
      break;

    case STARTUP_ROUTINE_SAVE_FENCE:
      updateFence();
      startupRoutineState = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_NEW_POSITION:
      sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request NEW GPS Measurement.
      startupRoutineState = STARTUP_ROUTINE_WAIT_NEW_POSITION;
      break;
    
    case STARTUP_ROUTINE_WAIT_NEW_POSITION:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if(updatePosition(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_GPS dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          startupRoutineState = STARTUP_ROUTINE_REQUEST_ZONE;
          startupTries = 0;
        }
      } else {
    	  startupTries++;
        if (tries >= MAX_TRIES)
          startupRoutineState = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
          //handleGPSFailure(); 
      }

      break;

    case STARTUP_ROUTINE_REQUEST_ZONE:
      sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, ModuleId_t::DISTANCE);
      startupRoutineState = STARTUP_ROUTINE_EVALUATE_ZONE;
      break;

    case STARTUP_ROUTINE_EVALUATE_ZONE:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if (updateDistAndZone(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_ZONE_TO_FENCE dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          startupRoutineState = STARTUP_ROUTINE_END;
        }
      } else {
    	  startupTries++;
        if (tries >= MAX_TRIES)
          startupRoutineState = STARTUP_ROUTINE_REQUEST_ZONE;
          //handleGPSFailure(); 
      }
      break;

    case STARTUP_ROUTINE_END:
      startupRoutineState = STARTUP_ROUTINE_BEGIN;
      if (isInFence() == HAL_OK)
        mainFSM = MainFSM_t::NORMAL_OPERATION;
      else
        mainFSM = MainFSM_t::FENCE_TRANSITION;
      break;
  }
}

void runNormalOperationFSM() {

  switch (normalOpFSM) {
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

void runInitializeFSM() {

  switch (initializeState) {
    case INITIALIZE_BEGIN:
      tries = 0;
      initializeState = INITIALIZE_REQUEST_POSITION;
      break;

    case INITIALIZE_REQUEST_POSITION:
      sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request GPS Measurement.
      initializeState = INITIALIZE_WAIT_POSITION;
      break;
    
    case INITIALIZE_WAIT_POSITION:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if(updatePosition(msgReceived) == HAL_OK) { // Si desencolo el Message y no eraMSG_ID_SEND_GPS dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          initializeState = INITIALIZE_REQUEST_ZONE;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          initializeState = INITIALIZE_REQUEST_POSITION;
          //handleGPSFailure(); 
      }

      break;

    case INITIALIZE_REQUEST_ZONE:
      sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, ModuleId_t::DISTANCE);
      initializeState = INITIALIZE_EVALUATE_ZONE;
      break;
    
    case INITIALIZE_EVALUATE_ZONE:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if (updateDistAndZone(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_ZONE_TO_FENCE dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          initializeState = INITIALIZE_END;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          initializeState = INITIALIZE_REQUEST_ZONE;
          //handleGPSFailure(); 
      }
      break;

    case INITIALIZE_END:
      initializeState = INITIALIZE_BEGIN;
      if (isInFence())
        normalOpFSM = NormalOpFSM_t::GREEN_ZONE;
      else
        normalOpFSM = NormalOpFSM_t::STIMULUS_ZONE;
      break;
  }
}

void runGreenZoneFSM() {

  switch (greenZoneState) {
    case GREEN_ZONE_BEGIN:
      sendZoneToStimulus(fsmParams->cow->getCurrentZone(), ModuleId_t::STIMULUS); // envío la zona a STIMULUS para que apague el estímulo
      tries = 0;
      greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;

      break;

    case GREEN_ZONE_REQUEST_ACCELERATION:
      sendMessage(MSG_ID_REQUEST_IMU, ModuleId_t::SENSOR_ACQ); // Request IMU Measurement.
      greenZoneState = GREEN_ZONE_WAIT_ACCELERATION;
      break;
    
    case GREEN_ZONE_WAIT_ACCELERATION:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if(updateAcceleration(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_IMU dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
          //handleGPSFailure(); 
      }

      break;

    case GREEN_ZONE_EVALUATE_COWSTATE:
      updateState();
      CowState state = cow.getState();
      if (state == CowState::GRAZING)
        greenZoneState = GREEN_ZONE_GRAZING;
      else if (state == CowState::SLEEP) 
        greenZoneState = GREEN_ZONE_SLEEP;
      else if (state == CowState::MOVEMENT)
        greenZoneState = GREEN_ZONE_MOVEMENT;
      break;

    case GREEN_ZONE_GRAZING:
      updateGpsAdqTime(GpsRate::SLOW) == HAL_OK;
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_SLEEP:
      updateGpsAdqTime(GpsRate::STOP) == HAL_OK;
      enterLowPowerSleep();
      // TODO: Cuando llega la interrupción de la IMU se despierta y sigue acá??
      // O tengo que poner un estado intermedio como WAKE_UP para verificar o algo así?
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_MOVEMENT:
      // updateDistance(); Ya se hace cuando se recibe la zona.
      if (cow.getDistanceToLimit() <= NEAR_LIMIT)
        greenZoneState = GREEN_ZONE_NEAR_LIMIT;
      else  
        greenZoneState = GREEN_ZONE_FAR_LIMIT;
      break;

    case GREEN_ZONE_NEAR_LIMIT:
      updateGpsAdqTime(GpsRate::FAST) == HAL_OK;
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_FAR_LIMIT:
      updateGpsAdqTime(GpsRate::MEDIUM);
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_WAIT_GPS_ADQ_TIME:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if (gpsResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_GPS_CONFIG_RESPONSE dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          greenZoneState = GREEN_ZONE_END;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
          //handleGPSFailure(); 
      }
      break;
      

    case GREEN_ZONE_END:
      greenZoneState = GREEN_ZONE_BEGIN;
      normalOpFSM = NormalOpFSM_t::INITIALIZE;  // Reinicia NORMAL_OPERATION FSM
    break;
  }
}

void runStimulusZoneFSM() {
  switch (stimulusZoneState) {
    case STIMULUS_ZONE_BEGIN:
      stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
      break;

    case STIMULUS_ZONE_SEND_ZONE:
      sendStimulus(cow.getCurrentZone(), ModuleId_t::STIMULUS);
      stimulusZoneState = STIMULUS_ZONE_WAIT_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_RESPONSE:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if (recievedStimulusResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_STIMULUS_FEEDBACK dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          stimulusZoneState = STIMULUS_ZONE_END;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
          //handleGPSFailure(); 
      }
      break;

    case STIMULUS_ZONE_END:
      stimulusZoneState = STIMULUS_ZONE_BEGIN;
      normalOpFSM = NormalOpFSM_t::INITIALIZE;  // Reinicia NORMAL_OPERATION FSM
    break;
  }
}

void runFenceTransitionFSM(){
  switch (fenceTransitionState) {
    case FENCE_TRANSITION_BEGIN:
      tries = 0;
      fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
      break;
  
    case FENCE_TRANSITION_GPSRATE_FAST:
      updateGpsAdqTime(GpsRate::FAST);
      fenceTransitionState = FENCE_TRANSITION_WAIT_GPS_ADQ_TIME;
      break;
    
    case FENCE_TRANSITION_WAIT_GPS_ADQ_TIME:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if (gpsResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_GPS_CONFIG_RESPONSE dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
          //handleGPSFailure(); 
      }
      break;
    
    case FENCE_TRANSITION_REQUEST_POSITION:
      sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request GPS Measurement.
      fenceTransitionState = FENCE_TRANSITION_WAIT_POSITION;
      break;
    
    case FENCE_TRANSITION_WAIT_POSITION:
      if (dequeuedMessage(msgReceived) == HAL_OK) {
        if(updatePosition(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_GPS dejo todo tal cual está y vuelvo a desencolar en la próxima pasada.
          fenceTransitionState = FENCE_TRANSITION_UPDATE_PARTIAL_FENCE;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
          //handleGPSFailure(); 
      }

      break;
      
    case FENCE_TRANSITION_UPDATE_PARTIAL_FENCE:
      updateFence();
      fenceTransitionState = FENCE_TRANSITION_REQUEST_NEW_POSITION;
      break;

    case FENCE_TRANSITION_REQUEST_NEW_POSITION:
    	sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request GPS Measurement.
    	fenceTransitionState = FENCE_TRANSITION_WAIT_NEW_POSITION;
    	break;

    case FENCE_TRANSITION_WAIT_NEW_POSITION:
      if (recievedPosition() == HAL_OK) {
        if (!fencesEqual() && !inPartialFence()) {
          updatePosition(msgReceived);
          fenceTransitionState = FENCE_TRANSITION_REQUEST_ZONE;
        }
        else if (fencesEqual() && !inPartialFence()){
          updatePosition(msgReceived);
          fenceTransitionState = FENCE_TRANSITION_UPDATE_PARTIAL_FENCE;
        }
        else if (fenceEqual() && inPartialFence()) {
          updatePosition(msgReceived);
          fenceTransitionState = FENCE_TRANSITION_END;
        }
      }
      break;

    case FENCE_TRANSITION_REQUEST_ZONE:
      if (requestZone() == ACK){
        fenceTransitionState = FENCE_TRANSITION_WAIT_ZONE;
        tries = 0;
      } else
        tries++;
        if (tries >= MAX_TRIES)
          //handleGPSFailure(); // opcional
      break;

    case FENCE_TRANSITION_WAIT_ZONE:  
      if (receivedZone() == HAL_OK)
        updateZone();
        fenceTransitionState = FENCE_TRANSITION_EVALUATE_ZONE;
      break;
    
    case FENCE_TRANSITION_EVALUATE_ZONE:
      evaluateZone();
      fenceTransitionState = FECNE_TRANSITION_SEND_ZONE;
      break;

    case FECNE_TRANSITION_SEND_ZONE:
      if(sendStimulus(cow.getCurrentZone()) == HAL_OK)
        fenceTransitionState = FENCE_TRANSITION_WAIT_RESPONSE;
      break;
    
    case FENCE_TRANSITION_WAIT_RESPONSE:
      if (recievedStimulusResponse(msgReceived) == HAL_OK)
        fenceTransitionState = FENCE_TRANSITION_REQUEST_NEW_POSITION;
      break;

    case FENCE_TRANSITION_END:
      fenceTransitionState = FENCE_TRANSITION_BEGIN;
      this->mainFSM = MainFSM_t::NORMAL_OPERATION;
      break;
      
  }
}


void sendMessage(uint8_t msgId, ModuleId_t dest) {
  Message* msg = new Message(msgId, ModuleId_t::FSM, dest, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
  osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
}

HAL_StatusTypeDef dequeuedMessage(Message *msgReceived) {
  if (osMessageQueueGet(dispatcherQueueHandle, &(msgReceived), NULL, 0) == osOK) {
    return HAL_OK;
  }
  return HAL_ERROR;
}

HAL_StatusTypeDef updatePosition(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;
  switch (msgReceived->id) {
    case MSG_ID_SEND_GPS:
      Position pos;
      if (msgReceived->length == 2 * sizeof(double) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
        memcpy(&pos.latitude, msgReceived->payload, sizeof(double));
        memcpy(&pos.longitude, msgReceived->payload + sizeof(double), sizeof(double));

        // printf("FSM recibió GPS: lat=%.5f, lon=%.5f\r\n", latitude, longitude);

        fsmParams->cow->updatePosition(pos);

        status = HAL_OK;

      } else {
        // Manejo de mensaje corrupto o malformado
        printf("FSM recibió GPS con payload inválido.\r\n");
      }

      break;

    // Otros casos futuros...
    // CASE 2...
  }

  // Importante: liberar memoria del mensaje recibido
  delete msgReceived;
  return status;
}

void sendPosition(uint8_t msgId, ModuleId_t dest) {
  Message* msg = new Message(msgId, ModuleId_t::FSM, dest, 2 * sizeof(double));  // [latitud, longitud] = 2 doubles
  memcpy(msg->payload, &(fsmParams->cow->latitude), sizeof(double));
  memcpy(msg->payload + sizeof(double), &(fsmParams->cow->longitude), sizeof(double));
  osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
}

// Recibe latitud y longitud (double) de LORA_RX con un tamaño fijo.
HAL_StatusTypeDef recievedFence(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;

  switch (msgReceived->id) {
    case MSG_ID_LORA_VERTEXES: 
      if (msgReceived->payload != nullptr && msgReceived->length % sizeof(Vertex) == 0) {
        int vertexCount = msgReceived->length / sizeof(Vertex);
        for (uint32_t i = 0; i < vertexCount; i++) {
          Vertex v;
          memcpy(&v, msgReceived->payload + i * sizeof(Vertex), sizeof(Vertex));
          fence.addVertex(v);
        }

        status = HAL_OK;
        // printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      
      } else {
        printf("FSM recibió payload inválido para vértices.\r\n");
      }

      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;
}

void updateFence() {
  fsmParams->fence->updateLimits();
}

HAL_StatusTypeDef isInFence() {
  if (fsmParams->cow->getcurrentZone() != zone_t::BLACK_ZONE) {
    return HAL_OK;
  }

  return HAL_ERROR;
}

HAL_StatusTypeDef updateDistAndZone(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;
  zone_t zone;
  float dist;

  switch (msgReceived->id) {
    case MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE: 
      if (msgReceived->payload != nullptr && msgReceived->length == (sizeof(zone_t) + sizeof(float))) {
        memcpy(&zone, msgReceived->payload, sizeof(zone_t));
        memcpy(&dist, msgReceived->payload + sizeof(zone_t), sizeof(float));

        fsmParams->cow->updateZone(zone);
        fsmParams->cow->updateDistanceToLimit(dist);
        status = HAL_OK;
        // printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      
      } else {
        printf("FSM recibió ZONE AND DISTANCE con payload inválido.\r\n");
      }

      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;  
}

HAL_StatusTypeDef updateAcceleration(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;
  switch (msgReceived->id) {
    case MSG_ID_SEND_IMU:
      Acceleration acc;
      if (msgReceived->length == 3 * sizeof(float) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
        memcpy(&acc.ax, msgReceived->payload, sizeof(float));
        memcpy(&acc.ay, msgReceived->payload + sizeof(float), sizeof(float));
        memcpy(&acc.az, msgReceived->payload + 2 * sizeof(float), sizeof(float));

        // printf("FSM recibió GPS: lat=%.5f, lon=%.5f\r\n", latitude, longitude);

        fsmParams->cow->updateAcceleration(acc);

        status = HAL_OK;

      } else {
        // Manejo de mensaje corrupto o malformado
        printf("FSM recibió IMU con payload inválido.\r\n");
      }

      break;

    // Otros casos futuros...
    // CASE 2...
  }

  // Importante: liberar memoria del mensaje recibido
  delete msgReceived;
  return status;
}

void updateState() {
	fsmParams->cow_>updateState(classifyMotion(fsmParams->cow->getAcceleration()));
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

void updateGpsAdqTime(GpsRate gpsRate) {
  Message* msg = new Message(MSG_ID_GPS_REQUEST_CONFIG, ModuleId_t::FSM, ModuleId_t::GPS, sizeof(GpsRate));
  memcpy(msg->payload, &gpsRate, sizeof(GpsRate));
  osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
}

void enterLowPowerSleep() {
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
}

HAL_StatusTypeDef gpsResponse(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;

  switch (msgReceived->id) {
    case MSG_ID_GPS_CONFIG_RESPONSE: 
      status = HAL_OK;
      // printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;
}

void sendZoneToStimulus(zone_t zone, ModuleId_t dest) {
  Message* msg = new Message(MSG_ID_ZONE_CHANGE, ModuleId_t::FSM, dest, sizeof(zone_t));
  memcpy(msg->payload, &zone, sizeof(zone_t));
  osMessageQueuePut(dispatcherQueueHandle, msg, 0, 0);
}

HAL_StatusTypeDef FSM::recievedStimulusResponse(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;

  switch (msgReceived->id) {
    case MSG_ID_STIMULUS_FEEDBACK: 
      status = HAL_OK;
      // printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;
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
