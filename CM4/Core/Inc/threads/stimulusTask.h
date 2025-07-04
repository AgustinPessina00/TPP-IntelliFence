#ifndef STIMULUSTASK_H
#define STIMULUSTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "cow.h"
// #include "fence.h"

enum zone_t {
    GREEN_ZONE,
    BLUE_ZONE,
    YELLOW_ZONE,
    RED_ZONE,
    BLACK_ZONE
};

void stimulusTask(Cow *cow);

#ifdef __cplusplus
}
#endif

#endif // STIMULUSTASK_H
