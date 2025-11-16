#ifndef MODULES_UART_UARTBUS_H_
#define MODULES_UART_UARTBUS_H_

#include "stm32wlxx_hal.h"
#include "cmsis_os2.h"

/**
 * @brief Códigos de resultado para operaciones UART
 */
enum UARTResult {
    UART_OK = 0,
    UART_ERROR,
    UART_BUSY,
    UART_TIMEOUT,
    UART_NOT_INITIALIZED
};

/**
 * @brief Configuración de UARTBus thread-safe
 */
typedef struct {
    uint32_t mutexTimeout;      // Timeout para adquisición de mutex (ms)
    uint32_t retryDelay;        // Delay entre reintentos (ms) 
    uint8_t maxRetries;         // Número máximo de reintentos
} UARTConfig;

/**
 * @brief Clase para manejo thread-safe de un bus UART específico
 * 
 * Esta clase encapsula un UART_HandleTypeDef de STM32 HAL y proporciona
 * acceso thread-safe usando mutex de FreeRTOS. Incluye manejo automático
 * de errores, reintentos y recuperación del bus.
 */
class UARTBus {
private:
    UART_HandleTypeDef* huart;          // Handle HAL del UART
    osMutexId_t busMutex;              // Mutex para thread safety
    UARTConfig config;                 // Configuración del bus
    bool initialized;                  // Estado de inicialización
    
    // Métodos internos de utilidad
    UARTResult halToUARTResult(HAL_StatusTypeDef halResult);
    bool requiresBusRecovery(UARTResult result);
    void recoverBus();

public:
    /**
     * @brief Constructor por defecto
     */
    UARTBus();
    
    /**
     * @brief Destructor
     */
    ~UARTBus();
    
    /**
     * @brief Inicializa el bus UART con configuración thread-safe
     * @param huart Handle HAL del UART 
     * @param config Configuración del bus
     * @return UARTResult código de resultado
     */
    UARTResult init(UART_HandleTypeDef* huart, const UARTConfig& config);
    
    /**
     * @brief Transmite datos por UART de forma thread-safe
     * @param pData Buffer con los datos a transmitir
     * @param size Cantidad de bytes a transmitir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult transmit(const uint8_t* pData, uint16_t size, uint32_t timeout);
    
    /**
     * @brief Recibe datos por UART de forma thread-safe
     * @param pData Buffer para almacenar los datos recibidos
     * @param size Cantidad de bytes a recibir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult receive(uint8_t* pData, uint16_t size, uint32_t timeout);
    
    /**
     * @brief Transmite y luego recibe datos (útil para protocolos request/response)
     * @param pTxData Buffer con los datos a transmitir
     * @param txSize Cantidad de bytes a transmitir
     * @param pRxData Buffer para almacenar los datos recibidos
     * @param rxSize Cantidad de bytes a recibir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    UARTResult transmitReceive(const uint8_t* pTxData, uint16_t txSize,
                              uint8_t* pRxData, uint16_t rxSize, 
                              uint32_t timeout);
    
    /**
     * @brief Limpia el buffer de recepción del UART
     */
    UARTResult flushRxBuffer() {
        if (!initialized || !huart) {
            return UART_NOT_INITIALIZED;
        }

        // Adquirir mutex para acceso exclusivo al bus
        if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
            return UART_BUSY;
        }

        // Abortar cualquier recepción en curso
        HAL_UART_AbortReceive(huart);
        
        // Limpiar los flags de error del UART
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF);   // Parity Error
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);   // Framing Error
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF);   // Noise Error
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF);  // Overrun Error
        
        // Leer datos residuales del registro de datos (si hay)
        volatile uint8_t dummy;
        while (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE)) {
            dummy = (uint8_t)(huart->Instance->RDR & 0xFF);
            (void)dummy;  // Evitar warning de variable no usada
        }

        // Liberar mutex
        osMutexRelease(busMutex);
        
        return UART_OK;
    }
    
    /**
     * @brief Verifica si el bus está inicializado
     * @return true si está inicializado, false en caso contrario
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief Obtiene la configuración actual del bus
     * @return Referencia a la configuración
     */
    const UARTConfig& getConfig() const { return config; }
};

#endif /* MODULES_UART_UARTBUS_H_ */