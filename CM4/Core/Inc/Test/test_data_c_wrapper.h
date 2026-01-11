#ifndef TEST_DATA_C_WRAPPER_H
#define TEST_DATA_C_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Funciones para inicializar y controlar el modo TEST
void TestData_Init(void);
void TestMode_Enable(void);
void TestMode_Disable(void);
bool TestMode_IsEnabled(void);
void TestMode_Reset(void);

// Constantes de test
#define TEST_GPS_DATA_COUNT 30
#define TEST_IMU_DATA_COUNT 30

#ifdef __cplusplus
}
#endif

#endif // TEST_DATA_C_WRAPPER_H
