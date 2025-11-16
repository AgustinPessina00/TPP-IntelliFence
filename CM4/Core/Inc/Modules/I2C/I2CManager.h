#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
}
#include "II2C.h"
#include "I2CBus.h"

/**
 * @brief Manager estático para instancias I2C - Sin allocación dinámica
 * 
 * Este manager proporciona acceso thread-safe a buses I2C usando el patrón
 * Singleton con instancias estáticas. Evita los problemas de fragmentación
 * y timing no determinístico de new/delete en microcontroladores.
 */
class I2CManager {
public:
    /**
     * @brief Obtiene referencia al bus I2C2 (instancia estática)
     * @return Referencia al bus I2C2
     */
    static I2CBus& getBus2();
    
    /**
     * @brief Inicializa todos los buses I2C disponibles
     * @return true si la inicialización fue exitosa
     */
    static bool initializeAll();
    
    /**
     * @brief Verifica si el sistema I2C está inicializado
     * @return true si está inicializado
     */
    static bool isInitialized();
    
    /**
     * @brief Reinicia el sistema I2C (para recovery)
     * @return true si el reinicio fue exitoso
     */
    static bool reset();
    
    /**
     * @brief Obtiene estadísticas de uso del bus I2C2
     * @param totalOperations Número total de operaciones realizadas
     * @param successfulOperations Número de operaciones exitosas
     * @param errorCount Número de errores encontrados
     */
    static void getBus2Stats(uint32_t& totalOperations, 
                            uint32_t& successfulOperations, 
                            uint32_t& errorCount);

private:
    static bool initialized;           ///< Estado de inicialización del sistema
    static I2CBus bus2;               ///< Instancia estática del bus I2C2
    
    // Estadísticas (útiles para debugging embedded)
    static uint32_t bus2TotalOps;      ///< Contador total de operaciones
    static uint32_t bus2SuccessOps;    ///< Contador de operaciones exitosas
    static uint32_t bus2ErrorCount;    ///< Contador de errores
    
    // Constructor privado (patrón Singleton)
    I2CManager() = delete;
    I2CManager(const I2CManager&) = delete;
    I2CManager& operator=(const I2CManager&) = delete;
    
    /**
     * @brief Incrementa contadores de estadísticas
     * @param success true si la operación fue exitosa
     */
    static void updateStats(bool success);
    
    friend class I2CBus; // Para que I2CBus pueda actualizar estadísticas
};

// ========== FUNCIONES DE CONVENIENCIA GLOBALES ==========

/**
 * @brief Función de conveniencia para obtener el bus I2C2
 * @return Referencia al bus I2C2
 */
I2CBus& getI2cBus2();

/**
 * @brief Función de conveniencia para inicializar el sistema I2C
 * @return true si la inicialización fue exitosa
 */
bool initI2cSystem();

/**
 * @brief Función de conveniencia para verificar inicialización
 * @return true si el sistema está inicializado
 */
bool isI2cSystemReady();

#endif // __cplusplus

// ========== INTERFAZ C ==========

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaz C para inicializar el sistema I2C
 * @return 1 si exitoso, 0 si falló
 */
int i2cSystemInit(void);

/**
 * @brief Interfaz C para verificar si está inicializado
 * @return 1 si inicializado, 0 si no
 */
int i2cSystemIsReady(void);

/**
 * @brief Interfaz C para operaciones básicas de lectura
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro
 * @param data Buffer para datos
 * @param size Cantidad de bytes a leer
 * @return 0=OK, 1=ERROR, 2=TIMEOUT, 3=BUSY, 4=NACK
 */
int i2cReadRegister(uint16_t deviceAddr, uint16_t regAddr, 
                   uint8_t* data, uint16_t size);

/**
 * @brief Interfaz C para operaciones básicas de escritura
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro
 * @param data Buffer con datos a escribir
 * @param size Cantidad de bytes a escribir
 * @return 0=OK, 1=ERROR, 2=TIMEOUT, 3=BUSY, 4=NACK
 */
int i2cWriteRegister(uint16_t deviceAddr, uint16_t regAddr, 
                    const uint8_t* data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif // I2C_MANAGER_H