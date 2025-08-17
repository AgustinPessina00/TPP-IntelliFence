#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include "stm32wlxx_hal.h"   // O el HAL que corresponda a tu MCU

#ifdef __cplusplus
extern "C" {
#endif

uint32_t millis(void);

#ifdef __cplusplus
}
#endif

#endif
