/**
 * @file ina226_test_wrapper.cpp
 * @brief Test wrapper for INA226 current/power sensor
 * @details Provides comprehensive testing of the INA226 implementation
 *          using thread-safe I2C communication
 */

#include "../../Modules/INA/ina226.h"
#include "I2CManager.h"
#include "main.h"
#include <stdio.h>

/**
 * @brief Comprehensive test of INA226 current/power sensor
 * 
 * This function tests all aspects of the INA226 implementation:
 * - I2C connectivity
 * - Register configuration
 * - Current, voltage and power measurements
 * - Different averaging and conversion time settings
 * - Thread-safe I2C communication
 */
extern "C" void ina226_comprehensive_test(void) {
    printf("[INA226] ========================================\n");
    printf("[INA226] INICIANDO TEST COMPRENSIVO INA226\n");
    printf("[INA226] Sensor de corriente y potencia thread-safe\n");
    printf("[INA226] PROBANDO DIRECCIONES: 0x40, 0x41, 0x45\n");
    printf("[INA226] ========================================\n");
    
    // ===== FASE 1: INICIALIZACION I2C =====
    printf("\n[INA226] === FASE 1: INICIALIZACION I2C ===\n");
    
    if (!I2CManager::isInitialized()) {
        printf("[INA226] Inicializando I2CManager...\n");
        if (!I2CManager::initializeAll()) {
            printf("[INA226] ERROR CRITICO - Fallo inicializacion I2CManager\n");
            return;
        }
    }
    printf("[INA226] OK - I2CManager inicializado\n");
    
    // ===== FASE 2: TEST CONECTIVIDAD =====
    printf("\n[INA226] === FASE 2: TEST CONECTIVIDAD ===\n");
    
    I2CBus& testBus = I2CManager::getBus2();
    
    // Configuraciones específicas para cada dirección
    struct InaConfig {
        uint8_t address;
        const char* name;
        float rShunt;
        float currentLSB;
        const char* application;
    };
    
    InaConfig configs[] = {
        {0x40, "GPS (0x40)",   0.75f,  0.1f / 32768.0f,       "Alto consumo (0.75 ohm)"},      // 0.1/2^15
        {0x41, "IMU (0x41)", 10.0f,  2.5e-6/10,   "Baja corriente (10 ohm)"},      // 0.00055/2^15  
        {0x45, "MCU (0x45)",  10.0f,  0.5f / 32768.0f,       "Corriente media (10 ohm)"}      // 0.5/2^15
    };
    
    const int numConfigs = 3;
    bool addressFound[3] = {false, false, false};
    
    for (int i = 0; i < numConfigs; i++) {
        printf("[INA226] Escaneando %s...\n", configs[i].name);
        
        uint8_t manufacturerID[2];
        I2CResult scanResult = testBus.memRead(configs[i].address, 0xFE, 1, manufacturerID, 2, 100); // Manufacturer ID register
        
        if (scanResult == I2C_OK) {
            uint16_t manID = (manufacturerID[0] << 8) | manufacturerID[1];
            printf("[INA226] OK - Dispositivo encontrado en %s\n", configs[i].name);
            printf("[INA226]      Manufacturer ID: 0x%04X %s\n", manID, 
                   (manID == 0x5449) ? "(Texas Instruments - Correcto)" : "(ID no reconocido)");
            addressFound[i] = true;
        } else {
            printf("[INA226] INFO - No respuesta en %s\n", configs[i].name);
        }
    }
    
    // Contar cuántas direcciones respondieron
    int foundCount = 0;
    for (int i = 0; i < numConfigs; i++) {
        if (addressFound[i]) foundCount++;
    }
    
    if (foundCount == 0) {
        printf("[INA226] ERROR - No se encontro ningun INA226 en las direcciones especificadas\n");
        printf("[INA226] Continuando con direccion por defecto 0x40 para demostracion...\n");
        addressFound[0] = true; // Forzar test de 0x40
    } else {
        printf("[INA226] RESUMEN - %d de 3 direcciones respondieron\n", foundCount);
    }
    
    // ===== FASE 3: PRUEBA SECUENCIAL DE DIRECCIONES =====
    printf("\n[INA226] === FASE 3: PRUEBA SECUENCIAL DIRECCIONES ===\n");
    
    for (int i = 0; i < numConfigs; i++) {
        InaConfig& config = configs[i];
        
        printf("\n[INA226] --- PROBANDO %s ---\n", config.name);
        
        if (!addressFound[i]) {
            printf("[INA226] SKIP - Direccion no responde, saltando...\n");
            continue;
        }
        
        printf("[INA226] Creando instancia INA226 para direccion 0x%02X...\n", config.address);
        printf("[INA226] Parametros de configuracion:\n");
        printf("[INA226] - Aplicacion: %s\n", config.application);
        printf("[INA226] - Resistencia shunt: %.2f ohm\n", config.rShunt);
        printf("[INA226] - Current LSB: %.9f A (%.6f mA)\n", config.currentLSB, config.currentLSB * 1000);
        printf("[INA226] - Rango corriente max: %.3f A\n", config.currentLSB * 32767);
        printf("[INA226] - Promediado: 128 muestras\n");
        printf("[INA226] - Tiempo conversion: 1.1ms\n");
        printf("[INA226] - Modo: Continuo shunt+bus\n");
        
        // Crear instancia con configuración específica para esta dirección
        Ina226 ina226;  // Default constructor - no hardware access
        
        printf("[INA226] Inicializando INA226 en 0x%02X...\n", config.address);
        if (!ina226.init(config.address,                      // I2C address específica
                         config.rShunt,                       // Resistencia shunt específica
                         config.currentLSB,                   // Current LSB específico
                         Ina226Averaging::AVG_128,           // 128 samples averaging
                         Ina226ConvTime::CT_1_1MS,           // 1.1ms bus voltage conversion time
                         Ina226ConvTime::CT_1_1MS,           // 1.1ms shunt voltage conversion time
                         Ina226Mode::SHUNT_BUS_CONTINUOUS)) {  // Continuous shunt+bus measurement
            printf("[INA226] ERROR - Fallo inicializacion en 0x%02X\n", config.address);
            continue;
        }
        printf("[INA226] OK - INA226 inicializado en 0x%02X\n", config.address);
        
        // Test rápido de registros
        printf("[INA226] Ejecutando test de registros...\n");
        ina226.testINA();
        
        // ===== MEDICIONES EN TIEMPO REAL =====
        printf("[INA226] Realizando 10 mediciones cada 300ms...\n");
        printf("[INA226] Esperando rango de corriente segun configuracion:\n");
        
        // Mostrar rango esperado según la configuración
        if (config.address == 0x40) {
            printf("[INA226] - 0x40: Corrientes altas (mA-A range) con shunt 0.75 ohm\n");
        } else if (config.address == 0x41) {
            printf("[INA226] - 0x41: Corrientes muy bajas (uA range) con shunt 10 ohm\n");
        } else if (config.address == 0x45) {
            printf("[INA226] - 0x45: Corrientes medias (mA range) con shunt 10 ohm\n");
        }
        
        printf("[INA226] [#] Vshunt | Vbus | Corriente | Potencia | Estado\n");
        printf("[INA226] ----------------------------------------------\n");
        
        int successfulReads = 0;
        float totalCurrent = 0.0f;
        float minCurrent = 999999.0f;
        float maxCurrent = -999999.0f;
        
        for (int j = 1; j <= 10; j++) {
            I2CResult result;
            bool allOk = true;
            
            printf("[INA226] [%2d] ", j);
            
            // Leer voltaje shunt
            result = ina226.readShuntVoltage_mV();
            if (result == I2C_OK) {
                printf("%.3fmV | ", ina226.shuntVoltage);
            } else {
                printf("ERROR | ");
                allOk = false;
            }
            
            // Leer voltaje bus
            result = ina226.readBusVoltage_mV();
            if (result == I2C_OK) {
                printf("%.2fV | ", ina226.busVoltage / 1000);
            } else {
                printf("ERROR | ");
                allOk = false;
            }
            
            // Leer corriente
            result = ina226.readCurrent_mA();
            if (result == I2C_OK) {
                if (config.address == 0x41) {
                    // Para 0x41 mostrar en microamperios debido a corrientes muy bajas
                    printf("%.3fuA | ", ina226.current * 1000);
                } else {
                    printf("%.3fmA | ", ina226.current);
                }
                
                // Estadísticas
                totalCurrent += ina226.current;
                if (ina226.current < minCurrent) minCurrent = ina226.current;
                if (ina226.current > maxCurrent) maxCurrent = ina226.current;
            } else {
                printf("ERROR | ");
                allOk = false;
            }
            
            // Leer potencia
            result = ina226.readPower_mW();
            if (result == I2C_OK) {
                if (config.address == 0x41) {
                    // Para 0x41 mostrar en microwatios
                    printf("%.3fuW | ", ina226.power * 1000);
                } else {
                    printf("%.3fmW | ", ina226.power);
                }
            } else {
                printf("ERROR | ");
                allOk = false;
            }
            
            // Estado de la medición
            if (allOk) {
                printf("OK");
                successfulReads++;
            } else {
                printf("ERROR");
            }
            
            printf("\n");
            HAL_Delay(300); // 300ms entre mediciones
        }
        
        // Análisis de resultados para esta dirección
        printf("[INA226] RESULTADO 0x%02X: %d/10 lecturas exitosas (%.1f%%)\n", 
               config.address, successfulReads, (float)successfulReads * 10.0f);
        
        if (successfulReads >= 8) {
            printf("[INA226] EXCELENTE - Sensor en 0x%02X funciona correctamente\n", config.address);
        } else if (successfulReads >= 5) {
            printf("[INA226] BUENO - Sensor en 0x%02X funciona con algunos errores\n", config.address);
        } else {
            printf("[INA226] PROBLEMAS - Sensor en 0x%02X tiene fallas frecuentes\n", config.address);
        }
        
        // Estadísticas de corriente si hubo mediciones exitosas
        if (successfulReads > 0) {
            float avgCurrent = totalCurrent / successfulReads;
            printf("[INA226] ESTADISTICAS 0x%02X:\n", config.address);
            
            if (config.address == 0x41) {
                printf("[INA226]   Promedio: %.3fuA, Min: %.3fuA, Max: %.3fuA\n", 
                       avgCurrent * 1000, minCurrent * 1000, maxCurrent * 1000);
            } else {
                printf("[INA226]   Promedio: %.3fmA, Min: %.3fmA, Max: %.3fmA\n", 
                       avgCurrent, minCurrent, maxCurrent);
            }
            
            // Análisis según la configuración esperada
            float currentAbs = (avgCurrent < 0) ? -avgCurrent : avgCurrent;
            if (config.address == 0x40) {
                // Alto consumo esperado
                if (currentAbs > 10.0f) {
                    printf("[INA226]   OK - Corriente alta como esperado para 0x40\n");
                } else {
                    printf("[INA226]   INFO - Corriente menor a esperada para aplicacion alto consumo\n");
                }
            } else if (config.address == 0x41) {
                // Muy bajo consumo esperado
                if (currentAbs < 1.0f) {
                    printf("[INA226]   OK - Corriente muy baja como esperado para 0x41\n");
                } else {
                    printf("[INA226]   WARNING - Corriente mayor a esperada para aplicacion bajo consumo\n");
                }
            } else if (config.address == 0x45) {
                // Consumo medio esperado
                if (currentAbs > 1.0f && currentAbs < 100.0f) {
                    printf("[INA226]   OK - Corriente media como esperado para 0x45\n");
                } else {
                    printf("[INA226]   INFO - Corriente fuera del rango medio esperado\n");
                }
            }
        }
        
        printf("[INA226] --- FIN PRUEBA 0x%02X ---\n", config.address);
    }
    
    // ===== CONCLUSION GENERAL =====
    printf("\n[INA226] ========== CONCLUSION GENERAL ==========\n");
    printf("[INA226] RESUMEN DE CONFIGURACIONES PROBADAS:\n");
    
    for (int i = 0; i < numConfigs; i++) {
        if (addressFound[i]) {
            printf("[INA226] OK %s - %s\n", configs[i].name, configs[i].application);
            printf("[INA226]    Rshunt=%.2f ohm, LSB=%.9f A\n", configs[i].rShunt, configs[i].currentLSB);
        } else {
            printf("[INA226] -- %s - No detectado\n", configs[i].name);
        }
    }
    
    printf("[INA226] VENTAJAS CONFIGURACION MULTI-SENSOR:\n");
    printf("[INA226] + Configuraciones optimizadas por aplicacion\n");
    printf("[INA226] + Diferentes rangos de medicion por sensor\n");
    printf("[INA226] + Resistencias shunt especificas\n");
    printf("[INA226] + Current LSB calculado por precision requerida\n");
    printf("[INA226] + 0x40: Alto consumo (0.75 ohm)\n");
    printf("[INA226] + 0x41: Bajo consumo precision (10 ohm)\n");
    printf("[INA226] + 0x45: Consumo medio (10 ohm)\n");
    printf("[INA226] + Arquitectura thread-safe unificada\n");
    printf("[INA226] =========================================\n");
}

/**
 * @brief Test de diferentes configuraciones del INA226
 */
extern "C" void ina226_configuration_test(void) {
    printf("[INA226-CFG] ========== TEST CONFIGURACIONES ==========\n");
    
    if (!I2CManager::isInitialized()) {
        I2CManager::initializeAll();
    }
    
    printf("[INA226-CFG] Probando diferentes configuraciones...\n");
    
    // Configuración 1: Alta velocidad, baja precisión
    printf("\n[INA226-CFG] === CONFIG 1: ALTA VELOCIDAD ===\n");
    {
        Ina226 ina226_fast;
        
        if (ina226_fast.init(0x40, 0.75f, 0.00000305f,
                             Ina226Averaging::AVG_1,         // Sin promediado
                             Ina226ConvTime::CT_140US,       // Conversión rápida
                             Ina226ConvTime::CT_140US,       // Conversión rápida
                             Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
            printf("[INA226-CFG] Config alta velocidad inicializada\n");
            
            // 5 mediciones rápidas
            for (int i = 0; i < 5; i++) {
                if (ina226_fast.readCurrent_mA() == I2C_OK) {
                    printf("[INA226-CFG] Lectura rápida %d: %.3fmA\n", i+1, ina226_fast.current);
                }
                HAL_Delay(50); // Solo 50ms entre lecturas
            }
        }
    }
    
    // Configuración 2: Alta precisión, baja velocidad  
    printf("\n[INA226-CFG] === CONFIG 2: ALTA PRECISION ===\n");
    {
        Ina226 ina226_precise;
        
        if (ina226_precise.init(0x40, 0.75f, 0.00000305f,
                                Ina226Averaging::AVG_1024,      // Máximo promediado
                                Ina226ConvTime::CT_8_244MS,     // Conversión lenta
                                Ina226ConvTime::CT_8_244MS,     // Conversión lenta  
                                Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
            printf("[INA226-CFG] Config alta precision inicializada\n");
            printf("[INA226-CFG] NOTA: Cada medicion toma ~17ms (8.244ms*2 + 1024 avg)\n");
            
            // 3 mediciones precisas
            for (int i = 0; i < 3; i++) {
                uint32_t startTime = HAL_GetTick();
                if (ina226_precise.readCurrent_mA() == I2C_OK) {
                    uint32_t elapsed = HAL_GetTick() - startTime;
                    printf("[INA226-CFG] Lectura precisa %d: %.6fmA (tomo %ums)\n", 
                           i+1, ina226_precise.current, (unsigned int)elapsed);
                }
                HAL_Delay(1000); // 1 segundo entre lecturas precisas
            }
        }
    }
    
    printf("[INA226-CFG] =========================================\n");
}

/**
 * @brief Test de monitoreo continuo del INA226
 */
extern "C" void ina226_continuous_monitor(void) {
    printf("[INA226-MONITOR] Iniciando monitor continuo INA226...\n");
    printf("[INA226-MONITOR] Presione reset para detener\n");
    
    if (!I2CManager::isInitialized()) {
        I2CManager::initializeAll();
    }
    
    Ina226 ina226;
    
    if (!ina226.init(0x40, 0.75f, 0.00000305f,
                     Ina226Averaging::AVG_64,
                     Ina226ConvTime::CT_1_1MS,  
                     Ina226ConvTime::CT_1_1MS,
                     Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        printf("[INA226-MONITOR] ERROR - Fallo inicializacion\n");
        return;
    }
    
    uint32_t readCount = 0;
    float maxCurrent = 0.0f;
    float minCurrent = 999999.0f;
    float avgCurrent = 0.0f;
    
    while (1) {
        readCount++;
        
        if (ina226.readCurrent_mA() == I2C_OK &&
            ina226.readBusVoltage_mV() == I2C_OK &&
            ina226.readPower_mW() == I2C_OK) {
            
            // Actualizar estadísticas
            if (ina226.current > maxCurrent) maxCurrent = ina226.current;
            if (ina226.current < minCurrent) minCurrent = ina226.current;
            avgCurrent = (avgCurrent * (readCount - 1) + ina226.current) / readCount;
            
            // Mostrar cada 50 lecturas
            if (readCount % 50 == 0) {
                printf("[INA226-MONITOR] #%u: %.3fV, %.3fmA, %.3fmW | Avg=%.3f Min=%.3f Max=%.3f\n",
                       (unsigned int)readCount, ina226.busVoltage, ina226.current, ina226.power,
                       avgCurrent, minCurrent, maxCurrent);
            }
        }
        
        HAL_Delay(100); // 100ms = 10Hz
    }
}