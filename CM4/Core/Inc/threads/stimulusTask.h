#ifndef STIMULUSTASK_H
#define STIMULUSTASK_H

#include "cow.h"
#include "fence.h"
#include "messages.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

void stimulusTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // STIMULUSTASK_H
