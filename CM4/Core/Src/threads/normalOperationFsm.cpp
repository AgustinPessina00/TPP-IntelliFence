#include "normalOperationFsm.h"
#include "threads/fsmTask.h"
#include "threads/fsm_helper.h"
#include "zone.h"
#include "getZone.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <math.h>

// ============================================================================
// NORMAL OPERATION FSM
// ============================================================================

void runNormalOperationFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState,
                          GreenZoneState_t& greenZoneState, StimulusZone_t& stimulusZoneState,
                          TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg) {
    
    if (msg != nullptr && *msg != nullptr)
        RTOS_LOG_INFO("[NORMAL_OPERATION] Received message - ID: %d, Sender: %d\r\n", (*msg)->id, (*msg)->sender);
    
    switch (normalOpFSM) {
        case NormalOpFSM_t::INITIALIZE:
            runInitializeFSM(normalOpFSM, initializeState, timeout, cow, fence, msg);
            break;
            
        case NormalOpFSM_t::GREEN_ZONE:
            runGreenZoneFSM(normalOpFSM, greenZoneState, timeout, cow, fence, msg);
            break;
            
        case NormalOpFSM_t::STIMULUS_ZONE:
            runStimulusZoneFSM(normalOpFSM, stimulusZoneState, timeout, cow, fence, msg);
            break;
    }
}

// ============================================================================
// INITIALIZE SUB-FSM
// ============================================================================

void runInitializeFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState,
                     TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg) {
    // newMessage ya está disponible desde runNormalOperationFSM
    static bool newMessage = false;
    
    if (msg != nullptr && *msg != nullptr) {
        newMessage = true;
    } else {
        newMessage = false;
    }
    
    switch (initializeState) {
        case INITIALIZE_BEGIN:
            initializeState = INITIALIZE_REQUEST_POSITION;
            break;
            
        case INITIALIZE_REQUEST_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_SENSOR_ACQ);
            Timeout_Start(&timeout, GPS_TIMEOUT_MS);
            initializeState = INITIALIZE_WAIT_POSITION;
            break;
            
        case INITIALIZE_WAIT_POSITION:
            if (waitForMessage(MSG_ID_SEND_GPS, timeout, msg, newMessage) == HAL_OK) {
                bool validPosition = false;
                if (processGpsMessage(*msg, cow, validPosition) == HAL_OK && validPosition) {
                    initializeState = INITIALIZE_REQUEST_ZONE;
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] INIT: GPS timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                initializeState = INITIALIZE_REQUEST_POSITION;
            }
            break;
            
        case INITIALIZE_REQUEST_ZONE:
            // Calcular zona localmente
            {
                float minDistance = 0.0f;
                zone_t calculatedZone = getZoneFromDistance(&cow, &fence, minDistance);
                
                cow.updateCurrentZone(calculatedZone);
                cow.updateDistanceToLimit(minDistance);
                
                RTOS_LOG_DEBUG("[FSM] INIT: Zone calculated: %d, Distance: %.2fm\r\n", calculatedZone, minDistance);
                initializeState = INITIALIZE_END;
            }
            break;
            
        case INITIALIZE_EVALUATE_ZONE:
            // Ya no se usa - eliminado
            initializeState = INITIALIZE_END;
            break;
            
        case INITIALIZE_END:
            initializeState = INITIALIZE_BEGIN;
            if (isInGreenZone(cow) == HAL_OK) {
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
                    TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg) {
    static bool newMessage = false;
    
    if (msg != nullptr && *msg != nullptr) {
        newMessage = true;
    } else {
        newMessage = false;
    }
    
    switch (greenZoneState) {
        case GREEN_ZONE_BEGIN:
            greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
            break;
            
        case GREEN_ZONE_REQUEST_ACCELERATION:
            sendMessage(MSG_ID_REQUEST_IMU, MODULE_SENSOR_ACQ);
            Timeout_Start(&timeout, IMU_TIMEOUT_MS);
            greenZoneState = GREEN_ZONE_WAIT_ACCELERATION;
            break;
            
        case GREEN_ZONE_WAIT_ACCELERATION:
            if (waitForMessage(MSG_ID_SEND_IMU, timeout, msg, newMessage) == HAL_OK) {
                if (processImuMessage(*msg, cow) == HAL_OK) {
                    greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] GREEN: IMU timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
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
            Timeout_Start(&timeout, GPS_CONFIG_TIMEOUT_MS);
            greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
            break;
            
        case GREEN_ZONE_SLEEP:
            updateGpsAdqTime(GpsRate::STOP);
            Timeout_Start(&timeout, GPS_CONFIG_TIMEOUT_MS);
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
            Timeout_Start(&timeout, GPS_CONFIG_TIMEOUT_MS);
            greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
            break;
            
        case GREEN_ZONE_FAR_LIMIT:
            updateGpsAdqTime(GpsRate::MEDIUM);
            Timeout_Start(&timeout, GPS_CONFIG_TIMEOUT_MS);
            greenZoneState = GREEN_ZONE_WAIT_GPS_ADQ_TIME;
            break;
            
        case GREEN_ZONE_WAIT_GPS_ADQ_TIME:
            if (waitForMessage(MSG_ID_GPS_CONFIG_RESPONSE, timeout, msg, newMessage) == HAL_OK) {
                if (processGpsConfigResponse(*msg) == HAL_OK) {
                    greenZoneState = GREEN_ZONE_END;
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] GREEN: GPS config timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
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
                       TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg) {
    static bool newMessage = false;
    
    if (msg != nullptr && *msg != nullptr) {
        newMessage = true;
    } else {
        newMessage = false;
    }
    
    switch (stimulusZoneState) {
        case STIMULUS_ZONE_BEGIN:
            stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
            break;
            
        case STIMULUS_ZONE_SEND_ZONE:
            sendZoneToStimulus(cow.getCurrentZone(), MODULE_STIMULUS);
            Timeout_Start(&timeout, STIMULUS_TIMEOUT_MS);
            stimulusZoneState = STIMULUS_ZONE_WAIT_RESPONSE;
            break;
            
        case STIMULUS_ZONE_WAIT_RESPONSE:
            if (waitForMessage(MSG_ID_STIMULUS_FEEDBACK, timeout, msg, newMessage) == HAL_OK) {
                if (processStimulusResponse(*msg) == HAL_OK) {
                    stimulusZoneState = STIMULUS_ZONE_END;
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] STIMULUS: Response timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                stimulusZoneState = STIMULUS_ZONE_SEND_ZONE;
            }
            break;
            
        case STIMULUS_ZONE_END:
            stimulusZoneState = STIMULUS_ZONE_BEGIN;
            normalOpFSM = NormalOpFSM_t::INITIALIZE;
            break;
    }
}
