#ifndef STIMULUSTASK_H
#define STIMULUSTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "cow.h"
#include "fence.h"

void stimulusTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // STIMULUSTASK_H
