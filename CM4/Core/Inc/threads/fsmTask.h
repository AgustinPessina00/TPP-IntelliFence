#ifndef FSMTASK_H
#define FSMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "messages.h"
#include "cow.h"
#include "fence.h"

#define NEAR_LIMIT  10.0f   // en metros
#define MAX_TRIES   10

typedef float distance_t;

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
  STARTUP_ROUTINE_END
} StartupRoutineState_t;

typedef enum {
  FENCE_TRANSITION_BEGIN,
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
  GREEN_ZONE_END
} GreenZoneState_t;

typedef enum {
  STIMULUS_ZONE_BEGIN,
  STIMULUS_ZONE_LIGHT_BLUE,
  STIMULUS_ZONE_WAIT_LIGHT_BLUE_RESPONSE,
  STIMULUS_ZONE_BLUE,
  STIMULUS_ZONE_WAIT_BLUE_RESPONSE,
  STIMULUS_ZONE_DARK_BLUE,
  STIMULUS_ZONE_WAIT_DARK_BLUE_RESPONSE,
  STIMULUS_ZONE_YELLOW,
  STIMULUS_ZONE_WAIT_YELLOW_RESPONSE,
  STIMULUS_ZONE_RED,
  STIMULUS_ZONE_WAIT_RED_RESPONSE,
  STIMULUS_ZONE_END
} StimulusZone_t;


class FSM {

private:
  void enterLowPowerSleep();
  CowState classifyMotion(Acceleration acc);

  MainFSM_t mainFSM;
  NormalOpFSM_t normalOpFSM;

  StartupRoutineState_t startupRutineState;

  InitializeState_t initializeState;
  GreenZoneState_t greenZoneState;
  StimulusZone_t stimulusZoneState;

  FenceTransitionState_t fenceTransitionState;

  uint8_t tries;
  
public:
  FSM();
  
  void fsmTask(void *argument);
  void runStartupRoutineFSM();
  void runNormalOperationFSM();
  void runInitializeFSM();
  void runGreenZoneFSM();
  void runStimulusZoneFSM();

};






enum class GpsRate {
  STOP,
  SLOW,
  MEDIUM,
  FAST
};

void fsmTask(void *argument);

void runStartupRoutineFSM();
void runNormalOperationFSM();
void runFenceTransitionFSM();

StartupRoutineState_t startupState = STARTUP_ROUTINE_BEGIN;


void enterLowPowerSleep(void);
CowState classifyMotion(Acceleration imu);

distance_t calculateDistanceToLimit(cow.getPosition(), fence.getSegments());
zone_t getZoneForDistance(distance_t dist, Fence fence);

#ifdef __cplusplus
}
#endif

#endif // FSMTASK_H
