#ifndef FSMTASK_H
#define FSMTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "messages.h"
#include "cow.h"
#include "fence.h"

#define NEAR_LIMIT  10.0f   // en metros

typedef float distance_t;

enum class GpsRate {
  VERY_SLOW,
  SLOW,
  MEDIUM,
  FAST
};

void fsmTask(void *argument);

void enterLowPowerSleep(void);
CowState classifyMotion(Acceleration imu);

distance_t calculateDistanceToLimit(cow.getPosition(), fence.getSegments());
zone_t getZoneForDistance(distance_t dist, Fence fence);

#ifdef __cplusplus
}
#endif

#endif // FSMTASK_H
