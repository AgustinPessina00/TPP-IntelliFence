#ifndef STARTUP_ROUTINE_FSM_H
#define STARTUP_ROUTINE_FSM_H

#include "fsmTask.h"
#include "cow.h"
#include "fence.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus

void runStartupRoutineFSM(MainFSM_t& mainFSM, StartupRoutineState_t* state, 
                         TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif // STARTUP_ROUTINE_FSM_H
