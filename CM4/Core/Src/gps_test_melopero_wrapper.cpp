/**
 * @file gps_test_melopero_wrapper.cpp
 * @brief Test wrapper for Melopero-based GPS implementation
 * @details Provides comprehensive testing of the new UBX protocol GPS implementation
 */

#include "../../Modules/GPS/sam_m10q_melopero.h"
#include "I2CManager.h"
#include "main.h"
#include <stdio.h>

/**
 * @brief Comprehensive test of Melopero-based GPS implementation
 * 
 * This function tests all aspects of the new GPS implementation:
 * - I2C connectivity
 * - UBX protocol configuration
 * - NAV-PVT data parsing
 * - ACK/NAK handling
 * - Data validation
 */
extern "C" void gps_melopero_comprehensive_test(void) {
    printf("[GPS-MELOPERO] ========================================\n");
    printf("[GPS-MELOPERO] INICIANDO TEST COMPRENSIVO MELOPERO\n");
    printf("[GPS-MELOPERO] Implementacion basada en enfoque Melopero SAM-M8Q\n");
    printf("[GPS-MELOPERO] ========================================\n");
    
    // ===== FASE 1: INICIALIZACION I2C =====
    printf("\n[GPS-MELOPERO] === FASE 1: INICIALIZACION I2C ===\n");
    
    if (!I2CManager::isInitialized()) {
        printf("[GPS-MELOPERO] Inicializando I2CManager...\n");
        if (!I2CManager::initializeAll()) {
            printf("[GPS-MELOPERO] ERROR CRITICO - Fallo inicializacion I2CManager\n");
            return;
        }
    }
    printf("[GPS-MELOPERO] OK - I2CManager inicializado\n");
    
    // ===== FASE 2: TEST CONECTIVIDAD =====
    printf("\n[GPS-MELOPERO] === FASE 2: TEST CONECTIVIDAD ===\n");
    
    I2CBus& testBus = I2CManager::getBus2();
    uint8_t dummyData;
    
    printf("[GPS-MELOPERO] Escaneando GPS en direccion 0x42...\n");
    I2CResult scanResult = testBus.memRead(0x42, 0xFF, 1, &dummyData, 1, 100);
    
    if (scanResult == I2C_OK) {
        printf("[GPS-MELOPERO] OK - GPS responde en direccion 0x42\n");
    } else if (scanResult == I2C_NACK) {
        printf("[GPS-MELOPERO] INFO - NACK en 0x42 (normal si buffer vacio)\n");
    } else {
        printf("[GPS-MELOPERO] ERROR - Error I2C: %d\n", scanResult);
        printf("[GPS-MELOPERO] Continuando con test...\n");
    }
    
    // ===== FASE 3: CREACION E INICIALIZACION GPS =====
    printf("\n[GPS-MELOPERO] === FASE 3: INICIALIZACION GPS ===\n");
    
    printf("[GPS-MELOPERO] Creando instancia SamM10qMelopero...\n");
    SamM10qMelopero gps(&testBus, 0x42);
    
    printf("[GPS-MELOPERO] Inicializando GPS...\n");
    if (!gps.initialize()) {
        printf("[GPS-MELOPERO] ERROR - Fallo inicializacion GPS\n");
        return;
    }
    printf("[GPS-MELOPERO] OK - GPS inicializado correctamente\n");
    
    // ===== FASE 4: CONFIGURACION UBX + INTERIOR =====
    printf("\n[GPS-MELOPERO] === FASE 4: CONFIGURACION UBX + INTERIOR ===\n");
    printf("[GPS-MELOPERO] Aplicando configuracion Melopero optimizada para interiores...\n");
    printf("[GPS-MELOPERO] - Protocolo UBX solamente\n");
    printf("[GPS-MELOPERO] - Modelo dinamico estatico\n");
    printf("[GPS-MELOPERO] - GPS + SBAS + Galileo + GLONASS\n");
    printf("[GPS-MELOPERO] - Mensajes NAV-PVT habilitados\n");
    printf("[GPS-MELOPERO] - Frecuencia optimizada 5Hz (5000ms)\n");
    
    if (!gps.configure(5000)) { // 5 seconds for indoor use
        printf("[GPS-MELOPERO] ERROR - Fallo configuracion UBX\n");
        printf("[GPS-MELOPERO] El GPS puede no estar respondiendo correctamente\n");
        // Continuamos el test para verificar comunicacion basica
    } else {
        printf("[GPS-MELOPERO] OK - Configuracion UBX + Interior aplicada\n");
    }
    
    // Delay para estabilizacion de configuracion
    printf("[GPS-MELOPERO] Esperando estabilizacion configuracion (5 segundos)...\n");
    HAL_Delay(5000);
    
    // ===== FASE 5: INFORMACION SATELITES =====
    printf("\n[GPS-MELOPERO] === FASE 5: INFORMACION SATELITES ===\n");
    printf("[GPS-MELOPERO] Consultando estado de constelaciones...\n");
    gps.printSatelliteInfo(16); // Mostrar hasta 16 satelites
    
    // ===== FASE 6: TEST ADQUISICION DATOS COLD START =====
    printf("\n[GPS-MELOPERO] === FASE 6: TEST COLD START (INTERIOR) ===\n");
    printf("[GPS-MELOPERO] ATENCION: Cold start en interiores puede tomar 10-15 minutos\n");
    printf("[GPS-MELOPERO] Probando lectura NAV-PVT cada 30 segundos por 10 intentos...\n");
    printf("[GPS-MELOPERO] Total tiempo de test: ~5 minutos\n");
    
    int sucessfulReads = 0;
    int validFixes = 0;

    for (int attempt = 1; attempt <= 45; attempt++) {
        printf("\n[GPS-MELOPERO] --- Intento %d/10 (Tiempo transcurrido: %d minutos) ---\n", 
               attempt, (attempt - 1) / 2); // Cada 2 intentos = 1 minuto aprox
        
        // Show satellite info every 3 attempts
        if (attempt % 3 == 1) {
            printf("[GPS-MELOPERO] Actualizacion estado satelites:\n");
            gps.printSatelliteInfo(8); // Mostrar solo 8 satelites para no saturar
        }
        
        bool dataReceived = gps.getNavigationData(true, 10000); // Poll with 10s timeout for cold start
        
        if (dataReceived) {
            sucessfulReads++;
            const GpsNavData& data = gps.getNavData();
            
            printf("[GPS-MELOPERO] OK - Datos NAV-PVT recibidos\n");
            printf("[GPS-MELOPERO] Tiempo GPS: %04d/%02d/%02d %02d:%02d:%02d\n",
                   data.year, data.month, data.day, 
                   data.hour, data.minute, data.second);
            
            printf("[GPS-MELOPERO] iTOW: %u ms\n", (unsigned int)data.iTOW);
            printf("[GPS-MELOPERO] Fix: %s\n", gps.getFixTypeString());
            printf("[GPS-MELOPERO] Satelites: %d\n", data.numSV);
            
            // Coordenadas
            printf("[GPS-MELOPERO] Coordenadas RAW: Lat=%d, Lon=%d (escala 1e-7)\n", 
                   (int)data.latitude, (int)data.longitude);
            printf("[GPS-MELOPERO] Coordenadas: %.8f deg, %.8f deg\n",
                   data.getLatitudeDegrees(), data.getLongitudeDegrees());
            
            // Altitud
            printf("[GPS-MELOPERO] Altura: %.3f m (elipsoide), %.3f m (MSL)\n",
                   data.getHeightMeters(), data.getHMSLMeters());
            
            // Precision
            printf("[GPS-MELOPERO] Precision: H=%.3fm, V=%.3fm\n",
                   data.getHAccMeters(), data.getVAccMeters());
            
            // Velocidad
            printf("[GPS-MELOPERO] Velocidad: %.3f m/s, N=%.3f, E=%.3f, D=%.3f\n",
                   (double)data.gSpeed * 1e-3,
                   (double)data.velN * 1e-3, 
                   (double)data.velE * 1e-3, 
                   (double)data.velD * 1e-3);
            
            // Validez
            printf("[GPS-MELOPERO] Validez: Fecha=%s, Tiempo=%s, Resuelto=%s\n",
                   data.validDate ? "SI" : "NO",
                   data.validTime ? "SI" : "NO", 
                   data.fullyResolved ? "SI" : "NO");
            
            if (gps.hasValidFix()) {
                validFixes++;
                printf("[GPS-MELOPERO] SUCCESS - Fix satelital valido!\n");
                
                // Verificar coordenadas no-cero
                if (data.getLatitudeDegrees() != 0.0 || data.getLongitudeDegrees() != 0.0) {
                    printf("[GPS-MELOPERO] SUCCESS - Coordenadas validas recibidas!\n");
                } else {
                    printf("[GPS-MELOPERO] WARN - Fix valido pero coordenadas en cero\n");
                }
            } else {
                printf("[GPS-MELOPERO] INFO - Sin fix satelital (normal en interiores)\n");
            }
            
        } else {
            printf("[GPS-MELOPERO] WARN - No se recibieron datos NAV-PVT\n");
            printf("[GPS-MELOPERO] Posibles causas:\n");
            printf("[GPS-MELOPERO] - GPS aun inicializando\n");
            printf("[GPS-MELOPERO] - Configuracion UBX no aplicada\n");
            printf("[GPS-MELOPERO] - Problema comunicacion I2C\n");
        }
        
        // Delay entre intentos (30 segundos para cold start)
        if (attempt < 45) {
            printf("[GPS-MELOPERO] Esperando 30 segundos para siguiente intento...\n");
            HAL_Delay(30000); // 30 seconds between attempts for cold start
        }
    }
    
    // ===== FASE 6: ESTADISTICAS FINALES =====
    printf("\n[GPS-MELOPERO] === FASE 6: ESTADISTICAS FINALES ===\n");
    
    printf("[GPS-MELOPERO] Lecturas exitosas: %d/10 (%.1f%%)\n", 
           sucessfulReads, (float)sucessfulReads * 10.0f);
    printf("[GPS-MELOPERO] Fixes validos: %d/10 (%.1f%%)\n", 
           validFixes, (float)validFixes * 10.0f);
    
    // ===== FASE 7: TEST FUNCIONES C =====
    printf("\n[GPS-MELOPERO] === FASE 7: TEST INTERFAZ C ===\n");
    printf("[GPS-MELOPERO] Probando funciones wrapper C...\n");
    
    // Test usando interfaz C
    if (gps_melopero_get_data()) {
        printf("[GPS-MELOPERO] OK - gps_melopero_get_data() funcional\n");
        printf("[GPS-MELOPERO] Lat C: %.8f\n", gps_melopero_get_latitude());
        printf("[GPS-MELOPERO] Lon C: %.8f\n", gps_melopero_get_longitude());
        printf("[GPS-MELOPERO] Fix C: %s\n", gps_melopero_has_fix() ? "SI" : "NO");
    } else {
        printf("[GPS-MELOPERO] WARN - Interfaz C no obtuvo datos\n");
    }
    
    // ===== CONCLUSION =====
    printf("\n[GPS-MELOPERO] ========== CONCLUSION ==========\n");
    
    if (sucessfulReads >= 7) {
        printf("[GPS-MELOPERO] EXCELENTE - Comunicacion UBX estable (>70%% exito)\n");
        printf("[GPS-MELOPERO] El protocolo Melopero funciona correctamente\n");
    } else if (sucessfulReads >= 3) {
        printf("[GPS-MELOPERO] BUENO - Comunicacion UBX parcial (30-70%% exito)\n");
        printf("[GPS-MELOPERO] GPS respondiendo, posible mejora en configuracion\n");
    } else {
        printf("[GPS-MELOPERO] PROBLEMAS - Comunicacion UBX limitada (<30%% exito)\n");
        printf("[GPS-MELOPERO] Revisar cableado, alimentacion o configuracion\n");
    }
    
    if (validFixes > 0) {
        printf("[GPS-MELOPERO] SUCCESS - GPS obtuvo fix satelital!\n");
        printf("[GPS-MELOPERO] Sistema listo para navegacion\n");
    } else {
        printf("[GPS-MELOPERO] INFO - Sin fix satelital (normal en interiores)\n");
        printf("[GPS-MELOPERO] Mover al exterior para prueba completa\n");
    }
    
    printf("[GPS-MELOPERO] VENTAJAS IMPLEMENTACION MELOPERO:\n");
    printf("[GPS-MELOPERO] + Protocolo UBX binario (mas eficiente que NMEA)\n");
    printf("[GPS-MELOPERO] + Configuracion con ACK/NAK (confirmacion)\n");
    printf("[GPS-MELOPERO] + Datos NAV-PVT completos (posicion + velocidad + tiempo)\n");
    printf("[GPS-MELOPERO] + Precision escalada (1e-7 para coordenadas)\n");
    printf("[GPS-MELOPERO] + Thread-safe con I2CManager\n");
    printf("[GPS-MELOPERO] + Sin asignacion dinamica (embedded-friendly)\n");
    
    printf("[GPS-MELOPERO] ==========================================\n");
}

/**
 * @brief Test de comunicacion UBX personalizada
 * 
 * Demuestra como enviar mensajes UBX personalizados usando la nueva implementacion
 */
extern "C" void gps_melopero_custom_ubx_test(void) {
    printf("[GPS-UBX] ========== TEST UBX PERSONALIZADO ==========\n");
    
    // Inicializar si no esta hecho
    if (!I2CManager::isInitialized()) {
        I2CManager::initializeAll();
    }
    
    SamM10qMelopero gps(&I2CManager::getBus2(), 0x42);
    
    if (!gps.initialize()) {
        printf("[GPS-UBX] ERROR - No se pudo inicializar GPS\n");
        return;
    }
    
    printf("[GPS-UBX] GPS inicializado para test UBX personalizado\n");
    
    // Test 1: Poll version info (si esta disponible)
    printf("[GPS-UBX] Test 1: Polling mensaje version...\n");
    uint8_t responseBuffer[256];
    uint16_t responseSize = 0;
    
    // Intentar obtener version del modulo (MON-VER)
    if (gps.pollUbxMessage(0x0A, 0x04, responseBuffer, sizeof(responseBuffer), responseSize, 2000)) {
        printf("[GPS-UBX] OK - Mensaje version recibido (%d bytes)\n", responseSize);
        
        // Mostrar algunos bytes de la respuesta
        printf("[GPS-UBX] Respuesta (primeros 20 bytes): ");
        for (int i = 0; i < 20 && i < responseSize; i++) {
            printf("%02X ", responseBuffer[i]);
        }
        printf("\n");
    } else {
        printf("[GPS-UBX] INFO - No se recibio mensaje version (normal)\n");
    }
    
    // Test 2: Enviar mensaje personalizado
    printf("[GPS-UBX] Test 2: Enviando mensaje CFG-RATE personalizado...\n");
    
    // Configurar 2Hz (500ms)
    uint8_t customPayload[] = {
        0xF4, 0x01, // 500ms period (little endian)
        0x01, 0x00, // Navigation rate = 1
        0x00, 0x00  // Time reference = UTC
    };
    
    if (gps.sendUbxMessage(UBX::CFG_CLASS, UBX::CFG_RATE, customPayload, sizeof(customPayload))) {
        printf("[GPS-UBX] OK - Mensaje CFG-RATE enviado\n");
        HAL_Delay(500); // Esperar procesamiento
    } else {
        printf("[GPS-UBX] ERROR - Fallo envio CFG-RATE\n");
    }
    
    // Test 3: Verificar nueva configuracion con datos
    printf("[GPS-UBX] Test 3: Verificando nueva configuracion (2Hz)...\n");
    
    uint32_t lastTime = HAL_GetTick();
    for (int i = 0; i < 5; i++) {
        if (gps.getNavigationData(true, 2000)) {
            uint32_t currentTime = HAL_GetTick();
            uint32_t deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            
            const GpsNavData& data = gps.getNavData();
            printf("[GPS-UBX] Datos #%d: iTOW=%u, intervalo=%ums\n", 
                   i+1, (unsigned int)data.iTOW, (unsigned int)deltaTime);
        }
        HAL_Delay(100);
    }
    
    printf("[GPS-UBX] ==========================================\n");
}

/**
 * @brief Test continuo para monitoreo de performance
 */
extern "C" void gps_melopero_continuous_monitor(void) {
    printf("[GPS-MONITOR] Iniciando monitor continuo GPS Melopero...\n");
    printf("[GPS-MONITOR] Presione reset para detener\n");
    
    // Usar interfaz C para simplicidad
    if (!gps_melopero_init()) {
        printf("[GPS-MONITOR] ERROR - Fallo inicializacion\n");
        return;
    }
    
    uint32_t readCount = 0;
    uint32_t sucessCount = 0;
    uint32_t fixCount = 0;
    
    while (1) {
        readCount++;
        
        if (gps_melopero_get_data()) {
            sucessCount++;
            
            if (gps_melopero_has_fix()) {
                fixCount++;
                
                // Solo mostrar cuando hay fix para reducir spam
                printf("[GPS-MONITOR] #%u: Fix! Lat=%.6f, Lon=%.6f\n",
                       (unsigned int)readCount,
                       gps_melopero_get_latitude(),
                       gps_melopero_get_longitude());
            }
        }
        
        // Estadisticas cada 50 lecturas
        if (readCount % 50 == 0) {
            float successRate = (float)sucessCount / readCount * 100.0f;
            float fixRate = (float)fixCount / readCount * 100.0f;
            
            printf("[GPS-MONITOR] Estadisticas: %u lecturas, %.1f%% exito, %.1f%% fix\n",
                   (unsigned int)readCount, successRate, fixRate);
        }
        
        HAL_Delay(2000); // 2 second interval
    }
}