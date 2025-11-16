#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
}
#include "IUART.h"
#include "UARTBus.h"

/**
 * @brief Manager estático para instancias UART - Sin allocación dinámica
 * 
 * Este manager proporciona acceso thread-safe a buses UART usando el patrón
 * Singleton con instancias estáticas. Evita los problemas de fragmentación
 * y timing no determinístico de new/delete en microcontroladores.
 */
class UARTManager {
public:
    /**
     * @brief Obtiene referencia al bus UART1 (instancia estática)
     * @return Referencia al bus UART1
     */
    static UARTBus& getUart1();
    
    /**
     * @brief Inicializa todos los buses UART disponibles
     * @return true si la inicialización fue exitosa
     */
    static bool initializeAll();
    
    /**
     * @brief Verifica si el sistema UART está inicializado
     * @return true si está inicializado
     */
    static bool isInitialized();
    
    /**
     * @brief Reinicia el sistema UART (para recovery)
     * @return true si el reinicio fue exitoso
     */
    static bool reset();
    
    /**
     * @brief Obtiene estadísticas de uso del bus UART1
     * @param totalOperations Número total de operaciones realizadas
     * @param successfulOperations Número de operaciones exitosas
     * @param errorCount Número de errores encontrados
     */
    static void getUart1Stats(uint32_t& totalOperations, 
                            uint32_t& successfulOperations, 
                            uint32_t& errorCount);

private:
    static bool initialized;           ///< Estado de inicialización del sistema
    static UARTBus uart1;               ///< Instancia estática del bus UART1
    
    // Estadísticas (útiles para debugging embedded)
    static uint32_t uart1TotalOps;      ///< Contador total de operaciones
    static uint32_t uart1SuccessOps;    ///< Contador de operaciones exitosas
    static uint32_t uart1ErrorCount;    ///< Contador de errores
    
    // Constructor privado (patrón Singleton)
    UARTManager() = delete;
    UARTManager(const UARTManager&) = delete;
    UARTManager& operator=(const UARTManager&) = delete;
    
    /**
     * @brief Incrementa contadores de estadísticas
     * @param success true si la operación fue exitosa
     */
    static void updateStats(bool success);
    
    friend class UARTBus; // Para que UARTBus pueda actualizar estadísticas
};

// ========== FUNCIONES DE CONVENIENCIA GLOBALES ==========

/**
 * @brief Función de conveniencia para obtener el bus UART1
 * @return Referencia al bus UART1
 */
UARTBus& getUart1();

/**
 * @brief Función de conveniencia para inicializar el sistema UART
 * @return true si la inicialización fue exitosa
 */
bool initUartSystem();

/**
 * @brief Función de conveniencia para verificar inicialización
 * @return true si el sistema está inicializado
 */
bool isUartSystemReady();

#endif // __cplusplus

// ========== INTERFAZ C ==========

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaz C para inicializar el sistema UART
 * @return 1 si exitoso, 0 si falló
 */
int uartSystemInit(void);

/**
 * @brief Interfaz C para verificar si está inicializado
 * @return 1 si inicializado, 0 si no
 */
int uartSystemIsReady(void);

/**
 * @brief Interfaz C para operaciones básicas de lectura
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro
 * @param data Buffer para datos
 * @param size Cantidad de bytes a leer
 * @return 0=OK, 1=ERROR, 2=TIMEOUT, 3=BUSY, 4=NACK
 */
int uartReadRegister(uint16_t deviceAddr, uint16_t regAddr, 
                   uint8_t* data, uint16_t size);

/**
 * @brief Interfaz C para operaciones básicas de escritura
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro
 * @param data Buffer con datos a escribir
 * @param size Cantidad de bytes a escribir
 * @return 0=OK, 1=ERROR, 2=TIMEOUT, 3=BUSY, 4=NACK
 */
int uartWriteRegister(uint16_t deviceAddr, uint16_t regAddr, 
                    const uint8_t* data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif // UART_MANAGER_H