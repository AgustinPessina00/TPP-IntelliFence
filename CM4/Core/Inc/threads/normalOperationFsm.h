#ifndef NORMAL_OPERATION_FSM_H
#define NORMAL_OPERATION_FSM_H

#include "fsmTask.h"
#include "cow.h"
#include "fence.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus

void runNormalOperationFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState,
                          GreenZoneState_t& greenZoneState, StimulusZone_t& stimulusZoneState,
                          TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runInitializeFSM(NormalOpFSM_t& normalOpFSM, InitializeState_t& initializeState,
                     TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runGreenZoneFSM(NormalOpFSM_t& normalOpFSM, GreenZoneState_t& greenZoneState,
                    TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

void runStimulusZoneFSM(NormalOpFSM_t& normalOpFSM, StimulusZone_t& stimulusZoneState,
                       TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif // NORMAL_OPERATION_FSM_H
