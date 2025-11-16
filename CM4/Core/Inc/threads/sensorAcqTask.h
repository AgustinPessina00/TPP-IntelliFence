
#ifndef SENSORACQTASK_NEW_H
#define SENSORACQTASK_NEW_H
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void sensorAcqTask(void *argument);

#ifdef __cplusplus
}
#endif

// Configuraciones específicas para cada dirección
struct InaConfig {
    uint8_t address;
    float rShunt;
    float currentLSB;
};

const InaConfig configs[3] = {
    {0x40, 0.75f,  0.1f / 32768.0f},      // GPS (0x40) - Alto consumo (0.75 ohm) - 0.1/2^15
    {0x41, 10.0f,  2.5e-6/10},            // IMU (0x41) - Baja corriente (10 ohm) - 0.00055/2^15  
    {0x45, 10.0f,  0.5f / 32768.0f}       // MCU (0x45) - Corriente media (10 ohm) - 0.5/2^15
};

const int numConfigs = 3;
const int gpsIndex = 0;
const int imuIndex = 1;
const int mcuIndex = 2;

const int gpsAddress = 0x42;
const int imuAddress = 0x6A;


#endif // SENSORACQTASK_NEW_H