#include "FreeRTOS.h"
#include "task.h"
#include "threads/fsmTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "zone.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <string.h>
#include <math.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

// Variables globales para la FSM (static para no usar stack)
static bool receivedMsgLoraRX = false;


// Variables de estado de las FSMs (static para ahorrar stack)
static MainFSM_t s_mainFSM = MainFSM_t::STARTUP_ROUTINE;
static NormalOpFSM_t s_normalOpFSM = NormalOpFSM_t::INITIALIZE;
static StartupRoutineState_t s_startupRoutineState = STARTUP_ROUTINE_BEGIN;
static InitializeState_t s_initializeState = INITIALIZE_BEGIN;
static GreenZoneState_t s_greenZoneState = GREEN_ZONE_BEGIN;
static StimulusZone_t s_stimulusZoneState = STIMULUS_ZONE_BEGIN;
static FenceTransitionState_t s_fenceTransitionState = FENCE_TRANSITION_BEGIN;

// ============================================================================
// MAIN FSM TASK
// ============================================================================

void fsmTask(void *argument) {
    (void)argument; // Unused parameter

    // Inicializar objetos Cow y Fence en memoria static (una sola vez)
    DeviceUID deviceUID = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    static Cow cow(deviceUID);
    static Fence fence;
    
    EmbeddedMessage_t *msgReceived = NULL;
    uint8_t tries = 0;
    
    RTOS_LOG_INFO("[FSM] Task initialized successfully\n");

    while(1) {

        //test para el stimulus
        // for(int i = GREEN_ZONE; i <= BLACK_ZONE; i++) {
        //     zone_t zone = static_cast<zone_t>(i);
        //     RTOS_LOG_INFO("[FSM] Zone enum value: %d\n", zone);
        //     sendZoneToStimulus(zone, MODULE_STIMULUS);
        //     osDelay(1000);
        // }
        
        switch (s_mainFSM) {
            case MainFSM_t::STARTUP_ROUTINE:
                runStartupRoutineFSM(s_mainFSM, &s_startupRoutineState, &msgReceived, tries, cow, fence);
                break;
                
            case MainFSM_t::NORMAL_OPERATION:
                runNormalOperationFSM(s_normalOpFSM, s_initializeState, s_greenZoneState, 
                                     s_stimulusZoneState, &msgReceived, tries, cow, fence);
                if (receivedMsgLoraRX) {
                    s_mainFSM = MainFSM_t::FENCE_TRANSITION;
                    receivedMsgLoraRX = false;
                }
                break;
                
            case MainFSM_t::FENCE_TRANSITION:
                runFenceTransitionFSM(s_mainFSM, s_fenceTransitionState, &msgReceived, tries, cow, fence);
                break;
        }
        
        osDelay(100);
    }
}

// ============================================================================
// STARTUP ROUTINE FSM
// ============================================================================

void runStartupRoutineFSM(MainFSM_t& mainFSM, StartupRoutineState_t* state, 
                         EmbeddedMessage_t** msgReceived, uint8_t& tries, 
                         Cow& cow, Fence& fence) {
    switch (*state) {
        case STARTUP_ROUTINE_BEGIN:
            tries = 0;
            RTOS_LOG_INFO("[FSM] Starting STARTUP_ROUTINE\n");
            *state = STARTUP_ROUTINE_REQUEST_POSITION;
            break;
            
        case STARTUP_ROUTINE_REQUEST_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_SENSOR_ACQ);
            RTOS_LOG_DEBUG("[FSM] Requesting initial GPS position\n");
            *state = STARTUP_ROUTINE_WAIT_POSITION;
            break;
            
        case STARTUP_ROUTINE_WAIT_POSITION:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updatePosition(cow, *msgReceived) == HAL_OK) {
                    *state = STARTUP_ROUTINE_SEND_POSITION_LORA;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    RTOS_LOG_WARN("[FSM] GPS position timeout, retrying...\n");
                    *state = STARTUP_ROUTINE_REQUEST_POSITION;
                }
            }
            break;
            
        case STARTUP_ROUTINE_SEND_POSITION_LORA:
            sendPosition(MSG_ID_LORA_SEND_POSITION, MODULE_LORA_TX, cow);
            *state = STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE;
            break;
            
        case STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (loraTxResponse(*msgReceived) == HAL_OK) {
                    *state = STARTUP_ROUTINE_WAIT_FENCE;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    *state = STARTUP_ROUTINE_SEND_POSITION_LORA;
                }
            }
            break;
            
        case STARTUP_ROUTINE_WAIT_FENCE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (receivedMsgLoraRX) {
                    *state = STARTUP_ROUTINE_SAVE_FENCE;
                    receivedMsgLoraRX = false;
                }
            }
            break;
            
        case STARTUP_ROUTINE_SAVE_FENCE:
            updateFence(fence);
            *state = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
            break;
            
        case STARTUP_ROUTINE_REQUEST_NEW_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_SENSOR_ACQ);
            *state = STARTUP_ROUTINE_WAIT_NEW_POSITION;
            break;
            
        case STARTUP_ROUTINE_WAIT_NEW_POSITION:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updatePosition(cow, *msgReceived) == HAL_OK) {
                    *state = STARTUP_ROUTINE_REQUEST_ZONE;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    *state = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
                }
            }
            break;
            
        case STARTUP_ROUTINE_REQUEST_ZONE:
            sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, MODULE_DISTANCE);
            *state = STARTUP_ROUTINE_EVALUATE_ZONE;
            break;
            
        case STARTUP_ROUTINE_EVALUATE_ZONE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updateDistAndZone(*msgReceived, cow) == HAL_OK) {
                    *state = STARTUP_ROUTINE_END;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    *state = STARTUP_ROUTINE_REQUEST_ZONE;
                }
            }
            break;
            
        case STARTUP_ROUTINE_END:
            *state = STARTUP_ROUTINE_BEGIN;
            if (isInFence(cow) == HAL_OK) {
                mainFSM = MainFSM_t::NORMAL_OPERATION;
                RTOS_LOG_INFO("[FSM] Startup complete - entering NORMAL_OPERATION\n");
            } else {
                mainFSM = MainFSM_t::FENCE_TRANSITION;
                RTOS_LOG_INFO("[FSM] Startup complete - entering FENCE_TRANSITION\n");
            }
            break;
    }
}

// ============================================================================
// NORMAL OPERATION FSM
// ============================================================================

void runNormalOperationFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState,
                          GreenZoneState_t& greenZoneState, StimulusZone_t& stimulusZoneState,
                          EmbeddedMessage_t** msgReceived, uint8_t& tries, 
                          Cow& cow, Fence& fence) {
    switch (normalOpFSM) {
        case NormalOpFSM_t::INITIALIZE:
            runInitializeFSM(normalOpFSM, initializeState, msgReceived, tries, cow, fence);
            break;
            
        case NormalOpFSM_t::GREEN_ZONE:
            runGreenZoneFSM(normalOpFSM, greenZoneState, msgReceived, tries, cow, fence);
            break;
            
        case NormalOpFSM_t::STIMULUS_ZONE:
            runStimulusZoneFSM(normalOpFSM, stimulusZoneState, msgReceived, tries, cow, fence);
            break;
    }
}

// ============================================================================
// INITIALIZE SUB-FSM
// ============================================================================

void runInitializeFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState,
                     EmbeddedMessage_t** msgReceived, uint8_t& tries, 
                     Cow& cow, Fence& fence) {
    switch (initializeState) {
        case INITIALIZE_BEGIN:
            tries = 0;
            initializeState = INITIALIZE_REQUEST_POSITION;
            break;
            
        case INITIALIZE_REQUEST_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_SENSOR_ACQ);
            initializeState = INITIALIZE_WAIT_POSITION;
            break;
            
        case INITIALIZE_WAIT_POSITION:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updatePosition(cow, *msgReceived) == HAL_OK) {
                    initializeState = INITIALIZE_REQUEST_ZONE;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    initializeState = INITIALIZE_REQUEST_POSITION;
                }
            }
            break;
            
        case INITIALIZE_REQUEST_ZONE:
            sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, MODULE_DISTANCE);
            initializeState = INITIALIZE_EVALUATE_ZONE;
            break;
            
        case INITIALIZE_EVALUATE_ZONE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updateDistAndZone(*msgReceived, cow) == HAL_OK) {
                    initializeState = INITIALIZE_END;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    initializeState = INITIALIZE_REQUEST_ZONE;
                }
            }
            break;
            
        case INITIALIZE_END:
            initializeState = INITIALIZE_BEGIN;
            if (isInFence(cow) == HAL_OK) {
                normalOpFSM = NormalOpFSM_t::GREEN_ZONE;
                sendZoneToStimulus(cow.getCurrentZone(), MODULE_STIMULUS);
            } else {
                normalOpFSM = NormalOpFSM_t::STIMULUS_ZONE;
            }
            break;
    }
}

// ============================================================================
// GREEN ZONE SUB-FSM
// ============================================================================

void runGreenZoneFSM(NormalOpFSM_t& normalOpFSM, GreenZoneState_t& greenZoneState,
                    EmbeddedMessage_t** msgReceived, uint8_t& tries, 
                    Cow& cow, Fence& fence) {
    // CowState state;
    
    switch (greenZoneState) {
        case GREEN_ZONE_BEGIN:
            sendZoneToStimulus(cow.getCurrentZone(), MODULE_STIMULUS);
            tries = 0;
            greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
            break;
            
        case GREEN_ZONE_REQUEST_ACCELERATION:
            sendMessage(MSG_ID_REQUEST_IMU, MODULE_SENSOR_ACQ);
            greenZoneState = GREEN_ZONE_WAIT_ACCELERATION;
            break;
            
        case GREEN_ZONE_WAIT_ACCELERATION:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updateAcceleration(*msgReceived, cow) == HAL_OK) {
                    greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
                }
            }
            break;
            
        case GREEN_ZONE_EVALUATE_COWSTATE:
            updateState(cow);
            switch (cow.getState()) {
                case CowState::GRAZING:
                    greenZoneState = GREEN_ZONE_GRAZING;
                    break;
                case CowState::SLEEP:
                    greenZoneState = GREEN_ZONE_SLEEP;
                    break;
                case CowState::MOVEMENT:
                    greenZoneState = GREEN_ZONE_MOVEMENT;
                    break;
            }
            break;
            
        case GREEN_ZONE_GRAZING:
            updateGpsAdqTime(GpsRate::SLOW);
            greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
            break;
            
        case GREEN_ZONE_SLEEP:
            updateGpsAdqTime(GpsRate::STOP);
            enterLowPowerSleep();
            greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
            break;
            
        case GREEN_ZONE_MOVEMENT:
            if (cow.getDistanceToLimit() <= NEAR_LIMIT) {
                greenZoneState = GREEN_ZONE_NEAR_LIMIT;
            } else {
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
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (gpsResponse(*msgReceived) == HAL_OK) {
                    greenZoneState = GREEN_ZONE_END;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
                }
            }
            break;
            
        case GREEN_ZONE_END:
            greenZoneState = GREEN_ZONE_BEGIN;
            normalOpFSM = NormalOpFSM_t::INITIALIZE;
            break;
    }
}

// ============================================================================
// STIMULUS ZONE SUB-FSM
// ============================================================================

void runStimulusZoneFSM(NormalOpFSM_t& normalOpFSM, StimulusZone_t& stimulusZoneState,
                       EmbeddedMessage_t** msgReceived, uint8_t& tries, 
                       Cow& cow, Fence& fence) {
    switch (stimulusZoneState) {
        case STIMULUS_ZONE_BEGIN:
            stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
            break;
            
        case STIMULUS_ZONE_SEND_ZONE:
            sendZoneToStimulus(cow.getCurrentZone(), MODULE_STIMULUS);
            stimulusZoneState = STIMULUS_ZONE_WAIT_RESPONSE;
            break;
            
        case STIMULUS_ZONE_WAIT_RESPONSE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (receivedStimulusResponse(*msgReceived) == HAL_OK) {
                    stimulusZoneState = STIMULUS_ZONE_END;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
                }
            }
            break;
            
        case STIMULUS_ZONE_END:
            stimulusZoneState = STIMULUS_ZONE_BEGIN;
            normalOpFSM = NormalOpFSM_t::INITIALIZE;
            break;
    }
}

// ============================================================================
// FENCE TRANSITION FSM
// ============================================================================

void runFenceTransitionFSM(MainFSM_t& mainFSM, FenceTransitionState_t& fenceTransitionState,
                          EmbeddedMessage_t** msgReceived, uint8_t& tries, 
                          Cow& cow, Fence& fence) {
    switch (fenceTransitionState) {
        case FENCE_TRANSITION_BEGIN:
            tries = 0;
            RTOS_LOG_INFO("[FSM] Starting FENCE_TRANSITION\n");
            fenceTransitionState = FENCE_TRANSITION_DISABLE_STIMULUS;
            break;
            
        case FENCE_TRANSITION_DISABLE_STIMULUS:
            sendZoneToStimulus(BLACK_ZONE, MODULE_STIMULUS);
            fenceTransitionState = FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE;
            break;
            
        case FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (receivedStimulusResponse(*msgReceived) == HAL_OK) {
                    fenceTransitionState = FENCE_TRANSITION_UPDATE_FENCE;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    fenceTransitionState = FENCE_TRANSITION_DISABLE_STIMULUS;
                }
            }
            break;
            
        case FENCE_TRANSITION_UPDATE_FENCE:
            updateFence(fence);
            fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
            break;
            
        case FENCE_TRANSITION_GPSRATE_FAST:
            updateGpsAdqTime(GpsRate::FAST);
            fenceTransitionState = FENCE_TRANSITION_WAIT_GPS_ADQ_TIME;
            break;
            
        case FENCE_TRANSITION_WAIT_GPS_ADQ_TIME:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (gpsResponse(*msgReceived) == HAL_OK) {
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
                }
            }
            break;
            
        case FENCE_TRANSITION_REQUEST_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_SENSOR_ACQ);
            fenceTransitionState = FENCE_TRANSITION_WAIT_POSITION;
            break;
            
        case FENCE_TRANSITION_WAIT_POSITION:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updatePosition(cow, *msgReceived) == HAL_OK) {
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_ZONE;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
                }
            }
            break;
            
        case FENCE_TRANSITION_REQUEST_ZONE:
            sendMessage(MSG_ID_REQUEST_ZONE_TO_FENCE, MODULE_DISTANCE);
            fenceTransitionState = FENCE_TRANSITION_EVALUATE_ZONE;
            break;
            
        case FENCE_TRANSITION_EVALUATE_ZONE:
            if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
                if (updateDistAndZone(*msgReceived, cow) == HAL_OK) {
                    fenceTransitionState = FENCE_TRANSITION_END;
                    tries = 0;
                }
            } else {
                tries++;
                if (tries >= MAX_TRIES) {
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_ZONE;
                }
            }
            break;
            
        case FENCE_TRANSITION_END:
            fenceTransitionState = FENCE_TRANSITION_BEGIN;
            mainFSM = MainFSM_t::NORMAL_OPERATION;
            RTOS_LOG_INFO("[FSM] Fence transition complete\n");
            break;
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void sendMessage(uint8_t msgId, ModuleId_t dest) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != NULL) {
        EmbeddedMessage_Create(msg, msgId, MODULE_FSM, dest);
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
    } else {
        RTOS_LOG_ERROR("[FSM] Failed to allocate message for ID:%d\n", msgId);
    }
}

HAL_StatusTypeDef dequeuedMessage(EmbeddedMessage_t **msgReceived, Fence& fence) {
    if (osMessageQueueGet(fsmQueueHandle, msgReceived, NULL, 0) == osOK) {
        if ((*msgReceived)->id == MSG_ID_LORA_VERTEXES_RECEIVED) {
            if (receivedFence(*msgReceived, fence) == HAL_OK) {
                receivedMsgLoraRX = true;
            }
        }
        return HAL_OK;
    }
    return HAL_ERROR;
}

HAL_StatusTypeDef updatePosition(Cow& cow, EmbeddedMessage_t *msgReceived) {
    if (msgReceived->id != MSG_ID_SEND_GPS) {
        MessagePool_Free(msgReceived);
        return HAL_ERROR;
    }
    
    if (msgReceived->length == 2 * sizeof(double)) {
        double latitude, longitude;
        memcpy(&latitude, msgReceived->payload, sizeof(double));
        memcpy(&longitude, msgReceived->payload + sizeof(double), sizeof(double));
        
        RTOS_LOG_DEBUG("[FSM] GPS position: lat=%.6f, lon=%.6f\n", latitude, longitude);
        
        cow.updatePosition({latitude, longitude});
        
        MessagePool_Free(msgReceived);
        return HAL_OK;
    }
    
    RTOS_LOG_WARN("[FSM] Invalid GPS payload\n");
    MessagePool_Free(msgReceived);
    return HAL_ERROR;
}

void sendPosition(uint8_t msgId, ModuleId_t dest, Cow& cow) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != NULL) {
        Position pos = cow.getPosition();
        double latitude = pos.latitude, longitude = pos.longitude;
        
        uint8_t data[2 * sizeof(double)];
        memcpy(data, &latitude, sizeof(double));
        memcpy(data + sizeof(double), &longitude, sizeof(double));
        
        EmbeddedMessage_CreateWithPayload(msg, msgId, MODULE_FSM, dest, data, 2 * sizeof(double));
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
    }
}

HAL_StatusTypeDef loraTxResponse(EmbeddedMessage_t *msgReceived) {
    HAL_StatusTypeDef status = HAL_ERROR;
    
    if (msgReceived->id == MSG_ID_LORA_SEND_POSITION_FEEDBACK) {
        RTOS_LOG_DEBUG("[FSM] LoRa TX confirmed position send\n");
        status = HAL_OK;
    }
    
    MessagePool_Free(msgReceived);
    return status;
}

HAL_StatusTypeDef receivedFence(EmbeddedMessage_t *msgReceived, Fence& fence) {
    if (msgReceived->id != MSG_ID_LORA_VERTEXES_RECEIVED) {
        MessagePool_Free(msgReceived);
        return HAL_ERROR;
    }
    
    uint8_t vertexCount = msgReceived->length / sizeof(Vertex);
    if (vertexCount > 0 && vertexCount <= MAX_VERTICES) {
        Vertex* vertices = (Vertex*)msgReceived->payload;
        fence.saveVertices(vertices, vertexCount);
        RTOS_LOG_INFO("[FSM] Received %d fence vertices\n", vertexCount);
    }
    
    MessagePool_Free(msgReceived);
    return HAL_OK;
}

void updateFence(Fence& fence) {
    fence.createLimits();
    RTOS_LOG_DEBUG("[FSM] Fence limits updated\n");
}

HAL_StatusTypeDef isInFence(Cow& cow) {
    return (cow.getCurrentZone() == GREEN_ZONE) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef updateDistAndZone(EmbeddedMessage_t *msgReceived, Cow& cow) {
    if (msgReceived->id != MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE) {
        MessagePool_Free(msgReceived);
        return HAL_ERROR;
    }
    
    if (msgReceived->length == (sizeof(zone_t) + sizeof(float))) {
        zone_t zone;
        float dist;
        
        memcpy(&zone, msgReceived->payload, sizeof(zone_t));
        memcpy(&dist, msgReceived->payload + sizeof(zone_t), sizeof(float));
        
        RTOS_LOG_DEBUG("[FSM] Zone:%d, Distance:%.2fm\n", zone, dist);
        
        cow.updateCurrentZone(zone);
        cow.updateDistanceToLimit(dist);
        
        MessagePool_Free(msgReceived);
        return HAL_OK;
    }
    
    MessagePool_Free(msgReceived);
    return HAL_ERROR;
}

HAL_StatusTypeDef updateAcceleration(EmbeddedMessage_t *msgReceived, Cow& cow) {
    if (msgReceived->id != MSG_ID_SEND_IMU) {
        MessagePool_Free(msgReceived);
        return HAL_ERROR;
    }
    
    if (msgReceived->length == 3 * sizeof(double)) {
        double ax, ay, az;
        memcpy(&ax, msgReceived->payload, sizeof(double));
        memcpy(&ay, msgReceived->payload + sizeof(double), sizeof(double));
        memcpy(&az, msgReceived->payload + 2 * sizeof(double), sizeof(double));
        
        RTOS_LOG_DEBUG("[FSM] IMU: ax=%.2f, ay=%.2f, az=%.2f g\n", ax, ay, az);
        
        cow.updateAcceleration({ax, ay, az});
        
        MessagePool_Free(msgReceived);
        return HAL_OK;
    }
    
    MessagePool_Free(msgReceived);
    return HAL_ERROR;
}

static CowState classifyMotion(Acceleration acc) {
    double abs_ax = fabs(acc.ax);
    double abs_ay = fabs(acc.ay);
    double abs_az = fabs(acc.az);
    
    if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az < 0.05f)
        return CowState::SLEEP;
    else if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az > 0.1f)
        return CowState::GRAZING;
    else
        return CowState::MOVEMENT;
}

void updateState(Cow& cow) {
    cow.updateState(classifyMotion(cow.getAcceleration()));
}

void updateGpsAdqTime(GpsRate gpsRate) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != NULL) {
        EmbeddedMessage_CreateWithPayload(msg, MSG_ID_GPS_REQUEST_CONFIG, MODULE_FSM, 
                                         MODULE_GPS, (uint8_t*)&gpsRate, sizeof(GpsRate));
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
        RTOS_LOG_DEBUG("[FSM] GPS rate updated: %d\n", (int)gpsRate);
    }
}

void enterLowPowerSleep() {
    // __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    // HAL_SuspendTick();
    // HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    // HAL_ResumeTick();
    RTOS_LOG_DEBUG("[FSM] Woke from sleep\n");
}

HAL_StatusTypeDef gpsResponse(EmbeddedMessage_t *msgReceived) {
    HAL_StatusTypeDef status = HAL_ERROR;
    
    if (msgReceived->id == MSG_ID_GPS_CONFIG_RESPONSE) {
        RTOS_LOG_DEBUG("[FSM] GPS config confirmed\n");
        status = HAL_OK;
    }
    
    MessagePool_Free(msgReceived);
    return status;
}

void sendZoneToStimulus(zone_t zone, ModuleId_t dest) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != NULL) {
        EmbeddedMessage_CreateWithPayload(msg, MSG_ID_ZONE_CHANGE, MODULE_FSM, dest,
                                         (uint8_t*)&zone, sizeof(zone_t));
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
        RTOS_LOG_DEBUG("[FSM] Sent zone %d to STIMULUS\n", zone);
    }
}

HAL_StatusTypeDef receivedStimulusResponse(EmbeddedMessage_t *msgReceived) {
    HAL_StatusTypeDef status = HAL_ERROR;
    
    if (msgReceived->id == MSG_ID_STIMULUS_FEEDBACK) {
        RTOS_LOG_DEBUG("[FSM] Stimulus feedback received\n");
        status = HAL_OK;
    }
    
    MessagePool_Free(msgReceived);
    return status;
}