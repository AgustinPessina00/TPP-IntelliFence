#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "threads/fsmTask.h"
#include "threads/fsm_helper.h"
#include "threads/startupRoutineFsm.h"
#include "threads/normalOperationFsm.h"
#include "threads/fenceTransitionFsm.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "zone.h"
#include "getZone.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <string.h>
#include <math.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

// Variables globales para la FSM
bool receivedNewFence = false;


// Variables de estado de las FSMs (static para ahorrar stack)
static MainFSM_t s_mainFSM = MainFSM_t::STARTUP_ROUTINE;
static NormalOpFSM_t s_normalOpFSM = NormalOpFSM_t::INITIALIZE;
static StartupRoutineState_t s_startupRoutineState = STARTUP_ROUTINE_BEGIN;
static InitializeState_t s_initializeState = INITIALIZE_BEGIN;
static GreenZoneState_t s_greenZoneState = GREEN_ZONE_BEGIN;
static StimulusZone_t s_stimulusZoneState = STIMULUS_ZONE_BEGIN;
static FenceTransitionState_t s_fenceTransitionState = FENCE_TRANSITION_BEGIN;

// Variables globales Cow y Fence (static file-scope)
static Cow cow;
static Fence fence;

// ============================================================================
// MAIN FSM TASK
// ============================================================================

void fsmTask(void *argument) {
    (void)argument; // Unused parameter

    // Inicializar objetos Cow y Fence
    DeviceUID deviceUID = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    cow.init(deviceUID);
    fence.init();
    EmbeddedMessage_t* msg = nullptr;
    
    TimeoutContext_t timeout = {0, 0};
    
    RTOS_LOG_INFO("[FSM] Task initialized successfully\r\n");

    static uint32_t stackMonitorCounter = 0;
    while(1) {
        // Monitorear stack cada ~10 segundos (cada 20 iteraciones × 500ms delay)
        if (++stackMonitorCounter >= 10) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(nullptr);
            RTOS_LOG_INFO("[FSM] Stack libre: %u words (%u bytes)\r\n", 
                         stackLeft, stackLeft * 4);
            stackMonitorCounter = 0;
        }

        if(osMessageQueueGet(fsmQueueHandle, &msg, NULL, 0) == osOK) {
            RTOS_LOG_INFO("[FSM] Received message - ID: %d, Sender: %d, MainFSM: %d\r\n", 
                         msg->id, msg->sender, (int)s_mainFSM);
            if (s_mainFSM == MainFSM_t::NORMAL_OPERATION && fence.getHasValidFence() && msg->id == MSG_ID_LORA_VERTEXES_RECEIVED) {
                receivedNewFence = true;
            }
            
        }
        else {
            msg = nullptr; // No hay mensaje disponible
        }

        switch (s_mainFSM) {
            case MainFSM_t::STARTUP_ROUTINE:
                runStartupRoutineFSM(s_mainFSM, &s_startupRoutineState, timeout, cow, fence, &msg);
                break;
                
            case MainFSM_t::NORMAL_OPERATION:
                runNormalOperationFSM(s_normalOpFSM, s_initializeState, s_greenZoneState, 
                                     s_stimulusZoneState, timeout, cow, fence, &msg);
                if (receivedNewFence) {
                    s_mainFSM = MainFSM_t::FENCE_TRANSITION;
                    receivedNewFence = false;
                }
                break;
                
            case MainFSM_t::FENCE_TRANSITION:
                runFenceTransitionFSM(s_mainFSM, s_fenceTransitionState, timeout, cow, fence, &msg);
                break;
        }
        
        osDelay(1000);
    }
}

