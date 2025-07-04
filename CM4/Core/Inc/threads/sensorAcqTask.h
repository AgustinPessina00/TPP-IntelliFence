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
#define GPS_SAMPLE_RATE  (60 * 1000)  // en milisegundos (ej: 60s)

typedef struct {
  float ax; // aceleración eje X
  float ay; // aceleración eje Y
  float az; // aceleración eje Z
} imu_data_t;

typedef enum {
  GRAZING,
  SLEEP,
  MOVEMENT
} state_t;

typedef float distance_t;

typedef enum {
  VERY_SLOW,
  SLOW,
  MEDIUM,
  FAST
} gps_rate_t;

void enterLowPowerSleep(void);
state_t classifyMotion(imu_data_t imu);

void sensorAcqTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // SENSORTASK_H
