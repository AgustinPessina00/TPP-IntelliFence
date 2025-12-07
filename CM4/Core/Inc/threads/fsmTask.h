#ifndef FSMTASK_NEW_H
#define FSMTASK_NEW_H

#include "EmbeddedMessage.h"
#include "stm32wlxx_hal.h"
#include "zone.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "cow.h"
#include "fence.h"
//TODO: Integration of GetZone in this task
#include "getZone.h"

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
  STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE,
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
  FENCE_TRANSITION_DISABLE_STIMULUS,
  FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE,
  FENCE_TRANSITION_UPDATE_FENCE,
  FENCE_TRANSITION_GPSRATE_FAST,
  FENCE_TRANSITION_WAIT_GPS_ADQ_TIME,
  FENCE_TRANSITION_REQUEST_POSITION,
  FENCE_TRANSITION_WAIT_POSITION,
  FENCE_TRANSITION_REQUEST_ZONE,
  FENCE_TRANSITION_EVALUATE_ZONE,
  FENCE_TRANSITION_END
} FenceTransitionState_t;

typedef enum {
  INITIALIZE_BEGIN,
  INITIALIZE_REQUEST_POSITION,
  INITIALIZE_WAIT_POSITION,
  INITIALIZE_REQUEST_ZONE,
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

// ============================================================================
// MAIN FSM FUNCTIONS
// ============================================================================

#ifdef __cplusplus

void fsmTask(void *argument);

void runStartupRoutineFSM(MainFSM_t& mainFSM, StartupRoutineState_t* startupRoutineState, 
                         EmbeddedMessage_t** msgReceived, uint8_t& tries, Cow& cow, Fence& fence);

void runNormalOperationFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState, 
                          GreenZoneState_t& greenZoneState, StimulusZone_t& stimulusZoneState, 
                          EmbeddedMessage_t** msgReceived, uint8_t& tries, Cow& cow, Fence& fence);

void runInitializeFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState, 
                     EmbeddedMessage_t** msgReceived, uint8_t& tries, Cow& cow, Fence& fence);

void runGreenZoneFSM(NormalOpFSM_t& normalOpFSM, GreenZoneState_t& greenZoneState, 
                    EmbeddedMessage_t** msgReceived, uint8_t& tries, Cow& cow, Fence& fence);

void runStimulusZoneFSM(NormalOpFSM_t& normalOpFSM, StimulusZone_t& stimulusZoneState, 
                       EmbeddedMessage_t** msgReceived, uint8_t& tries, Cow& cow, Fence& fence);

void runFenceTransitionFSM(MainFSM_t& mainFSM, FenceTransitionState_t& fenceTransitionState, 
                          EmbeddedMessage_t** msgReceived, uint8_t& tries, Cow& cow, Fence& fence);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void sendMessage(uint8_t msgId, ModuleId_t dest);
HAL_StatusTypeDef dequeuedMessage(EmbeddedMessage_t **msgReceived, Fence& fence);
HAL_StatusTypeDef updatePosition(Cow& cow, EmbeddedMessage_t *msgReceived);
void sendPosition(uint8_t msgId, ModuleId_t dest, Cow& cow);
HAL_StatusTypeDef loraTxResponse(EmbeddedMessage_t *msgReceived);
HAL_StatusTypeDef receivedFence(EmbeddedMessage_t *msgReceived, Fence& fence);
void updateFence(Fence& fence);
HAL_StatusTypeDef isInFence(Cow& cow);
HAL_StatusTypeDef updateDistAndZone(EmbeddedMessage_t *msgReceived, Cow& cow);
HAL_StatusTypeDef updateAcceleration(EmbeddedMessage_t *msgReceived, Cow& cow);
void updateState(Cow& cow);
HAL_StatusTypeDef gpsResponse(EmbeddedMessage_t *msgReceived);
void updateGpsAdqTime(GpsRate gpsRate);
void enterLowPowerSleep();
void sendZoneToStimulus(zone_t zone, ModuleId_t dest);
HAL_StatusTypeDef receivedStimulusResponse(EmbeddedMessage_t *msgReceived);

#endif // __cplusplus


#ifdef __cplusplus
}
#endif

#endif // FSMTASK_NEW_H
