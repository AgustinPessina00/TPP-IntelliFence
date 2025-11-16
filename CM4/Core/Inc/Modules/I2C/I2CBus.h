#ifndef I2C_BUS_H
#define I2C_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
}
#include "II2C.h"
#include "cmsis_os.h"
#include "i2c.h"

/**
 * @brief Configuración de parámetros para el bus I2C
 */
struct I2CBusConfig {
    uint8_t maxRetries;      ///< Número máximo de reintentos por operación
    uint32_t retryDelay;     ///< Delay entre reintentos en ms
    uint32_t busTimeout;     ///< Timeout para operaciones individuales en ms
    uint32_t mutexTimeout;   ///< Timeout para adquirir el mutex en ms
};

/**
 * @brief Wrapper thread-safe para bus I2C con mutex, reintentos y recuperación
 * 
 * Esta clase implementa la interfaz II2C proporcionando:
 * - Thread safety mediante mutex FreeRTOS
 * - Lógica de reintentos automáticos
 * - Recuperación del bus en caso de error
 * - Detección y manejo de estados de error
 */
class I2CBus : public II2C {
private:
    I2C_HandleTypeDef* hi2c;        ///< Handle del peripheral I2C de HAL
    osMutexId_t busMutex;          ///< Mutex para thread safety
    I2CBusConfig config;           ///< Configuración del bus
    bool initialized;              ///< Estado de inicialización
    
    /**
     * @brief Intenta recuperar el bus I2C en caso de error
     * @return true si la recuperación fue exitosa
     */
    bool recoverBus();
    
    /**
     * @brief Convierte códigos de error HAL a I2CResult
     * @param halStatus Estado retornado por HAL
     * @return I2CResult equivalente
     */
    I2CResult halToI2CResult(HAL_StatusTypeDef halStatus);
    
    /**
     * @brief Verifica si el error requiere recuperación del bus
     * @param error Código de error a verificar
     * @return true si requiere recuperación
     */
    bool requiresBusRecovery(I2CResult error);

public:
    /**
     * @brief Constructor de I2CBus
     * @param hi2c Handle del peripheral I2C
     * @param config Configuración del bus (opcional, usa defaults si es nullptr)
     */
    explicit I2CBus(I2C_HandleTypeDef* hi2c, const I2CBusConfig* config = nullptr);
    
    /**
     * @brief Destructor
     */
    ~I2CBus();
    
    /**
     * @brief Inicializa el bus I2C y crea el mutex
     * @return I2CResult código de resultado
     */
    I2CResult initialize();
    
    /**
     * @brief Verifica si el bus está inicializado correctamente
     * @return true si está inicializado
     */
    bool isInitialized() const { return initialized; }
    
    // Implementación de la interfaz II2C
    I2CResult memRead(uint16_t deviceAddr, 
                     uint16_t memAddr, 
                     uint16_t memAddrSize,
                     uint8_t* pData, 
                     uint16_t size, 
                     uint32_t timeout) override;
                     
    I2CResult memWrite(uint16_t deviceAddr, 
                      uint16_t memAddr, 
                      uint16_t memAddrSize,
                      const uint8_t* pData, 
                      uint16_t size, 
                      uint32_t timeout) override;
                      
    I2CResult writeRead(uint16_t deviceAddr, 
                       const uint8_t* pWriteData, 
                       uint16_t writeSize,
                       uint8_t* pReadData, 
                       uint16_t readSize, 
                       uint32_t timeout) override;
    
    /**
     * @brief Transmite datos directamente sin usar registros (para dispositivos como GPS)
     * @param deviceAddr Dirección 7-bit del dispositivo
     * @param pData Buffer con los datos a transmitir
     * @param size Cantidad de bytes a transmitir
     * @param timeout Timeout en milisegundos
     * @return I2CResult código de resultado
     */
    I2CResult transmit(uint16_t deviceAddr,
                      const uint8_t* pData,
                      uint16_t size,
                      uint32_t timeout);
    
    /**
     * @brief Obtiene configuración predeterminada para el bus
     * @return Configuración con valores por defecto
     */
    static I2CBusConfig getDefaultConfig();
};

#endif // __cplusplus

#endif // I2C_BUS_H