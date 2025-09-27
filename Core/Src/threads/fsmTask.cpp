
#include "threads/fsmTask.h"
#include "math.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

bool receivedMsgLoraRX;

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

  receivedMsgLoraRX = false;

  uint8_t tries = 0;

  // Esperamos unos segundos para que el sistema arranque
  //vTaskDelay(pdMS_TO_TICKS(2000));

  // Creamos una posición simulada FUERA DEL CERCO
  Position posicionFalsa;
  posicionFalsa.latitude = -99.999;   // Algo MUY lejos
  posicionFalsa.longitude = -99.999;

  // Creamos un mensaje falso
  Message *mensajeFake = new Message(MSG_ID_SEND_GPS, ModuleId_t::SENSOR_ACQ, ModuleId_t::FSM,(uint8_t*)&posicionFalsa,sizeof(Position));

  // Mandamos el mensaje como si viniera del sensor
  osMessageQueuePut(dispatcherQueueHandle, &mensajeFake, 0, 0);

  //printf("[SIMULACION] Mandé una posición falsa fuera del cerco\n");

  while (1) {
	//printf("[FSM] fsmTask running...\n");
    switch (mainFSM) {
      case MainFSM_t::STARTUP_ROUTINE:
        runStartupRoutineFSM(mainFSM, &startupRoutineState, msgReceived, tries, fsmParams);
        break;

      case MainFSM_t::NORMAL_OPERATION:
        runNormalOperationFSM(normalOpFSM, initializeState, greenZoneState, stimulusZoneState, msgReceived, tries, fsmParams);
        if (receivedMsgLoraRX) {
          mainFSM = MainFSM_t::FENCE_TRANSITION;
          receivedMsgLoraRX = false;
        }
        break;

      case MainFSM_t::FENCE_TRANSITION:
        runFenceTransitionFSM(mainFSM, fenceTransitionState, msgReceived, tries, fsmParams);
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void runStartupRoutineFSM(MainFSM_t mainFSM, StartupRoutineState_t* startupRoutineState, Message* msgReceived, uint8_t tries, fsmTaskParams *fsmParams) {

  switch (*startupRoutineState) {
    case STARTUP_ROUTINE_BEGIN:
      tries = 0;
      *startupRoutineState = STARTUP_ROUTINE_REQUEST_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_POSITION:
      sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request GPS Measurement.
      //printf("[FSM] Pido posicion GPS\n");
      *startupRoutineState = STARTUP_ROUTINE_WAIT_POSITION;
      break;
    
    case STARTUP_ROUTINE_WAIT_POSITION:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if(updatePosition(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_GPS dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          *startupRoutineState = STARTUP_ROUTINE_SEND_POSITION_LORA;
          tries = 0;
        }
      } else {
    	  tries++;
        if (tries >= MAX_TRIES)
          *startupRoutineState = STARTUP_ROUTINE_REQUEST_POSITION;
          //handleGPSFailure(); 
      }

      break;

    case STARTUP_ROUTINE_SEND_POSITION_LORA:
      sendPosition(MSG_ID_LORA_SEND_POSITION, ModuleId_t::LORA_TX, fsmParams);
      *startupRoutineState = STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE;
      break;

    case STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
		if (loraTxResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_LORA_SEND_POSITION_FEEDBACK dejo tal cual está y vuelvo a desencolar en la próxima pasada.
			*startupRoutineState = STARTUP_ROUTINE_WAIT_FENCE;
    	    tries = 0;
		}
	  } else {
		tries++;
		if (tries >= MAX_TRIES){
			*startupRoutineState = STARTUP_ROUTINE_SEND_POSITION_LORA;
		}
	  }
	  break;

    case STARTUP_ROUTINE_WAIT_FENCE:
      // TODO: CREAR TIMEOUT PARA ESPERAR FENCE
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
    	  if(receivedMsgLoraRX){
    		  *startupRoutineState = STARTUP_ROUTINE_SAVE_FENCE;
    		  receivedMsgLoraRX = false;
    	  }
	  }
      break;

    case STARTUP_ROUTINE_SAVE_FENCE:
      updateFence(fsmParams);
      *startupRoutineState = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
      break;

    case STARTUP_ROUTINE_REQUEST_NEW_POSITION:
      sendMessage(MSG_ID_REQUEST_GPS, ModuleId_t::SENSOR_ACQ); // Request NEW GPS Measurement.
      *startupRoutineState = STARTUP_ROUTINE_WAIT_NEW_POSITION;
      break;
    
    case STARTUP_ROUTINE_WAIT_NEW_POSITION:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if(updatePosition(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_GPS dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          *startupRoutineState = STARTUP_ROUTINE_REQUEST_ZONE;
          tries = 0;
        }
      } else {
    	  tries++;
        if (tries >= MAX_TRIES)
          *startupRoutineState = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
          //handleGPSFailure(); 
      }
      break;

    case STARTUP_ROUTINE_REQUEST_ZONE:
      sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, ModuleId_t::DISTANCE);
      *startupRoutineState = STARTUP_ROUTINE_EVALUATE_ZONE;
      break;

    case STARTUP_ROUTINE_EVALUATE_ZONE:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if (updateDistAndZone(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_ZONE_TO_FENCE dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          *startupRoutineState = STARTUP_ROUTINE_END;
          tries = 0;
        }
      } else {
    	  tries++;
        if (tries >= MAX_TRIES)
          *startupRoutineState = STARTUP_ROUTINE_REQUEST_ZONE;
          //handleGPSFailure(); 
      }
      break;

    case STARTUP_ROUTINE_END:
      *startupRoutineState = STARTUP_ROUTINE_BEGIN;
      if (isInFence(fsmParams) == HAL_OK)
        mainFSM = MainFSM_t::NORMAL_OPERATION;
      else
        mainFSM = MainFSM_t::FENCE_TRANSITION;
      break;
  }
}

void runNormalOperationFSM(NormalOpFSM_t normalOpFSM, InitializeState_t initializeState, GreenZoneState_t greenZoneState, StimulusZone_t stimulusZoneState, Message* msgReceived, uint8_t tries,fsmTaskParams *fsmParams) {

  switch (normalOpFSM) {
    case NormalOpFSM_t::INITIALIZE:
      runInitializeFSM(normalOpFSM, initializeState, msgReceived, tries, fsmParams);
      break;

    case NormalOpFSM_t::GREEN_ZONE:
      runGreenZoneFSM(normalOpFSM, greenZoneState, msgReceived, tries, fsmParams);
      break;

    case NormalOpFSM_t::STIMULUS_ZONE:
      runStimulusZoneFSM(normalOpFSM, stimulusZoneState, msgReceived, tries, fsmParams);
      break;
  }
}

void runInitializeFSM(NormalOpFSM_t normalOpFSM, InitializeState_t initializeState, Message* msgReceived, uint8_t tries, fsmTaskParams *fsmParams) {

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
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if(updatePosition(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no eraMSG_ID_SEND_GPS dejo tal cual está y vuelvo a desencolar en la próxima pasada.
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
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if (updateDistAndZone(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_ZONE_TO_FENCE dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          initializeState = INITIALIZE_END;
          tries = 0;
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
      if (isInFence(fsmParams)){
        normalOpFSM = NormalOpFSM_t::GREEN_ZONE;
      	sendZoneToStimulus(fsmParams->cow->getCurrentZone(), ModuleId_t::STIMULUS);
      } else
        normalOpFSM = NormalOpFSM_t::STIMULUS_ZONE;
      break;
  }
}

void runGreenZoneFSM(NormalOpFSM_t normalOpFSM, GreenZoneState_t greenZoneState, Message* msgReceived, uint8_t tries, fsmTaskParams *fsmParams) {
  CowState state;
  switch (greenZoneState) {
    case GREEN_ZONE_BEGIN:
      sendZoneToStimulus(fsmParams->cow->getCurrentZone(), ModuleId_t::STIMULUS); // envío la zona a STIMULUS para que apague el estímulo. DEBERÍA SER GREEN_ZONE
      tries = 0;
      greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;

      break;

    case GREEN_ZONE_REQUEST_ACCELERATION:
      sendMessage(MSG_ID_REQUEST_IMU, ModuleId_t::SENSOR_ACQ); // Request IMU Measurement.
      greenZoneState = GREEN_ZONE_WAIT_ACCELERATION;
      break;
    
    case GREEN_ZONE_WAIT_ACCELERATION:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if(updateAcceleration(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_IMU dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES) {
          greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
          //handleGPSFailure(); 
        }
      }
      break;

    case GREEN_ZONE_EVALUATE_COWSTATE:
      updateState(fsmParams);
      state = fsmParams->cow->getState();
      if (state == CowState::GRAZING) {
        greenZoneState = GREEN_ZONE_GRAZING;
      }
      else if (state == CowState::SLEEP) {
        greenZoneState = GREEN_ZONE_SLEEP;
      }
      else if (state == CowState::MOVEMENT) {
        greenZoneState = GREEN_ZONE_MOVEMENT;
      }
      break;

    case GREEN_ZONE_GRAZING:
      updateGpsAdqTime(GpsRate::SLOW);
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_SLEEP:
      updateGpsAdqTime(GpsRate::STOP);
      enterLowPowerSleep();
      // TODO: Cuando llega la interrupción de la IMU se despierta y sigue acá??
      // O tengo que poner un estado intermedio como WAKE_UP para verificar o algo así?
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_MOVEMENT:
      // updateDistance(); Ya se hace cuando se recibe la zona.
      if (fsmParams->cow->getDistanceToLimit() <= NEAR_LIMIT) {
        greenZoneState = GREEN_ZONE_NEAR_LIMIT;
      }
      else {
        greenZoneState = GREEN_ZONE_FAR_LIMIT;
      }
      break;

    case GREEN_ZONE_NEAR_LIMIT:
      updateGpsAdqTime(GpsRate::FAST);
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_FAR_LIMIT:
      updateGpsAdqTime(GpsRate::MEDIUM);
      greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
      break;

    case GREEN_ZONE_WAIT_GPS_ADQ_TIME:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if (gpsResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_GPS_CONFIG_RESPONSE dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          greenZoneState = GREEN_ZONE_END;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES){
          greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
          //handleGPSFailure(); 
        }
      }
      break;
      
    case GREEN_ZONE_END:
      greenZoneState = GREEN_ZONE_BEGIN;
      normalOpFSM = NormalOpFSM_t::INITIALIZE;  // Reinicia NORMAL_OPERATION FSM
    break;
  }
}

void runStimulusZoneFSM(NormalOpFSM_t normalOpFSM, StimulusZone_t stimulusZoneState, Message* msgReceived, uint8_t tries, fsmTaskParams *fsmParams) {
  switch (stimulusZoneState) {
    case STIMULUS_ZONE_BEGIN:
      stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
      break;

    case STIMULUS_ZONE_SEND_ZONE:
      sendZoneToStimulus(fsmParams->cow->getCurrentZone(), ModuleId_t::STIMULUS);
      stimulusZoneState = STIMULUS_ZONE_WAIT_RESPONSE;
      break;
    
    case STIMULUS_ZONE_WAIT_RESPONSE:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if (receivedStimulusResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_STIMULUS_FEEDBACK dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          stimulusZoneState = STIMULUS_ZONE_END;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES){
          stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
          //handleGPSFailure(); 
        }
      }
      break;

    case STIMULUS_ZONE_END:
      stimulusZoneState = STIMULUS_ZONE_BEGIN;
      normalOpFSM = NormalOpFSM_t::INITIALIZE;  // Reinicia NORMAL_OPERATION FSM
    break;
  }
}

void runFenceTransitionFSM(MainFSM_t mainFSM, FenceTransitionState_t fenceTransitionState, Message* msgReceived, uint8_t tries, fsmTaskParams *fsmParams){
  switch (fenceTransitionState) {
    case FENCE_TRANSITION_BEGIN:
      tries = 0;
      fenceTransitionState = FENCE_TRANSITION_DISABLE_STIMULUS;
      break;

    case FENCE_TRANSITION_DISABLE_STIMULUS:
      sendZoneToStimulus(BLACK_ZONE, ModuleId_t::STIMULUS);
      fenceTransitionState = FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE;
      break;

    case FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
		if (receivedStimulusResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_STIMULUS_FEEDBACK dejo tal cual está y vuelvo a desencolar en la próxima pasada.
			fenceTransitionState = FENCE_TRANSITION_UPDATE_FENCE;
		  tries = 0;
		}
	  }
      else {
		tries++;
		if (tries >= MAX_TRIES){
			fenceTransitionState = FENCE_TRANSITION_DISABLE_STIMULUS;
		  //handleGPSFailure();
		}
      }
	  break;

    case FENCE_TRANSITION_UPDATE_FENCE:
      updateFence(fsmParams);
      fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
      break;

    case FENCE_TRANSITION_GPSRATE_FAST:
      updateGpsAdqTime(GpsRate::FAST);
      fenceTransitionState = FENCE_TRANSITION_WAIT_GPS_ADQ_TIME;
      break;
    
    case FENCE_TRANSITION_WAIT_GPS_ADQ_TIME:
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if (gpsResponse(msgReceived) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_GPS_CONFIG_RESPONSE dejo tal cual está y vuelvo a desencolar en la próxima pasada.
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
      if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
        if(updatePosition(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_GPS dejo tal cual está y vuelvo a desencolar en la próxima pasada.
          fenceTransitionState = FENCE_TRANSITION_REQUEST_ZONE;
          tries = 0;
        }
      } else {
        tries++;
        if (tries >= MAX_TRIES)
          fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
          //handleGPSFailure(); 
      }
      break;
      
    case FENCE_TRANSITION_REQUEST_ZONE:
      sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, ModuleId_t::DISTANCE);
      fenceTransitionState = FENCE_TRANSITION_EVALUATE_ZONE;
      break;

    case FENCE_TRANSITION_EVALUATE_ZONE:
	  if (dequeuedMessage(msgReceived, fsmParams) == HAL_OK) {
		if (updateDistAndZone(msgReceived, fsmParams) == HAL_OK) { // Si desencolo el Message y no era MSG_ID_SEND_ZONE_TO_FENCE dejo tal cual está y vuelvo a desencolar en la próxima pasada.
			fenceTransitionState = FENCE_TRANSITION_END;
		  tries = 0;
		}
	  }
	  else {
		tries++;
		if (tries >= MAX_TRIES)
			fenceTransitionState = FENCE_TRANSITION_REQUEST_ZONE;
		  //handleGPSFailure();
	  }
      break;

    case FENCE_TRANSITION_END:
      fenceTransitionState = FENCE_TRANSITION_BEGIN;
      mainFSM = MainFSM_t::NORMAL_OPERATION;
      break;
  }
}


void sendMessage(uint8_t msgId, ModuleId_t dest) {
  Message* msg = new Message(msgId, ModuleId_t::FSM, dest, 0);  // Payload vacío, lo único que me importa es el ID (FLAG).
  osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
}

HAL_StatusTypeDef dequeuedMessage(Message *msgReceived, fsmTaskParams *fsmParams) {
  if (osMessageQueueGet(dispatcherQueueHandle, &msgReceived, NULL, 0) == osOK) {
	  if (msgReceived->id == MSG_ID_LORA_VERTEXES_RECEIVED){
		  if(receivedFence(msgReceived, fsmParams) == HAL_OK){
			  receivedMsgLoraRX = true;
		  }
	  }
	  return HAL_OK;
  }
  return HAL_ERROR;
}

HAL_StatusTypeDef updatePosition(Message *msgReceived, fsmTaskParams *fsmParams) {
  HAL_StatusTypeDef status = HAL_ERROR;
  switch (msgReceived->id) {
    case MSG_ID_SEND_GPS:
      Position pos;
      if (msgReceived->length == 2 * sizeof(double) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
        memcpy(&pos.latitude, msgReceived->payload, sizeof(double));
        memcpy(&pos.longitude, msgReceived->payload + sizeof(double), sizeof(double));

        // //printf("FSM recibió GPS: lat=%.5f, lon=%.5f\r\n", latitude, longitude);

        fsmParams->cow->updatePosition(pos);

        status = HAL_OK;

        //printf("[FSM] Recibí posicion del GPS");
		//printf("[FSM] Posición recibida: %.6f, %.6f\n", pos.latitude, pos.longitude);

      } else {
        // Manejo de mensaje corrupto o malformado
        //printf("FSM recibió GPS con payload inválido.\r\n");
      }

      break;

    // Otros casos futuros...
    // CASE 2...
  }

  // Importante: liberar memoria del mensaje recibido
  delete msgReceived;
  return status;
}

void sendPosition(uint8_t msgId, ModuleId_t dest, fsmTaskParams *fsmParams) {
  Message* msg = new Message(msgId, ModuleId_t::FSM, dest, 2 * sizeof(double));  // [latitud, longitud] = 2 doubles
  Position pos = fsmParams->cow->getPosition();
  memcpy(msg->payload, &(pos.latitude), sizeof(double));
  memcpy(msg->payload + sizeof(double), &(pos.longitude), sizeof(double));
  osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
}

HAL_StatusTypeDef loraTxResponse(Message *msgReceived) {
  HAL_StatusTypeDef status = HAL_ERROR;

  switch (msgReceived->id) {
    case MSG_ID_LORA_SEND_POSITION_FEEDBACK:
      status = HAL_OK;
      // //printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;
}

// Recibe latitud y longitud (double) de LORA_RX con un tamaño fijo.
HAL_StatusTypeDef receivedFence(Message *msgReceived, fsmTaskParams *fsmParams) {
  HAL_StatusTypeDef status = HAL_ERROR;

  switch (msgReceived->id) {
    case MSG_ID_LORA_VERTEXES_RECEIVED:
      std::vector<Vertex> vertices;
      int vertexCount = msgReceived->length / sizeof(Vertex);
      vertices.resize(vertexCount);
      memcpy(vertices.data(), msgReceived->payload, vertexCount * sizeof(Vertex));
      if (msgReceived->payload != nullptr && vertexCount >= 3) {
        fsmParams->fence->saveVertices(vertices);

        status = HAL_OK;
        // //printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      
      } else {
        //printf("FSM recibió payload inválido para vértices.\r\n");
      }

      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;
}

void updateFence(fsmTaskParams *fsmParams) {
  fsmParams->fence->createLimits();
}

HAL_StatusTypeDef isInFence(fsmTaskParams *fsmParams) {
  if (fsmParams->cow->getCurrentZone() != zone_t::GREEN_ZONE) {

	  return HAL_OK;
  }

  return HAL_ERROR;
}

HAL_StatusTypeDef updateDistAndZone(Message *msgReceived,fsmTaskParams *fsmParams) {
  HAL_StatusTypeDef status = HAL_ERROR;
  zone_t zone;
  float dist;

  switch (msgReceived->id) {
    case MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE: 
      if (msgReceived->payload != nullptr && msgReceived->length == (sizeof(zone_t) + sizeof(float))) {
        memcpy(&zone, msgReceived->payload, sizeof(zone_t));
        memcpy(&dist, msgReceived->payload + sizeof(zone_t), sizeof(float));

        fsmParams->cow->updateCurrentZone(zone);
        fsmParams->cow->updateDistanceToLimit(dist);
        status = HAL_OK;
        // //printf("FSM recibió %lu vértices del cerco\r\n", vertexCount);
      
      } else {
        //printf("FSM recibió ZONE AND DISTANCE con payload inválido.\r\n");
      }

      break;

    // Otros casos futuros...
  }

  delete msgReceived;
  return status;  
}

HAL_StatusTypeDef updateAcceleration(Message *msgReceived, fsmTaskParams *fsmParams) {
  HAL_StatusTypeDef status = HAL_ERROR;
  switch (msgReceived->id) {
    case MSG_ID_SEND_IMU:
      Acceleration acc;
      if (msgReceived->length == 3 * sizeof(float) && msgReceived->payload != nullptr) { // Es necesario verificar el length?
        memcpy(&acc.ax, msgReceived->payload, sizeof(float));
        memcpy(&acc.ay, msgReceived->payload + sizeof(float), sizeof(float));
        memcpy(&acc.az, msgReceived->payload + 2 * sizeof(float), sizeof(float));

        // //printf("FSM recibió GPS: lat=%.5f, lon=%.5f\r\n", latitude, longitude);

        fsmParams->cow->updateAcceleration(acc);

        status = HAL_OK;

      } else {
        // Manejo de mensaje corrupto o malformado
        //printf("FSM recibió IMU con payload inválido.\r\n");
      }

      break;

    // Otros casos futuros...
    // CASE 2...
  }

  // Importante: liberar memoria del mensaje recibido
  delete msgReceived;
  return status;
}

void updateState(fsmTaskParams *fsmParams) {
	fsmParams->cow->updateState(classifyMotion(fsmParams->cow->getAcceleration()));
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
  osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
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
  osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);
}

HAL_StatusTypeDef receivedStimulusResponse(Message *msgReceived) {
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
