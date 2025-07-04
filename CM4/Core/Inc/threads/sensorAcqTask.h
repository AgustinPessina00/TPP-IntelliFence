#ifndef SENSORTASK_H
#define SENSORTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "cow.h"
#include "fence.h"

#define NEAR_LIMIT  10.0f             // en metros
#define GPS_SAMPLE_RATE  (1 * 1000)  // en milisegundos (ej: 1s)

typedef float distance_t;

typedef enum {
  VERY_SLOW,
  SLOW,
  MEDIUM,
  FAST
} gps_rate_t;

void enterLowPowerSleep(void);
CowState classifyMotion(Acceleration imu);

distance_t calculateDistanceToLimit(cow.getPosition(), fence.getSegments());
zone_t getZoneForDistance(distance_t dist, Fence fence);

void sensorAcqTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // SENSORTASK_H
