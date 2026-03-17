#include "fenceTransitionFsm.h"
#include "threads/fsmTask.h"
#include "threads/fsm_helper.h"
#include "zone.h"
#include "getZone.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"

// ============================================================================
// FENCE TRANSITION FSM
// ============================================================================

void runFenceTransitionFSM(MainFSM_t& mainFSM, FenceTransitionState_t& fenceTransitionState,
                          TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg) {
    static bool newMessage = false;
    
    if (msg != nullptr && *msg != nullptr) {
        RTOS_LOG_INFO("[FENCE_TRANSITION] Received message - ID: %d, Sender: %d\r\n", (*msg)->id, (*msg)->sender);
        newMessage = true;
    } else {
        newMessage = false;
    }
    
    switch (fenceTransitionState) {
        case FENCE_TRANSITION_BEGIN:
            RTOS_LOG_INFO("[FENCE_TRANS] Starting FENCE_TRANSITION\r\n");
            fenceTransitionState = FENCE_TRANSITION_DISABLE_STIMULUS;
            break;
            
        case FENCE_TRANSITION_DISABLE_STIMULUS:
            sendZoneToStimulus(BLACK_ZONE, MODULE_STIMULUS);
            Timeout_Start(&timeout, STIMULUS_TIMEOUT_MS);
            fenceTransitionState = FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE;
            break;
            
        case FENCE_TRANSITION_WAIT_STIMULUS_RESPONSE:
            if (waitForMessage(MSG_ID_STIMULUS_FEEDBACK, timeout, msg, newMessage) == HAL_OK) {
                if (processStimulusResponse(*msg) == HAL_OK) {
                    // Fence ya fue procesado antes de entrar a FENCE_TRANSITION
                    // Simplemente verificamos que esté válido
                    if (fence.getHasValidFence()) {
                        RTOS_LOG_INFO("[FENCE_TRANS] Fence already saved (%d limits)\r\n", fence.getLimitCount());
                        fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
                    } else {
                        RTOS_LOG_ERROR("[FENCE_TRANS] No valid fence found!\r\n");
                        // Volver a NORMAL_OPERATION si no hay fence válido
                        fenceTransitionState = FENCE_TRANSITION_END;
                    }
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FENCE_TRANS] Stimulus timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                fenceTransitionState = FENCE_TRANSITION_DISABLE_STIMULUS;
            }
            break;
            
        case FENCE_TRANSITION_GPSRATE_FAST:
            // updateGpsAdqTime(GpsRate::FAST);
            // Timeout_Start(&timeout, GPS_CONFIG_TIMEOUT_MS);
            fenceTransitionState = FENCE_TRANSITION_WAIT_GPS_ADQ_TIME;
            break;
            
        case FENCE_TRANSITION_WAIT_GPS_ADQ_TIME:
            // if (waitForMessage(MSG_ID_GPS_CONFIG_RESPONSE, timeout, msg, newMessage) == HAL_OK) {
            //     if (processGpsConfigResponse(*msg) == HAL_OK) {
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
            //     }
            // } else if (Timeout_IsExpired(&timeout)) {
            //     RTOS_LOG_WARN("[FSM] FENCE_TRANS: GPS config timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
            //     fenceTransitionState = FENCE_TRANSITION_GPSRATE_FAST;
            // }
            break;
            
        case FENCE_TRANSITION_REQUEST_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_GPS);
            Timeout_Start(&timeout, GPS_TIMEOUT_MS);
            fenceTransitionState = FENCE_TRANSITION_WAIT_POSITION;
            break;
            
        case FENCE_TRANSITION_WAIT_POSITION:
            if (waitForMessage(MSG_ID_SEND_GPS, timeout, msg, newMessage) == HAL_OK) {
                bool validPosition = false;
                if (processGpsMessage(*msg, cow, validPosition) == HAL_OK && validPosition) {
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_ZONE;
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FENCE_TRANS] GPS timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
            }
            break;
            
        case FENCE_TRANSITION_REQUEST_ZONE:
            // Calcular zona localmente
            {
                float minDistance = 0.0f;
                zone_t calculatedZone = getZoneFromDistance(&cow, &fence, minDistance);
                
                cow.updateCurrentZone(calculatedZone);
                cow.updateDistanceToLimit(minDistance);
                
                RTOS_LOG_DEBUG("[FENCE_TRANS] Zone calculated: %d, Distance: %.2fm\r\n", calculatedZone, minDistance);

                if (calculatedZone != GREEN_ZONE) {
                    RTOS_LOG_INFO("[FENCE_TRANS] Zone is not green, please move the cow to a safe area\r\n");
                    fenceTransitionState = FENCE_TRANSITION_REQUEST_POSITION;
                }
                else {
                    fenceTransitionState = FENCE_TRANSITION_END;
                }
                
            }
            break;
            
        case FENCE_TRANSITION_END:
            fenceTransitionState = FENCE_TRANSITION_BEGIN;
            mainFSM = MainFSM_t::NORMAL_OPERATION;
            RTOS_LOG_INFO("[FENCE_TRANS] Fence transition complete\r\n");
            break;

    }
}
