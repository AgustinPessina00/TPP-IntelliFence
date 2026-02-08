#include "I2CIntegration.h"
#include "I2CManager.h"
#include <stdio.h>

// ========== IMPLEMENTACIÓN DE FUNCIONES DE INTEGRACIÓN ==========

int initializeI2cArchitecture(void) {
    return i2cSystemInit();
}

void startI2cDemonstrations(void) {
    // Esta función está definida en I2CStaticExample.cpp
    extern void startI2cStaticExamples(void);
    startI2cStaticExamples();
}

int checkI2cSystemHealth(void) {
    return i2cSystemIsReady();
}

int simpleI2cReadByte(uint8_t deviceAddr, uint8_t regAddr, uint8_t* value) {
    if (value == NULL) {
        return 0;
    }
    
    return (i2cReadRegister(deviceAddr, regAddr, value, 1) == 0) ? 1 : 0;
}

int simpleI2cWriteByte(uint8_t deviceAddr, uint8_t regAddr, uint8_t value) {
    return (i2cWriteRegister(deviceAddr, regAddr, &value, 1) == 0) ? 1 : 0;
}

int simpleI2cReadBytes(uint8_t deviceAddr, uint8_t regAddr, 
                      uint8_t* buffer, uint8_t length) {
    if (buffer == NULL || length == 0) {
        return 0;
    }
    
    return (i2cReadRegister(deviceAddr, regAddr, buffer, length) == 0) ? 1 : 0;
}

void getI2cStatistics(uint32_t* totalOps, uint32_t* successfulOps, 
                     uint32_t* errorCount) {
    if (totalOps && successfulOps && errorCount) {
        // Esta función requiere acceso a I2CManager desde C++
        // Implementación en I2CManager.cpp
        extern void getI2cStatsCWrapper(uint32_t* total, uint32_t* success, uint32_t* errors);
        getI2cStatsCWrapper(totalOps, successfulOps, errorCount);
    }
}

void printI2cStatistics(void) {
    uint32_t total, success, errors;
    getI2cStatistics(&total, &success, &errors);
    
    printf("\n=== Estadísticas I2C ===\r\n");
    printf("Total operaciones: %u\r\n", (unsigned int)total);
    printf("Operaciones exitosas: %u\r\n", (unsigned int)success);
    printf("Errores: %u\r\n", (unsigned int)errors);
    
    if (total > 0) {
        float successRate = (float)success / total * 100.0f;
        printf("Tasa de éxito: %.1f%%\r\n", successRate);
    } else {
        printf("No hay operaciones registradas\r\n");
    }
}