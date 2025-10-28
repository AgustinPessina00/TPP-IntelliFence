#ifndef SENSORACQTASK_H
#define SENSORACQTASK_H

#include "sam_m10q.h"
#include "lsm6dso.h"
#include "ina226.h"
#include "messages.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLE_RATE  100  // en milisegundos

typedef struct {
  SamM10q *gps;
  Lsm6dso *imu;
  Ina226 *inaMcu;
  Ina226 *inaGps;
  Ina226 *inaImu;
}sensorAcqTaskParams;

void sensorAcqTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // SENSORACQTASK_H
