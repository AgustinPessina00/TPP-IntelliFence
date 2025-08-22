#ifndef GPSTASK_H
#define GPSTASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "messages.h"
#include "GPS/sam_m10q.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  SamM10q *gps;
}gpsAcqTaskParams;

void gpsTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // GPSTASK_H
