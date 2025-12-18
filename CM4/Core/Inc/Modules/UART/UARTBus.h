#ifndef UART_BUS_H
#define UART_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
}
#include "IUART.h"
#include "cmsis_os.h"
#include "usart.h"

/**
 * @brief Configuración de parámetros para el bus UART
 */
struct UARTConfig {
    uint8_t maxRetries;      ///< Número máximo de reintentos por operación
    uint32_t retryDelay;     ///< Delay entre reintentos en ms
    uint32_t busTimeout;     ///< Timeout para operaciones individuales en ms
    uint32_t mutexTimeout;   ///< Timeout para adquirir el mutex en ms
};

/**
 * @brief Wrapper thread-safe para bus UART con mutex, reintentos y recuperación
 * 
 * Esta clase implementa comunicación UART proporcionando:
 * - Thread safety mediante mutex FreeRTOS
 * - Lógica de reintentos automáticos
 * - Recuperación del bus en caso de error
 * - Detección y manejo de estados de error
 */
class UARTBus : public IUART {
private:
    UART_HandleTypeDef* huart;        ///< Handle del peripheral UART de HAL
    osMutexId_t busMutex;             ///< Mutex para thread safety
    UARTConfig config;                ///< Configuración del bus
    bool initialized;                 ///< Estado de inicialización
    
    /**
     * @brief Intenta recuperar el bus UART en caso de error
     * @return true si la recuperación fue exitosa
     */
    bool recoverComm();
    
    /**
     * @brief Convierte códigos de error HAL a UARTResult
     * @param halStatus Estado retornado por HAL
     * @return UARTResult equivalente
     */
    UARTResult halToUARTResult(HAL_StatusTypeDef halStatus);
    
    /**
     * @brief Verifica si el error requiere recuperación del bus
     * @param error Código de error a verificar
     * @return true si requiere recuperación
     */
    bool requiresCommRecovery(UARTResult error);

public:
    /**
     * @brief Constructor de UARTBus
     * @param huart Handle del peripheral UART
     * @param config Configuración del bus (opcional, usa defaults si es nullptr)
     */
    explicit UARTBus(UART_HandleTypeDef* huart, const UARTConfig* config = nullptr);
    
    /**
     * @brief Destructor
     */
    ~UARTBus();
    
    /**
     * @brief Inicializa el bus UART y crea el mutex
     * @return UARTResult código de resultado
     */
    UARTResult initialize();
    
    /**
     * @brief Verifica si el bus está inicializado correctamente
     * @return true si está inicializado
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief Lee datos de un registro de memoria del dispositivo UART
     * @param deviceAddr Dirección del dispositivo
     * @param memAddr Dirección del registro a leer
     * @param memAddrSize Tamaño de la dirección de memoria
     * @param pData Buffer donde almacenar los datos leídos
     * @param size Cantidad de bytes a leer
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult memRead(uint16_t deviceAddr, 
                     uint16_t memAddr, 
                     uint16_t memAddrSize,
                     uint8_t* pData, 
                     uint16_t size, 
                     uint32_t timeout) override;
                     
    /**
     * @brief Escribe datos a un registro de memoria del dispositivo UART
     * @param deviceAddr Dirección del dispositivo
     * @param memAddr Dirección del registro a escribir
     * @param memAddrSize Tamaño de la dirección de memoria
     * @param pData Buffer con los datos a escribir
     * @param size Cantidad de bytes a escribir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult memWrite(uint16_t deviceAddr, 
                      uint16_t memAddr, 
                      uint16_t memAddrSize,
                      const uint8_t* pData, 
                      uint16_t size, 
                      uint32_t timeout) override;
                      
    /**
     * @brief Realiza una operación de escritura seguida de lectura
     * @param deviceAddr Dirección del dispositivo
     * @param pWriteData Buffer con los datos a escribir
     * @param writeSize Cantidad de bytes a escribir
     * @param pReadData Buffer donde almacenar los datos leídos
     * @param readSize Cantidad de bytes a leer
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult writeRead(uint16_t deviceAddr, 
                       const uint8_t* pWriteData, 
                       uint16_t writeSize,
                       uint8_t* pReadData, 
                       uint16_t readSize, 
                       uint32_t timeout) override;
    
    /**
     * @brief Transmite datos por UART (para dispositivos de comunicación serial como GPS)
     * @param pData Buffer con los datos a transmitir
     * @param size Cantidad de bytes a transmitir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult transmit(const uint8_t* pData, uint16_t size, uint32_t timeout);
    
    /**
     * @brief Recibe datos por UART
     * @param pData Buffer donde almacenar los datos recibidos
     * @param size Cantidad de bytes a recibir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult receive(uint8_t* pData,
                      uint16_t size,
                      uint32_t timeout);
    
    /**
     * @brief Recibe datos disponibles en el buffer UART sin esperar a llenarlo completamente
     * @param pData Buffer donde almacenar los datos recibidos
     * @param maxSize Tamaño máximo del buffer
     * @param bytesReceived Cantidad de bytes realmente recibidos
     * @param timeout Timeout en milisegundos para esperar al menos 1 byte
     * @return UARTResult código de resultado
     */
    UARTResult receiveAvailable(uint8_t* pData,
                               uint16_t maxSize,
                               uint16_t* bytesReceived,
                               uint32_t timeout);
    
    /**
     * @brief Recibe datos hasta encontrar un byte delimitador o timeout
     * @param pData Buffer donde almacenar los datos recibidos
     * @param maxSize Tamaño máximo del buffer
     * @param delimiter Byte delimitador que marca el fin de la recepción
     * @param bytesReceived Cantidad de bytes realmente recibidos
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult receiveUntil(uint8_t* pData,
                           uint16_t maxSize,
                           uint8_t delimiter,
                           uint16_t* bytesReceived,
                           uint32_t timeout);
    
    /**
     * @brief Obtiene configuración predeterminada para el bus UART
     * @return Configuración con valores por defecto
     */
    static UARTConfig getDefaultConfig();
};

#endif // __cplusplus

#endif // UART_BUS_H