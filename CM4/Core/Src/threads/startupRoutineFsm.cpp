#include "startupRoutineFsm.h"
#include "threads/fsmTask.h"
#include "threads/fsm_helper.h"
#include "zone.h"
#include "getZone.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"

// ============================================================================
// STARTUP ROUTINE FSM
// ============================================================================

void runStartupRoutineFSM(MainFSM_t& mainFSM, StartupRoutineState_t* state, 
                         TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg) {
    static bool newMessage = false;
    
    if (msg != nullptr && *msg != nullptr) {
        RTOS_LOG_INFO("[STARTUP] Received message - ID: %d, Sender: %d\r\n", (*msg)->id, (*msg)->sender);
        newMessage = true;
    } else {
        newMessage = false;
    }
    
    switch (*state) {
        case STARTUP_ROUTINE_BEGIN:
            RTOS_LOG_INFO("[FSM] Starting STARTUP_ROUTINE\r\n");
            *state = STARTUP_ROUTINE_WAIT_JOIN;
            Timeout_Start(&timeout, 5000);  // 5 segundos entre mensajes de espera
            break;
            
        case STARTUP_ROUTINE_WAIT_JOIN:
            if (waitForMessage(MSG_ID_LORA_JOINED, timeout, msg, newMessage) == HAL_OK) {
                RTOS_LOG_INFO("[FSM] LoRaWAN joined successfully\r\n");
                *state = STARTUP_ROUTINE_REQUEST_POSITION;
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_INFO("[FSM] Waiting for LoRaWAN join...\r\n");
                Timeout_Start(&timeout, 5000);  // Reiniciar timeout y seguir esperando
            }
            break;
            
        case STARTUP_ROUTINE_REQUEST_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_GPS);
            Timeout_Start(&timeout, GPS_TIMEOUT_MS);
            RTOS_LOG_DEBUG("[FSM] Requesting initial GPS position (timeout: %lums)\r\n", timeout.timeoutMs);
            *state = STARTUP_ROUTINE_WAIT_POSITION;
            break;
            
        case STARTUP_ROUTINE_WAIT_POSITION:
            if (waitForMessage(MSG_ID_SEND_GPS, timeout, msg, newMessage) == HAL_OK) {
                bool validPosition = false;
                if (processGpsMessage(*msg, cow, validPosition) == HAL_OK) {
                    if (validPosition) {
                        RTOS_LOG_DEBUG("[FSM] Valid GPS position received after %lums\r\n", Timeout_GetElapsed(&timeout));
                        *state = STARTUP_ROUTINE_REQUEST_GPS_CONFIGURATION_PSM;
                    } else {
                        RTOS_LOG_WARN("[FSM] GPS without fix, retrying...\r\n");
                        *state = STARTUP_ROUTINE_REQUEST_POSITION;
                    }
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] GPS timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                *state = STARTUP_ROUTINE_REQUEST_POSITION;
            }
            break;

        case STARTUP_ROUTINE_REQUEST_GPS_CONFIGURATION_PSM:
            sendMessage(MSG_ID_REQUEST_GPS_CONFIGURATION_PSM, MODULE_GPS);
            Timeout_Start(&timeout, GPS_PSM_TIMEOUT_MS);
            RTOS_LOG_DEBUG("[FSM] Requesting GPS configuration PSM (timeout: %lums)\r\n", timeout.timeoutMs);
            *state = STARTUP_ROUTINE_WAIT_GPS_CONFIGURATION_PSM;
            break;
            
        case STARTUP_ROUTINE_WAIT_GPS_CONFIGURATION_PSM:
            if (waitForMessage(MSG_ID_SEND_GPS_CONFIGURATION_PSM, timeout, msg, newMessage) == HAL_OK) {
                RTOS_LOG_DEBUG("[FSM] GPS configuration PSM received after %lums\r\n", Timeout_GetElapsed(&timeout));
                *state = STARTUP_ROUTINE_SEND_POSITION_LORA;
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] GPS configuration PSM timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                *state = STARTUP_ROUTINE_REQUEST_GPS_CONFIGURATION_PSM;
            }
            break;

        case STARTUP_ROUTINE_SEND_POSITION_LORA:
            sendPosition(MSG_ID_LORA_SEND_POSITION, MODULE_LORA_TX, cow);
            Timeout_Start(&timeout, LORA_TX_TIMEOUT_MS);
            RTOS_LOG_DEBUG("[FSM] Sending position via LoRa (timeout: %lums)\r\n", timeout.timeoutMs);
            *state = STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE;
            break;
            
        case STARTUP_ROUTINE_WAIT_SEND_POSITION_RESPONSE:
            if (waitForMessage(MSG_ID_LORA_SEND_POSITION_FEEDBACK, timeout, msg, newMessage) == HAL_OK) {
                if (processLoRaTxResponse(*msg) == HAL_OK) {
                    RTOS_LOG_DEBUG("[FSM] LoRa TX confirmed after %lums\r\n", Timeout_GetElapsed(&timeout));
                    *state = STARTUP_ROUTINE_WAIT_FENCE;
                    Timeout_Start(&timeout, LORA_RX_TIMEOUT_MS);
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] LoRa TX timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                *state = STARTUP_ROUTINE_SEND_POSITION_LORA;
            }
            break;
            
        case STARTUP_ROUTINE_WAIT_FENCE:
            if (waitForMessage(MSG_ID_LORA_VERTEXES_RECEIVED, timeout, msg, newMessage) == HAL_OK) {
                if (processFenceMessage(*msg, fence) == HAL_OK) {
                    RTOS_LOG_INFO("[FSM] Fence vertices received after %lums\r\n", Timeout_GetElapsed(&timeout));
                    *state = STARTUP_ROUTINE_SAVE_FENCE;
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] Fence RX timeout (%lums), waiting...\r\n", Timeout_GetElapsed(&timeout));
                // Re-iniciar timeout para seguir esperando
                Timeout_Start(&timeout, LORA_RX_TIMEOUT_MS);
            }
            break;
            
        case STARTUP_ROUTINE_SAVE_FENCE:
            // Fence ya fue actualizado en processFenceMessage() via createLimits()
            *state = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
            break;
            
        case STARTUP_ROUTINE_REQUEST_NEW_POSITION:
            sendMessage(MSG_ID_REQUEST_GPS, MODULE_GPS);
            Timeout_Start(&timeout, GPS_TIMEOUT_MS);
            RTOS_LOG_DEBUG("[FSM] Requesting GPS position after fence update\r\n");
            *state = STARTUP_ROUTINE_WAIT_NEW_POSITION;
            break;
            
        case STARTUP_ROUTINE_WAIT_NEW_POSITION:
            if (waitForMessage(MSG_ID_SEND_GPS, timeout, msg, newMessage) == HAL_OK) {
                bool validPosition = false;
                if (processGpsMessage(*msg, cow, validPosition) == HAL_OK) {
                    if (validPosition) {
                        RTOS_LOG_DEBUG("[FSM] Valid GPS position received after %lums\r\n", Timeout_GetElapsed(&timeout));
                        *state = STARTUP_ROUTINE_REQUEST_ZONE;
                    } else {
                        RTOS_LOG_WARN("[FSM] GPS without fix, retrying...\r\n");
                        *state = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
                    }
                }
            } else if (Timeout_IsExpired(&timeout)) {
                RTOS_LOG_WARN("[FSM] GPS timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
                *state = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
            }
            break;
            
        case STARTUP_ROUTINE_REQUEST_ZONE:
            // Calcular zona localmente
            {
                float minDistance = 0.0f;
                zone_t calculatedZone = getZoneFromDistance(&cow, &fence, minDistance);
                
                cow.updateCurrentZone(calculatedZone);
                cow.updateDistanceToLimit(minDistance);
                
                RTOS_LOG_DEBUG("[FSM] Zone calculated: %d, Distance: %.2fm\r\n", calculatedZone, minDistance);
                *state = STARTUP_ROUTINE_END;
            }
            break;
            
        case STARTUP_ROUTINE_END:
            *state = STARTUP_ROUTINE_BEGIN;
            if (isInGreenZone(cow) == HAL_OK) {
                mainFSM = MainFSM_t::NORMAL_OPERATION;
                RTOS_LOG_INFO("[FSM] Startup complete - entering NORMAL_OPERATION\r\n");
            } else {
                mainFSM = MainFSM_t::FENCE_TRANSITION;
                RTOS_LOG_INFO("[FSM] Startup complete - entering FENCE_TRANSITION\r\n");
            }
            break;
    }
}
