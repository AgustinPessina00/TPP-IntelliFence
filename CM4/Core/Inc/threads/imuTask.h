#ifndef IMUTASK_H
#define IMUTASK_H

#ifdef __cplusplus
extern "C" {
#endif

void imuTask(void *argument);

#ifdef __cplusplus
}
#endif

// I2C Address for LSM6DSO IMU
const int imuAddress = 0x6A;

#endif // IMUTASK_H
