#ifndef GPSTASK_H
#define GPSTASK_H

#ifdef __cplusplus
extern "C" {
#endif

void gpsTask(void *argument);

#ifdef __cplusplus
}
#endif

// I2C Address for SAM-M10Q GPS
const int gpsAddress = 0x42;

#endif // GPSTASK_H
