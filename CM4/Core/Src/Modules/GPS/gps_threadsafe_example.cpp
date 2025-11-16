/**
 * @file gps_threadsafe_example.cpp
 * @brief Ejemplo de uso del GPS SAM-M10Q con arquitectura I2C thread-safe
 * 
 * Este ejemplo muestra cómo usar el GPS SAM-M10Q con la nueva implementación
 * thread-safe que evita colisiones de bus I2C en entornos multitarea.
 */

#include "sam_m10q.h"
#include "I2CManager.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

// Instancia global del GPS
SamM10q* gps = nullptr;

/**
 * @brief Tarea de lectura del GPS
 * @param pvParameters Parámetros de la tarea (no usado)
 */
void gps_task(void* pvParameters) {
    printf("[GPS] Iniciando tarea GPS thread-safe...\n");
    
    // Configurar e inicializar GPS
    gps = new SamM10q(0x42);  // Dirección GPS desplazada para HAL
    
    // Inicializar GPS (esto ahora incluye inicialización del bus thread-safe)
    gps->initSamM10q();
    
    printf("[GPS] GPS inicializado con arquitectura thread-safe\n");
    
    uint32_t lastReadTime = 0;
    
    while (1) {
        uint32_t currentTime = xTaskGetTickCount();
        
        // Leer GPS cada 2 segundos
        if (currentTime - lastReadTime >= pdMS_TO_TICKS(2000)) {
            lastReadTime = currentTime;
            
            // Leer posición GPS usando método thread-safe
            HAL_StatusTypeDef result = gps->read_gps_position();
            
            if (result == HAL_OK) {
                double lat = gps->latitude;
                double lon = gps->longitude;
                uint32_t fecha = gps->fechaUTC;
                uint32_t hora = gps->horaUTC;
                
                printf("[GPS] Posicion: %.6f, %.6f - Fecha: %06lu Hora: %06lu\n", 
                       lat, lon, fecha, hora);
            } else {
                printf("[GPS] Error leyendo posicion GPS\n");
            }
        }
        
        // Yield para otras tareas
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Función de inicialización del ejemplo GPS thread-safe
 * 
 * Esta función debe ser llamada desde main() después de la inicialización
 * del sistema y antes del inicio del scheduler de FreeRTOS.
 */
void gps_threadsafe_example_init(void) {
    printf("Iniciando ejemplo GPS thread-safe...\n");
    
    // Inicializar el manager I2C thread-safe
    if (!I2CManager::initializeAll()) {
        printf("ERROR: No se pudo inicializar el sistema I2C thread-safe\n");
        return;
    }
    
    printf("Sistema I2C thread-safe inicializado correctamente\n");
    
    // Crear tarea GPS
    xTaskCreate(gps_task, "GPS_Task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
    
    printf("Tarea GPS creada - sistema listo\n");
}

/**
 * @brief Función para obtener lectura única del GPS
 * 
 * Esta función puede ser llamada desde cualquier tarea para obtener
 * una lectura rápida del GPS de forma thread-safe.
 * 
 * @return true si la lectura fue exitosa
 */
bool gps_get_single_position(double* lat, double* lon) {
    if (!gps || !lat || !lon) {
        return false;
    }
    
    // Leer posición de forma thread-safe
    HAL_StatusTypeDef result = gps->read_gps_position();
    
    if (result == HAL_OK) {
        *lat = gps->latitude;
        *lon = gps->longitude;
        return true;
    }
    
    return false;
}

/**
 * @brief Función de test simple para verificar funcionamiento
 */
void gps_test_threadsafe(void) {
    printf("[TEST] Iniciando test GPS thread-safe...\n");
    
    if (!gps) {
        printf("[TEST] GPS no inicializado\n");
        return;
    }
    
    double lat, lon;
    if (gps_get_single_position(&lat, &lon)) {
        printf("[TEST] Posicion obtenida: %.6f, %.6f\n", lat, lon);
    } else {
        printf("[TEST] Error obteniendo posicion\n");
    }
}