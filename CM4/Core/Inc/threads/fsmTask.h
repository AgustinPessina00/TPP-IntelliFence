#ifndef FSMTASK_H
#define FSMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "stm32wlxx_hal.h"

#include "messages.h"
#include "cow.h"
#include "fence.h"

#define NEAR_LIMIT  10.0f   // en metros
#define MAX_TRIES   10

enum class GpsRate {
  STOP,
  SLOW,
  MEDIUM,
  FAST
};

enum class MainFSM_t {
  STARTUP_ROUTINE,
  NORMAL_OPERATION,
  FENCE_TRANSITION
};

// NORMAL_OPERATION FSM es una FSM compuesta
enum class NormalOpFSM_t {
  INITIALIZE,
  GREEN_ZONE,
  STIMULUS_ZONE
};

typedef enum {
  STARTUP_ROUTINE_BEGIN,
  STARTUP_ROUTINE_REQUEST_POSITION,
  STARTUP_ROUTINE_WAIT_POSITION,
  STARTUP_ROUTINE_SEND_POSITION_LORA,
  STARTUP_ROUTINE_WAIT_FENCE,
  STARTUP_ROUTINE_SAVE_FENCE,
  STARTUP_ROUTINE_REQUEST_NEW_POSITION,
  STARTUP_ROUTINE_WAIT_NEW_POSITION,
  STARTUP_ROUTINE_REQUEST_ZONE,
  STARTUP_ROUTINE_EVALUATE_ZONE,
  STARTUP_ROUTINE_END
} StartupRoutineState_t;

typedef enum {
  FENCE_TRANSITION_BEGIN,
  FENCE_TRANSITION_GPSRATE_FAST,
  FENCE_TRANSITION_WAIT_GPS_ADQ_TIME,
  FENCE_TRANSITION_REQUEST_POSITION,
  FENCE_TRANSITION_WAIT_POSITION,
  FENCE_TRANSITION_UPDATE_PARTIAL_FENCE,
  FENCE_TRANSITION_REQUEST_NEW_POSITION,
  FENCE_TRANSITION_WAIT_NEW_POSITION,
  FENCE_TRANSITION_REQUEST_ZONE,
  FENCE_TRANSITION_WAIT_ZONE,
  FENCE_TRANSITION_EVALUATE_ZONE,
  FECNE_TRANSITION_SEND_ZONE,
  FENCE_TRANSITION_WAIT_RESPONSE,
  FENCE_TRANSITION_END
} FenceTransitionState_t;

typedef enum {
  INITIALIZE_BEGIN,
  INITIALIZE_REQUEST_POSITION,
  INITIALIZE_WAIT_POSITION,
  INITIALIZE_REQUEST_ZONE,
  INITIALIZE_WAIT_ZONE,
  INITIALIZE_EVALUATE_ZONE,
  INITIALIZE_END
} InitializeState_t;

typedef enum {
  GREEN_ZONE_BEGIN,
  GREEN_ZONE_REQUEST_ACCELERATION,
  GREEN_ZONE_WAIT_ACCELERATION,
  GREEN_ZONE_EVALUATE_COWSTATE,
  GREEN_ZONE_GRAZING,
  GREEN_ZONE_SLEEP,
  GREEN_ZONE_MOVEMENT,
  GREEN_ZONE_NEAR_LIMIT,
  GREEN_ZONE_FAR_LIMIT,
  GREEN_ZONE_WAIT_GPS_ADQ_TIME,
  GREEN_ZONE_END
} GreenZoneState_t;

typedef enum {
  STIMULUS_ZONE_BEGIN,
  STIMULUS_ZONE_SEND_ZONE,
  STIMULUS_ZONE_WAIT_RESPONSE,
  STIMULUS_ZONE_END
} StimulusZone_t;


class FSM {

private:

  void sendMessage(uint8_t msgId, ModuleId_t dest);
  HAL_StatusTypeDef dequeuedMessage();
  HAL_StatusTypeDef updatePosition();
  void sendPosition(uint8_t msgId, ModuleId_t dest);
  HAL_StatusTypeDef recievedFence();
  void updateFence();
  HAL_StatusTypeDef isInFence();
  HAL_StatusTypeDef updateDistAndZone();
  HAL_StatusTypeDef updateAcceleration();
  void updateState();
  CowState classifyMotion(Acceleration acc);
  HAL_StatusTypeDef gpsResponse();
  HAL_StatusTypeDef updateGpsAdqTime(GpsRate gpsRate);
  void enterLowPowerSleep();
  void sendZoneToStimulus(zone_t zone, ModuleId_t dest);
  HAL_StatusTypeDef recievedStimulusResponse();
  

  MainFSM_t mainFSM;
  NormalOpFSM_t normalOpFSM;

  StartupRoutineState_t startupRutineState;

  InitializeState_t initializeState;
  GreenZoneState_t greenZoneState;
  StimulusZone_t stimulusZoneState;

  FenceTransitionState_t fenceTransitionState;

  uint8_t tries;

  Message* msgReceived;
  
public:
  FSM();
  
  void fsmTask(void *argument);
  void runStartupRoutineFSM();
  void runNormalOperationFSM();
  void runInitializeFSM();
  void runGreenZoneFSM();
  void runStimulusZoneFSM();
  void runFenceTransitionFSM();

};


#ifdef __cplusplus
}
#endif

#endif // FSMTASK_H
