#include "UARTBus.h"
#include <stdio.h>

/**
 * @brief Constructor por defecto
 */
UARTBus::UARTBus() 
    : huart(nullptr), busMutex(nullptr), initialized(false) {
    // Configuración por defecto
    config.mutexTimeout = 1000;  // 1 segundo
    config.retryDelay = 10;      // 10ms
    config.maxRetries = 3;       // 3 reintentos
}

/**
 * @brief Destructor
 */
UARTBus::~UARTBus() {
    if (busMutex != nullptr) {
        osMutexDelete(busMutex);
        busMutex = nullptr;
    }
    initialized = false;
}

/**
 * @brief Inicializa el bus UART con configuración thread-safe
 */
UARTResult UARTBus::init(UART_HandleTypeDef* huart, const UARTConfig& config) {
    if (huart == nullptr) {
        return UART_ERROR;
    }
    
    this->huart = huart;
    this->config = config;
    
    // Crear mutex para thread safety
    const osMutexAttr_t mutexAttr = {
        .name = "UARTBusMutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = nullptr,
        .cb_size = 0
    };
    
    busMutex = osMutexNew(&mutexAttr);
    if (busMutex == nullptr) {
        return UART_ERROR;
    }
    
    initialized = true;
    return UART_OK;
}

/**
 * @brief Convierte HAL_StatusTypeDef a UARTResult
 */
UARTResult UARTBus::halToUARTResult(HAL_StatusTypeDef halResult) {
    switch (halResult) {
        case HAL_OK:      return UART_OK;
        case HAL_ERROR:   return UART_ERROR;
        case HAL_BUSY:    return UART_BUSY;
        case HAL_TIMEOUT: return UART_TIMEOUT;
        default:          return UART_ERROR;
    }
}

/**
 * @brief Determina si se requiere recuperación del bus
 */
bool UARTBus::requiresBusRecovery(UARTResult result) {
    return (result == UART_ERROR || result == UART_BUSY);
}

/**
 * @brief Intenta recuperar el bus UART
 */
void UARTBus::recoverBus() {
    if (huart != nullptr) {
        // Intentar reinicializar el UART
        HAL_UART_DeInit(huart);
        osDelay(1); // Pequeña pausa
        HAL_UART_Init(huart);
    }
}

/**
 * @brief Transmite datos por UART de forma thread-safe
 */
UARTResult UARTBus::transmit(const uint8_t* pData, uint16_t size, uint32_t timeout) {
    if (!initialized || huart == nullptr || pData == nullptr) {
        return UART_NOT_INITIALIZED;
    }
    
    // Adquirir mutex para thread safety
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        HAL_StatusTypeDef halResult = HAL_UART_Transmit(
            huart,
            const_cast<uint8_t*>(pData),
            size,
            timeout
        );
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Lógica de retry y recovery del bus
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

/**
 * @brief Recibe datos por UART de forma thread-safe
 */
UARTResult UARTBus::receive(uint8_t* pData, uint16_t size, uint32_t timeout) {
    if (!initialized || huart == nullptr || pData == nullptr) {
        return UART_NOT_INITIALIZED;
    }
    
    // Adquirir mutex para thread safety
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        HAL_StatusTypeDef halResult = HAL_UART_Receive(
            huart,
            pData,
            size,
            timeout
        );
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Lógica de retry y recovery del bus
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

/**
 * @brief Transmite y luego recibe datos (útil para protocolos request/response)
 */
UARTResult UARTBus::transmitReceive(const uint8_t* pTxData, uint16_t txSize,
                                   uint8_t* pRxData, uint16_t rxSize, 
                                   uint32_t timeout) {
    if (!initialized || huart == nullptr || pTxData == nullptr || pRxData == nullptr) {
        return UART_NOT_INITIALIZED;
    }
    
    // Adquirir mutex para thread safety
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        // Primero transmitir
        HAL_StatusTypeDef halResult = HAL_UART_Transmit(
            huart,
            const_cast<uint8_t*>(pTxData),
            txSize,
            timeout
        );
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            // Si transmisión exitosa, intentar recepción
            halResult = HAL_UART_Receive(
                huart,
                pRxData,
                rxSize,
                timeout
            );
            
            result = halToUARTResult(halResult);
        }
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Lógica de retry y recovery del bus
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}