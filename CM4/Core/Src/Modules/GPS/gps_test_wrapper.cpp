/**
 * @file gps_test_wrapper.cpp
 * @brief Wrapper C++ para probar GPS desde main.c
 * 
 * Este archivo proporciona una interfaz C para probar el GPS SAM-M10Q
 * desde el main.c que está en C.
 */

#include "../../Modules/GPS/sam_m10q.h"
#include "I2CManager.h"
#include "main.h"
#include <stdio.h>
#include "UARTManager.h"
#include "stm32wlxx_hal_uart.h"

// Handle I2C2 externo definido en i2c.c
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
/**
 * @brief Función C wrapper para inicializar y probar GPS
 * 
 * Esta función puede ser llamada desde código C y maneja internamente
 * la lógica C++ del GPS thread-safe.
 */
extern "C" void gps_init_and_test(void) {
    printf("[GPS] ========== DIAGNOSTICO COMPLETO GPS ==========\r\n");
    printf("[GPS] Inicializando sistema I2C thread-safe...\r\n");
    
    // ¡IMPORTANTE! Inicializar el I2CManager primero
    if (!I2CManager::initializeAll()) {
        printf("[GPS] ERROR CRITICO: No se pudo inicializar I2CManager\r\n");
        printf("[GPS] Verifique que hi2c2 este correctamente configurado\r\n");
        return;
    }

    printf("[GPS] OK - I2CManager inicializado correctamente\r\n");
    printf("[GPS] Verificando bus I2C...\r\n");

    // Inicializar UARTManager
    if(!UARTManager::initializeAll()) {
        printf("[GPS] ERROR CRITICO: No se pudo inicializar UART1 para GPS\r\n");
        return;
    }
    printf("[GPS] OK - UART1 inicializado correctamente\r\n");

    
    
    // Test básico de conectividad I2C antes de crear GPS
    I2CBus& testBus = I2CManager::getBus2();
    uint8_t dummyData;
    
    // Probar direcciones GPS comunes: 0x42 (GPS SAM-M10Q)
    printf("[GPS] Escaneando direccion GPS 0x42...\r\n");
    I2CResult scanResult = testBus.memRead(0x42, 0xFF, 1, &dummyData, 1, 100);
    
    if (scanResult == I2C_OK) {
        printf("[GPS] OK - GPS encontrado en direccion 0x42\r\n");
    } else if (scanResult == I2C_NACK) {
        printf("[GPS] WARN - No hay respuesta en 0x42 (GPS desconectado o direccion incorrecta)\r\n");
    } else {
        printf("[GPS] ERROR - Error I2C: %d (revisar cableado/alimentacion)\r\n", scanResult);
    }
    
    printf("[GPS] Creando instancia GPS SAM-M10Q...\r\n");
    
    // CORRECCIÓN: Usar dirección 7-bit (0x42), no 8-bit (0x84)
    SamM10q gps;  // Default constructor - no hardware access
    
    printf("[GPS] Instancia creada. Inicializando GPS...\r\n");
    
    // Inicializar GPS
    if (!gps.init(0x42)) {  // Dirección 7-bit correcta para GPS SAM-M10Q
        printf("[GPS] ERROR - Fallo inicializacion GPS\r\n");
        return;
    }
    
    printf("[GPS] GPS inicializado\r\n");
    printf("[GPS] ========== CONFIGURACIÓN GPS ==========\r\n");
    
    // Test de configuración más detallado
    // printf("[GPS] Aplicando configuración MEDIUM rate...\r\n");
    // bool config_result = gps.set_new_acq_time(gpsRateSpeed::MEDIUM);
    
    // if (config_result) {
    //     printf("[GPS] OK - Configuracion aplicada exitosamente\r\n");
    // } else {
    //     printf("[GPS] WARN - Configuracion fallo - GPS puede no estar respondiendo\r\n");
    // }
    
    // Esperar tiempo para que GPS configure
    printf("[GPS] Esperando estabilizacion GPS (3 segundos)...\r\n");
    HAL_Delay(3000);
    
    printf("[GPS] ========== TEST DE LECTURA DETALLADO ==========\r\n");
    
    // Test de lectura NMEA más detallado
    for (int i = 0; i < 5; i++) {
        printf("[GPS] === Intento de lectura #%d ===\r\n", i + 1);
        
        // Leer posicion
        HAL_StatusTypeDef result = gps.read_gps_position();
        
        printf("[GPS] Status I2C: %s\r\n", (result == HAL_OK) ? "OK" : "ERROR");
        
        // Verificar datos GPS
        if (result == HAL_OK) {
            printf("[GPS] Latitud: %.8f grados\r\n", gps.latitude);
            printf("[GPS] Longitud: %.8f grados\r\n", gps.longitude);
            
            // Verificar si los datos son validos (no cero)
            if (gps.latitude != 0.0 || gps.longitude != 0.0) {
                printf("[GPS] OK - DATOS GPS VALIDOS RECIBIDOS!\r\n");
                
                if (gps.fechaUTC > 0) {
                    printf("[GPS] Fecha UTC: %06u\r\n", (unsigned int)gps.fechaUTC);
                }
                if (gps.horaUTC > 0) {
                    printf("[GPS] Hora UTC: %06u\r\n", (unsigned int)gps.horaUTC);
                }
            } else {
                printf("[GPS] WARN - GPS sin fix - datos en cero\r\n");
                printf("[GPS] Posible causa: Sin senal satelital o GPS en indoor\r\n");
            }
        } else {
            printf("[GPS] ERROR - NO FIX\r\n");
        }
        
        // Delay entre lecturas
        HAL_Delay(1000);
    }
    
    printf("[GPS] ========== DIAGNOSTICO FINAL ==========\r\n");
    
    // Test directo de stream NMEA
    //PESSI: REALIZO CAMBIOS YA QUE NO USAMOS NMEA STREAM AHORA, USAMOS GETPVT.
    printf("[GPS] Test directo de getPVT...\r\n");
    HAL_StatusTypeDef streamResult = gps.read_gps_position();
    
    if (streamResult == HAL_OK) {
        printf("[GPS] OK - getPVT leido correctamente\r\n");
        printf("[GPS] Datos procesados en la estructura de datos PVT\r\n");
    } else {
        printf("[GPS] ERROR - Error utilizando la función getPVT\r\n");
    }
    
    printf("[GPS] ========== CONCLUSION ==========\r\n");
    
    if (gps.latitude != 0.0 || gps.longitude != 0.0) {
        printf("[GPS] SUCCESS - GPS FUNCIONANDO - Datos validos recibidos\r\n");
        printf("[GPS] INFO - Sistema listo para navegacion\r\n");
    } else {
        printf("[GPS] INFO - GPS comunicando pero sin fix satelital\r\n");
        printf("[GPS] TIP - Mueva el dispositivo al exterior para fix\r\n");
        printf("[GPS] STATUS - Sistema I2C thread-safe funcionando correctamente\r\n");
    }
    
    printf("[GPS] ==========================================\n\r\n");
}

/**
 * @brief Función adicional para test continuo (opcional)
 * 
 * Esta función puede ser llamada desde una tarea FreeRTOS para
 * hacer lecturas continuas del GPS.
 */
extern "C" void gps_continuous_test(void) {
    static SamM10q gps_instance;  // Static object, initialized only once
    static bool initialized = false;
    
    if (!initialized) {
        // Inicializar I2CManager si no está ya inicializado
        if (!I2CManager::isInitialized()) {
            I2CManager::initializeAll();
        }
        
        if (!gps_instance.init(0x42)) {  // Direccion 7-bit correcta
            printf("[GPS] ERROR - Fallo inicializacion GPS\r\n");
            return;
        }
        initialized = true;
        printf("[GPS] Instancia para test continuo creada\r\n");
    }
    
    // Lectura continua cada 2 segundos
    while (1) {
        HAL_StatusTypeDef result = gps_instance.read_gps_position();
        
        if (result == HAL_OK) {
            printf("[GPS] Continuo - Lat: %.6f, Lon: %.6f\r\n", 
                   gps_instance.latitude, gps_instance.longitude);
        } else {
            printf("[GPS] Continuo - Sin senal GPS\r\n");
        }
        
        HAL_Delay(2000);  // 2 segundos
    }
}