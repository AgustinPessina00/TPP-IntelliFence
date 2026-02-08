#ifndef FENCE_TRANSITION_FSM_H
#define FENCE_TRANSITION_FSM_H

#include "fsmTask.h"
#include "cow.h"
#include "fence.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus

void runFenceTransitionFSM(MainFSM_t& mainFSM, FenceTransitionState_t& fenceTransitionState,
                          TimeoutContext_t& timeout, Cow& cow, Fence& fence, EmbeddedMessage_t** msg);

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif // FENCE_TRANSITION_FSM_H
