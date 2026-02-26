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

// ============================================================================
// TIMEOUT CONFIGURATION (en milisegundos)
// ============================================================================
#define GPS_TIMEOUT_MS       10000  // 10 segundos - GPS puede tardar en obtener fix
#define GPS_PSM_TIMEOUT_MS   20000  // 20 segundos - GPS puede tardar en aplicar configuración PSM
#define LORA_TX_TIMEOUT_MS   10000  // 10 segundos - Transmisión LoRa
#define LORA_RX_TIMEOUT_MS   10000  // 60 segundos - Espera de mensajes LoRa entrantes
#define IMU_TIMEOUT_MS       10000  // 10 segundos - Burst collection (52 samples @ 26Hz = 2000ms) + processing
#define DISTANCE_TIMEOUT_MS   3000  // 3 segundos  - Cálculo de zona y distancia
#define STIMULUS_TIMEOUT_MS   5000  // 5 segundos  - Respuesta del módulo de estímulo
#define GPS_CONFIG_TIMEOUT_MS 30000  // 30 segundos  - Configuración de tasa GPS
#define FSM_TICKS_GREEN_ZONE 1000  // 1 segundo - Ticks entre iteraciones en zona verde (ajustable según necesidades)
#define FSM_TICKS_NEAR_LIMIT FSM_TICKS_GREEN_ZONE/2
#define FSM_TICKS_STIMULOUS_ZONE FSM_TICKS_GREEN_ZONE/5
#define NEAR_LIMIT  10.0f   // en metros


extern uint32_t fsmTicks;
// ============================================================================
// TIMEOUT CONTEXT STRUCTURE
// ============================================================================
typedef struct {
    uint32_t startTick;    // Tick de inicio de la operación
    uint32_t timeoutMs;    // Timeout en milisegundos
} TimeoutContext_t;

// Helper para iniciar timeout
inline void Timeout_Start(TimeoutContext_t* ctx, uint32_t timeoutMs) {
    ctx->startTick = xTaskGetTickCount();
    ctx->timeoutMs = timeoutMs;
}

// Helper para verificar si expiró el timeout
inline bool Timeout_IsExpired(const TimeoutContext_t* ctx) {
    uint32_t elapsed = (xTaskGetTickCount() - ctx->startTick) * portTICK_PERIOD_MS;
    return (elapsed >= ctx->timeoutMs);
}

// Helper para obtener tiempo transcurrido
inline uint32_t Timeout_GetElapsed(const TimeoutContext_t* ctx) {
    return (xTaskGetTickCount() - ctx->startTick) * portTICK_PERIOD_MS;
}

enum class GpsRate {
  GREEN_ZONE_RATE,
  NEAR_LIMIT_RATE,
  CONTINUOUS_RATE,
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
  STARTUP_ROUTINE_WAIT_JOIN,
  STARTUP_ROUTINE_REQUEST_POSITION,
  STARTUP_ROUTINE_WAIT_POSITION,
  STARTUP_ROUTINE_REQUEST_GPS_CONFIGURATION_PSM,
  STARTUP_ROUTINE_WAIT_GPS_CONFIGURATION_PSM,
  STARTUP_ROUTINE_SEND_POSITION_LORA,
  STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE,
  STARTUP_ROUTINE_WAIT_FENCE,
  STARTUP_ROUTINE_SAVE_FENCE,
  STARTUP_ROUTINE_REQUEST_NEW_POSITION,
  STARTUP_ROUTINE_WAIT_NEW_POSITION,
  STARTUP_ROUTINE_REQUEST_ZONE,
  STARTUP_ROUTINE_END
} StartupRoutineState_t;

typedef enum {
  FENCE_TRANSITION_BEGIN,
  FENCE_TRANSITION_DISABLE_STIMULUS,
  FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE,
  FENCE_TRANSITION_GPSRATE_FAST,
  FENCE_TRANSITION_WAIT_GPS_ADQ_TIME,
  FENCE_TRANSITION_REQUEST_POSITION,
  FENCE_TRANSITION_WAIT_POSITION,
  FENCE_TRANSITION_REQUEST_ZONE,
  FENCE_TRANSITION_END
} FenceTransitionState_t;

typedef enum {
  INITIALIZE_BEGIN,
  INITIALIZE_REQUEST_POSITION,
  INITIALIZE_WAIT_POSITION,
  INITIALIZE_REQUEST_ZONE,
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
                         TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runNormalOperationFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState, 
                          GreenZoneState_t& greenZoneState, StimulusZone_t& stimulusZoneState, 
                          TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runInitializeFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState, 
                     TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runGreenZoneFSM(NormalOpFSM_t& normalOpFSM, GreenZoneState_t& greenZoneState, 
                    TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runStimulusZoneFSM(NormalOpFSM_t& normalOpFSM, StimulusZone_t& stimulusZoneState, 
                       TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runFenceTransitionFSM(MainFSM_t& mainFSM, FenceTransitionState_t& fenceTransitionState, 
                          TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Message queue operations
void sendMessage(uint8_t msgId, ModuleId_t dest);
HAL_StatusTypeDef dequeueMessage(EmbeddedMessage_t **msg);
HAL_StatusTypeDef waitForMessage(uint8_t expectedMsgId, TimeoutContext_t& timeout, 
                                 EmbeddedMessage_t** outMsg);

// Message processors
HAL_StatusTypeDef processLoRaTxResponse(EmbeddedMessage_t *msg);
HAL_StatusTypeDef processFenceMessage(EmbeddedMessage_t *msg, Fence& fence);
HAL_StatusTypeDef processImuMessage(EmbeddedMessage_t *msg, Cow& cow);
HAL_StatusTypeDef processGpsConfigResponse(EmbeddedMessage_t *msg);
HAL_StatusTypeDef processStimulusResponse(EmbeddedMessage_t *msg);

// High-level operations
void sendPosition(uint8_t msgId, ModuleId_t dest, Cow& cow);
void sendZoneToStimulus(zone_t zone, ModuleId_t dest);
void updateGpsAdqTime(GpsRate gpsRate);
void enterLowPowerSleep();

// Cow operations
HAL_StatusTypeDef isInGreenZone(Cow& cow);
void updateState(Cow& cow);

#endif // __cplusplus


#ifdef __cplusplus
}
#endif

#endif // FSMTASK_NEW_H
