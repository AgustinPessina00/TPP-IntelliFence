#ifndef SENSORACQTASK_H
#define SENSORACQTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "sam_m10q.h"
#include "lsm6dso.h"
#include "ina226.h"

#include "messages.h"

#define SAMPLE_RATE  100  // en milisegundos

struct typedef
{
  SamM10q *gps;
  Lsm6dso *imu;
  Ina226 *inaMcu;
  Ina226 *inaGps;
  Ina226 *inaImu;
}sensorAcqTaskParams;

int error_count;

void sensorAcqTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // SENSORACQTASK_H
