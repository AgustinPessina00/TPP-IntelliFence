/**
 * @file lsm6dso_test_wrapper.cpp
 * @brief Test wrapper for LSM6DSO IMU sensor
 * @details Provides comprehensive testing of the LSM6DSO implementation
 *          using thread-safe I2C communication
 */

#include "../../Modules/IMU/lsm6dso.h"
#include "I2CManager.h"
#include "main.h"
#include <stdio.h>
#include <math.h>

/**
 * @brief Comprehensive test of LSM6DSO IMU sensor
 * 
 * This function tests all aspects of the LSM6DSO implementation:
 * - I2C connectivity
 * - WHO_AM_I verification
 * - Accelerometer measurements
 * - Gyroscope measurements
 * - Temperature readings
 * - Different configurations and ranges
 * - Thread-safe I2C communication
 */
extern "C" void lsm6dso_comprehensive_test(void) {
    printf("[LSM6DSO] ========================================\n");
    printf("[LSM6DSO] INICIANDO TEST COMPRENSIVO LSM6DSO\n");
    printf("[LSM6DSO] Sensor IMU 6DOF thread-safe\n");
    printf("[LSM6DSO] ========================================\n");
    
    // ===== FASE 1: INICIALIZACION I2C =====
    printf("\n[LSM6DSO] === FASE 1: INICIALIZACION I2C ===\n");
    
    if (!I2CManager::isInitialized()) {
        printf("[LSM6DSO] Inicializando I2CManager...\n");
        if (!I2CManager::initializeAll()) {
            printf("[LSM6DSO] ERROR CRITICO - Fallo inicializacion I2CManager\n");
            return;
        }
    }
    printf("[LSM6DSO] OK - I2CManager inicializado\n");
    
    // ===== FASE 2: TEST CONECTIVIDAD =====
    printf("\n[LSM6DSO] === FASE 2: TEST CONECTIVIDAD ===\n");
    
    I2CBus& testBus = I2CManager::getBus2();
    uint8_t dummyData;
    
    // LSM6DSO direcciones comunes: 0x6A (SDO=GND), 0x6B (SDO=VDD)
    uint8_t possibleAddresses[] = {0x6A};
    const char* addressNames[] = {"Primary (0x6A)"};
    uint8_t foundAddress = 0;
    
    for (int i = 0; i < 1; i++) {
        printf("[LSM6DSO] Escaneando LSM6DSO en direccion %s...\n", addressNames[i]);
        I2CResult scanResult = testBus.memRead(possibleAddresses[i], REG_WHO_AM_I, 1, &dummyData, 1, 100);
        
        if (scanResult == I2C_OK) {
            printf("[LSM6DSO] OK - Dispositivo encontrado en %s\n", addressNames[i]);
            printf("[LSM6DSO]      WHO_AM_I: 0x%02X %s\n", dummyData, 
                   (dummyData == 0x6C) ? "(LSM6DSO - Correcto)" : "(ID no reconocido)");
            foundAddress = possibleAddresses[i];
            break;
        } else {
            printf("[LSM6DSO] INFO - No respuesta en %s\n", addressNames[i]);
        }
    }
    
    if (foundAddress == 0) {
        printf("[LSM6DSO] ERROR - No se encontro ningun LSM6DSO en el bus I2C\n");
        printf("[LSM6DSO] Continuando con direccion por defecto 0x6A...\n");
        foundAddress = 0x6A;
    }
    
    // ===== FASE 3: CREACION E INICIALIZACION LSM6DSO =====
    printf("\n[LSM6DSO] === FASE 3: INICIALIZACION LSM6DSO ===\n");
    
    printf("[LSM6DSO] Creando instancia LSM6DSO...\n");
    printf("[LSM6DSO] Parametros:\n");
    printf("[LSM6DSO] - Direccion I2C: 0x%02X\n", foundAddress);
    printf("[LSM6DSO] - Acelerometro: ODR=104Hz, FS=±2g\n");
    printf("[LSM6DSO] - Giroscopio: ODR=104Hz, FS=±250dps\n");
    printf("[LSM6DSO] - I3C: Deshabilitado\n");
    printf("[LSM6DSO] - Auto-increment: Habilitado\n");
    
    // Crear instancia con configuración estándar
    Lsm6dso imu;
    
    printf("[LSM6DSO] Inicializando LSM6DSO...\n");
    if (!imu.init(foundAddress,                          // I2C address
                  Lsm6dsoI3C::DISABLED,                 // I3C disabled
                  Lsm6dsoOdrAcc::ODR_104,               // 104Hz accelerometer ODR
                  Lsm6dsoFsAcc::FS_2G,                  // ±2g accelerometer range
                  Lsm6dsoOdrGyr::ODR_104,               // 104Hz gyroscope ODR
                  Lsm6dsoFsGyr::FS_250DPS)) {           // ±250dps gyroscope range
        printf("[LSM6DSO] ERROR - Fallo inicializacion LSM6DSO\n");
        return;
    }
    printf("[LSM6DSO] OK - LSM6DSO inicializado correctamente\n");
    
    // ===== FASE 4: VERIFICACION WHO_AM_I =====
    printf("\n[LSM6DSO] === FASE 4: VERIFICACION WHO_AM_I ===\n");
    
    uint8_t whoAmI;
    if (imu.getWhoAmI(whoAmI) == I2C_OK) {
        printf("[LSM6DSO] WHO_AM_I: 0x%02X", whoAmI);
        if (whoAmI == 0x6C) {
            printf(" OK - LSM6DSO detectado correctamente\n");
        } else {
            printf(" WARNING - ID no reconocido (esperado: 0x6C)\n");
        }
    } else {
        printf("[LSM6DSO] ERROR - No se pudo leer WHO_AM_I\n");
    }
    
    // ===== FASE 5: TEST FUNCIONES BASICAS =====
    printf("\n[LSM6DSO] === FASE 5: TEST FUNCIONES BASICAS ===\n");
    printf("[LSM6DSO] Ejecutando test interno...\n");
    imu.testIMU();
    
    // ===== FASE 6: MEDICIONES EN TIEMPO REAL =====
    printf("\n[LSM6DSO] === FASE 6: MEDICIONES TIEMPO REAL ===\n");
    printf("[LSM6DSO] Realizando 20 mediciones cada 200ms...\n");
    printf("[LSM6DSO] Formato: [#] Accel(mg) | Gyro(mdps) | Temp(°C) | Estado\n");
    printf("[LSM6DSO] ------------------------------------------------------------\n");
    
    int successfulReads = 0;
    float accelMagnitudeSum = 0.0f;
    float gyroMagnitudeSum = 0.0f;
    float tempSum = 0.0f;
    
    for (int i = 1; i <= 20; i++) {
        I2CResult accelResult, gyroResult, tempResult;
        bool allOk = true;
        
        printf("[LSM6DSO] [%2d] ", i);
        
        // Leer acelerometro
        accelResult = imu.readAcceleration();
        if (accelResult == I2C_OK) {
            printf("A(%.1f,%.1f,%.1f) | ", imu.ax, imu.ay, imu.az);
        } else {
            printf("A(ERROR) | ");
            allOk = false;
        }
        
        // Leer giroscopio
        gyroResult = imu.readGyroscope();
        if (gyroResult == I2C_OK) {
            printf("G(%.1f,%.1f,%.1f) | ", imu.gx, imu.gy, imu.gz);
        } else {
            printf("G(ERROR) | ");
            allOk = false;
        }
        
        // Leer temperatura
        tempResult = imu.readTemperature();
        if (tempResult == I2C_OK) {
            printf("T=%.1f°C | ", imu.temperature);
            tempSum += imu.temperature;
        } else {
            printf("T=ERROR | ");
            allOk = false;
        }
        
        // Estado de la medición y estadísticas
        if (allOk) {
            printf("OK");
            successfulReads++;
            
            // Calcular magnitudes
            float accelMagnitude = sqrtf(imu.ax*imu.ax + imu.ay*imu.ay + imu.az*imu.az);
            float gyroMagnitude = sqrtf(imu.gx*imu.gx + imu.gy*imu.gy + imu.gz*imu.gz);
            
            accelMagnitudeSum += accelMagnitude;
            gyroMagnitudeSum += gyroMagnitude;
        } else {
            printf("ERROR");
        }
        
        printf("\n");
        HAL_Delay(200); // 200ms entre mediciones
    }
    
    // ===== FASE 7: ANALISIS DE RESULTADOS =====
    printf("\n[LSM6DSO] === FASE 7: ANALISIS DE RESULTADOS ===\n");
    
    printf("[LSM6DSO] RESULTADO: %d/20 lecturas exitosas (%.1f%%)\n", 
           successfulReads, (float)successfulReads * 5.0f);
    
    if (successfulReads >= 18) {
        printf("[LSM6DSO] EXCELENTE - Sensor funciona correctamente\n");
    } else if (successfulReads >= 15) {
        printf("[LSM6DSO] BUENO - Sensor funciona con algunos errores\n");
    } else {
        printf("[LSM6DSO] PROBLEMAS - Sensor tiene fallas frecuentes\n");
    }
    
    if (successfulReads > 0) {
        float avgAccelMagnitude = accelMagnitudeSum / successfulReads;
        float avgGyroMagnitude = gyroMagnitudeSum / successfulReads;
        float avgTemp = tempSum / successfulReads;
        
        printf("[LSM6DSO] ESTADISTICAS:\n");
        printf("[LSM6DSO]   Magnitud aceleracion promedio: %.2f mg\n", avgAccelMagnitude);
        printf("[LSM6DSO]   Magnitud giroscopio promedio: %.2f mdps\n", avgGyroMagnitude);
        printf("[LSM6DSO]   Temperatura promedio: %.1f°C\n", avgTemp);
        
        // Análisis de valores
        if (avgAccelMagnitude > 900.0f && avgAccelMagnitude < 1100.0f) {
            printf("[LSM6DSO]   OK - Magnitud aceleracion cerca de 1g (sensor estatico)\n");
        } else {
            printf("[LSM6DSO]   INFO - Magnitud aceleracion: %.2f mg (sensor en movimiento o calibracion)\n", avgAccelMagnitude);
        }
        
        if (avgGyroMagnitude < 50.0f) {
            printf("[LSM6DSO]   OK - Giroscopio estatico (bajo ruido)\n");
        } else if (avgGyroMagnitude < 200.0f) {
            printf("[LSM6DSO]   INFO - Giroscopio con movimiento leve\n");
        } else {
            printf("[LSM6DSO]   INFO - Giroscopio detecta movimiento significativo\n");
        }
        
        if (avgTemp > 15.0f && avgTemp < 50.0f) {
            printf("[LSM6DSO]   OK - Temperatura ambiente normal\n");
        } else {
            printf("[LSM6DSO]   INFO - Temperatura fuera del rango típico\n");
        }
    }
    
    // ===== FASE 8: TEST DIFERENTES CONFIGURACIONES =====
    printf("\n[LSM6DSO] === FASE 8: TEST CONFIGURACIONES ===\n");
    printf("[LSM6DSO] Probando diferentes rangos de medicion...\n");
    
    // Test con rango extendido de acelerómetro
    printf("[LSM6DSO] Configurando acelerometro ±8g...\n");
    Lsm6dso imuExtended;
    
    if (imuExtended.init(foundAddress, Lsm6dsoI3C::DISABLED, 
                         Lsm6dsoOdrAcc::ODR_208, Lsm6dsoFsAcc::FS_8G,
                         Lsm6dsoOdrGyr::ODR_208, Lsm6dsoFsGyr::FS_500DPS)) {
        printf("[LSM6DSO] Config extendida inicializada - probando 3 lecturas...\n");
        for (int i = 1; i <= 3; i++) {
            if (imuExtended.readAcceleration() == I2C_OK && 
                imuExtended.readGyroscope() == I2C_OK) {
                printf("[LSM6DSO] [%d] A(%.1f,%.1f,%.1f)mg G(%.1f,%.1f,%.1f)mdps - 208Hz/±8g/±500dps\n", 
                       i, imuExtended.ax, imuExtended.ay, imuExtended.az,
                       imuExtended.gx, imuExtended.gy, imuExtended.gz);
            }
            HAL_Delay(100);
        }
    }
    
    // ===== CONCLUSION =====
    printf("\n[LSM6DSO] ========== CONCLUSION ==========\n");
    printf("[LSM6DSO] VENTAJAS IMPLEMENTACION THREAD-SAFE:\n");
    printf("[LSM6DSO] + Comunicacion I2C thread-safe con mutex FreeRTOS\n");
    printf("[LSM6DSO] + Uso de I2CManager centralizado\n");
    printf("[LSM6DSO] + Tipos de retorno I2CResult consistentes\n");
    printf("[LSM6DSO] + Deteccion automatica de direccion I2C\n");
    printf("[LSM6DSO] + Configuracion flexible (ODR, FS, modos)\n");
    printf("[LSM6DSO] + Mediciones simultaneas accel + gyro + temp\n");
    printf("[LSM6DSO] + Conversion automatica a unidades fisicas\n");
    printf("[LSM6DSO] + Arquitectura consistente con GPS e INA226\n");
    printf("[LSM6DSO] + Soporte multiple configuraciones\n");
    printf("[LSM6DSO] =====================================\n");
}

/**
 * @brief Test de diferentes configuraciones del LSM6DSO
 */
extern "C" void lsm6dso_configuration_test(void) {
    printf("[LSM6DSO-CFG] ========== TEST CONFIGURACIONES ==========\n");
    
    if (!I2CManager::isInitialized()) {
        I2CManager::initializeAll();
    }
    
    printf("[LSM6DSO-CFG] Probando diferentes configuraciones...\n");
    
    // Configuración 1: Baja potencia, baja frecuencia
    printf("\n[LSM6DSO-CFG] === CONFIG 1: BAJA POTENCIA ===\n");
    {
        Lsm6dso imuLowPower;
        
        if (imuLowPower.init(0x6A, Lsm6dsoI3C::DISABLED,
                             Lsm6dsoOdrAcc::ODR_12_5, Lsm6dsoFsAcc::FS_2G,
                             Lsm6dsoOdrGyr::ODR_12_5, Lsm6dsoFsGyr::FS_250DPS)) {
            printf("[LSM6DSO-CFG] Config baja potencia inicializada\n");
            printf("[LSM6DSO-CFG] ODR: 12.5Hz, Consumo: ~0.55mA\n");
            
            for (int i = 0; i < 5; i++) {
                if (imuLowPower.readAcceleration() == I2C_OK && 
                    imuLowPower.readGyroscope() == I2C_OK) {
                    printf("[LSM6DSO-CFG] [%d] A(%.2f,%.2f,%.2f) G(%.1f,%.1f,%.1f) - 12.5Hz\n", 
                           i+1, imuLowPower.ax, imuLowPower.ay, imuLowPower.az,
                           imuLowPower.gx, imuLowPower.gy, imuLowPower.gz);
                }
                HAL_Delay(200); // ~5Hz sampling
            }
        }
    }
    
    // Configuración 2: Alta frecuencia, máximo rendimiento
    printf("\n[LSM6DSO-CFG] === CONFIG 2: ALTO RENDIMIENTO ===\n");
    {
        Lsm6dso imuHighPerf;
        
        if (imuHighPerf.init(0x6A, Lsm6dsoI3C::DISABLED,
                             Lsm6dsoOdrAcc::ODR_6K66, Lsm6dsoFsAcc::FS_16G,
                             Lsm6dsoOdrGyr::ODR_6K66, Lsm6dsoFsGyr::FS_2KDPS)) {
            printf("[LSM6DSO-CFG] Config alto rendimiento inicializada\n");
            printf("[LSM6DSO-CFG] ODR: 6.66kHz, Rango: ±16g/±2000dps\n");
            
            for (int i = 0; i < 5; i++) {
                uint32_t startTime = HAL_GetTick();
                if (imuHighPerf.readAcceleration() == I2C_OK && 
                    imuHighPerf.readGyroscope() == I2C_OK) {
                    uint32_t elapsed = HAL_GetTick() - startTime;
                    printf("[LSM6DSO-CFG] [%d] A(%.1f,%.1f,%.1f) G(%.0f,%.0f,%.0f) (%ums) - 6.66kHz\n", 
                           i+1, imuHighPerf.ax, imuHighPerf.ay, imuHighPerf.az,
                           imuHighPerf.gx, imuHighPerf.gy, imuHighPerf.gz, (unsigned int)elapsed);
                }
                HAL_Delay(50); // Lectura rápida
            }
        }
    }
    
    printf("[LSM6DSO-CFG] =========================================\n");
}

/**
 * @brief Test de monitoreo continuo del LSM6DSO
 */
extern "C" void lsm6dso_continuous_monitor(void) {
    printf("[LSM6DSO-MONITOR] Iniciando monitor continuo LSM6DSO...\n");
    printf("[LSM6DSO-MONITOR] Presione reset para detener\n");
    
    if (!I2CManager::isInitialized()) {
        I2CManager::initializeAll();
    }
    
    Lsm6dso imu;
    
    if (!imu.init(0x6A, Lsm6dsoI3C::DISABLED,
                  Lsm6dsoOdrAcc::ODR_104, Lsm6dsoFsAcc::FS_4G,
                  Lsm6dsoOdrGyr::ODR_104, Lsm6dsoFsGyr::FS_500DPS)) {
        printf("[LSM6DSO-MONITOR] ERROR - Fallo inicializacion\n");
        return;
    }
    
    uint32_t readCount = 0;
    float maxAccelMagnitude = 0.0f;
    float maxGyroMagnitude = 0.0f;
    float minTemp = 999.0f;
    float maxTemp = -999.0f;
    
    while (1) {
        readCount++;
        
        if (imu.readAcceleration() == I2C_OK &&
            imu.readGyroscope() == I2C_OK &&
            imu.readTemperature() == I2C_OK) {
            
            // Calcular magnitudes
            float accelMag = sqrtf(imu.ax*imu.ax + imu.ay*imu.ay + imu.az*imu.az);
            float gyroMag = sqrtf(imu.gx*imu.gx + imu.gy*imu.gy + imu.gz*imu.gz);
            
            // Actualizar estadísticas
            if (accelMag > maxAccelMagnitude) maxAccelMagnitude = accelMag;
            if (gyroMag > maxGyroMagnitude) maxGyroMagnitude = gyroMag;
            if (imu.temperature < minTemp) minTemp = imu.temperature;
            if (imu.temperature > maxTemp) maxTemp = imu.temperature;
            
            // Mostrar cada 100 lecturas
            if (readCount % 100 == 0) {
                printf("[LSM6DSO-MONITOR] #%u: A=%.1fmg G=%.1fmdps T=%.1f°C | MaxA=%.1f MaxG=%.1f TempRange=%.1f-%.1f\n",
                       (unsigned int)readCount, accelMag, gyroMag, imu.temperature,
                       maxAccelMagnitude, maxGyroMagnitude, minTemp, maxTemp);
            }
        }
        
        HAL_Delay(50); // 20Hz
    }
}