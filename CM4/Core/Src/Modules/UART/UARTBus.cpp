#include "UARTBus.h"
#include <string.h>

// Declaración externa de funciones de buffer circular GPS
extern "C" {
    uint16_t GPS_GetAvailableBytes(uint8_t* pData, uint16_t maxSize);
}

// Configuración por defecto
static const UARTConfig defaultConfig = {
    .maxRetries = 3,
    .retryDelay = 10,     // 10ms entre reintentos
    .busTimeout = 1000,   // 1 segundo timeout por operación
    .mutexTimeout = 5000  // 5 segundos para adquirir mutex
};

UARTBus::UARTBus(UART_HandleTypeDef* huart, const UARTConfig* config) 
    : huart(huart), busMutex(nullptr), initialized(false) {
    
    if (config != nullptr) {
        this->config = *config;
    } else {
        this->config = defaultConfig;
    }
}

UARTBus::~UARTBus() {
    if (busMutex != nullptr) {
        osMutexDelete(busMutex);
    }
}

UARTResult UARTBus::initialize() {
    if (huart == nullptr) {
        return UART_ERROR;
    }
    
    // Crear mutex para thread safety
    const osMutexAttr_t mutexAttr = {
        .name = "UARTBusMutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = nullptr,
        .cb_size = 0U
    };
    
    busMutex = osMutexNew(&mutexAttr);
    if (busMutex == nullptr) {
        return UART_ERROR;
    }
    
    initialized = true;
    return UART_OK;
}

bool UARTBus::recoverComm() {
    if (huart == nullptr) {
        return false;
    }
    
    // Intentar reset del peripheral UART
    __HAL_UART_DISABLE(huart);
    osDelay(10);  // Pequeño delay
    __HAL_UART_ENABLE(huart);
    
    // Limpiar flags de error
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF);
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF);
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF);
    
    // Verificar que el UART esté listo
    return (huart->gState == HAL_UART_STATE_READY);
}

UARTResult UARTBus::halToUARTResult(HAL_StatusTypeDef halStatus) {
    switch (halStatus) {
        case HAL_OK:
            return UART_OK;
        case HAL_TIMEOUT:
            return UART_TIMEOUT;
        case HAL_BUSY:
            return UART_BUSY;
        case HAL_ERROR:
            // Verificar tipos específicos de error
            if (huart->ErrorCode & HAL_UART_ERROR_PE) {
                return UART_ERROR;  // Parity error
            }
            if (huart->ErrorCode & HAL_UART_ERROR_FE) {
                return UART_ERROR;  // Frame error
            }
            if (huart->ErrorCode & HAL_UART_ERROR_NE) {
                return UART_ERROR;  // Noise error
            }
            if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
                return UART_ERROR;  // Overrun error
            }
            return UART_ERROR;
        default:
            return UART_ERROR;
    }
}

bool UARTBus::requiresCommRecovery(UARTResult error) {
    return (error == UART_ERROR || error == UART_BUSY);
}

UARTResult UARTBus::memRead(uint16_t deviceAddr, 
                           uint16_t memAddr, 
                           uint16_t memAddrSize,
                           uint8_t* pData, 
                           uint16_t size, 
                           uint32_t timeout) {
    
    if (!initialized || huart == nullptr || pData == nullptr) {
        return UART_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        // Para UART, primero enviamos la dirección del registro
        uint8_t addrBuffer[2];
        uint8_t addrSize = 0;
        
        if (memAddrSize == 1) {
            addrBuffer[0] = (uint8_t)(memAddr & 0xFF);
            addrSize = 1;
        } else if (memAddrSize == 2) {
            addrBuffer[0] = (uint8_t)((memAddr >> 8) & 0xFF);
            addrBuffer[1] = (uint8_t)(memAddr & 0xFF);
            addrSize = 2;
        }
        
        // Transmitir dirección del registro
        HAL_StatusTypeDef halResult = HAL_UART_Transmit(
            huart, 
            addrBuffer, 
            addrSize, 
            timeout
        );
        
        if (halResult == HAL_OK) {
            // Si la escritura fue exitosa, intentar lectura
            halResult = HAL_UART_Receive(
                huart, 
                pData, 
                size, 
                timeout
            );
        }
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresCommRecovery(result)) {
            recoverComm();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            // Delay simple para otros tipos de error
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

UARTResult UARTBus::memWrite(uint16_t deviceAddr, 
                            uint16_t memAddr, 
                            uint16_t memAddrSize,
                            const uint8_t* pData, 
                            uint16_t size, 
                            uint32_t timeout) {
    
    if (!initialized || huart == nullptr || pData == nullptr) {
        return UART_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        // Para UART, construimos un buffer con dirección + datos
        uint8_t txBuffer[256];  // Buffer temporal
        uint8_t addrSize = 0;
        
        if (memAddrSize == 1) {
            txBuffer[0] = (uint8_t)(memAddr & 0xFF);
            addrSize = 1;
        } else if (memAddrSize == 2) {
            txBuffer[0] = (uint8_t)((memAddr >> 8) & 0xFF);
            txBuffer[1] = (uint8_t)(memAddr & 0xFF);
            addrSize = 2;
        }
        
        // Copiar datos después de la dirección
        memcpy(&txBuffer[addrSize], pData, size);
        
        // Transmitir dirección + datos
        HAL_StatusTypeDef halResult = HAL_UART_Transmit(
            huart, 
            txBuffer, 
            addrSize + size, 
            timeout
        );
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresCommRecovery(result)) {
            recoverComm();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            // Delay simple para otros tipos de error
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

UARTResult UARTBus::writeRead(uint16_t deviceAddr, 
                             const uint8_t* pWriteData, 
                             uint16_t writeSize,
                             uint8_t* pReadData, 
                             uint16_t readSize, 
                             uint32_t timeout) {
    
    if (!initialized || huart == nullptr || 
        pWriteData == nullptr || pReadData == nullptr) {
        return UART_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        // Primero escribir
        HAL_StatusTypeDef halResult = HAL_UART_Transmit(
            huart, 
            const_cast<uint8_t*>(pWriteData), 
            writeSize, 
            timeout
        );
        
        if (halResult == HAL_OK) {
            // Si la escritura fue exitosa, intentar lectura
            halResult = HAL_UART_Receive(
                huart, 
                pReadData, 
                readSize, 
                timeout
            );
        }
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresCommRecovery(result)) {
            recoverComm();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            // Delay simple para otros tipos de error
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

/**
 * @brief Transmite datos por UART de forma thread-safe
 */


UARTResult UARTBus::transmit(const uint8_t* pData, uint16_t size, uint32_t timeout) {
    
    if (!initialized || huart == nullptr || pData == nullptr) {
        return UART_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        HAL_StatusTypeDef halResult = HAL_UART_Transmit(
            huart, 
            const_cast<uint8_t*>(pData),  // HAL no usa const
            size, 
            timeout
        );
        
        result = halToUARTResult(halResult);
        
        if (result == UART_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresCommRecovery(result)) {
            recoverComm();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            // Delay simple para otros tipos de error
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

UARTResult UARTBus::receive(uint8_t* pData,
                           uint16_t size,
                           uint32_t timeout) {
    
    if (!initialized || huart == nullptr || pData == nullptr) {
        return UART_ERROR;
    }
    
    // Adquirir mutex
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
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresCommRecovery(result)) {
            recoverComm();
            osDelay(config.retryDelay);
        } else if (attempts <= config.maxRetries) {
            // Delay simple para otros tipos de error
            osDelay(config.retryDelay);
        }
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

UARTResult UARTBus::receiveAvailable(uint8_t* pData,
                                    uint16_t maxSize,
                                    uint16_t* bytesReceived,
                                    uint32_t timeout) {
    
    if (!initialized || huart == nullptr || pData == nullptr || bytesReceived == nullptr) {
        return UART_ERROR;
    }
    
    *bytesReceived = 0;
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_OK;
    
    // Si es USART1 (GPS), usar buffer circular con interrupciones
    if (huart->Instance == USART1) {
        uint32_t startTime = HAL_GetTick();
        
        // Esperar a que haya al menos 1 byte disponible o timeout
        while (*bytesReceived == 0 && (HAL_GetTick() - startTime) < timeout) {
            *bytesReceived = GPS_GetAvailableBytes(pData, maxSize);
            if (*bytesReceived == 0) {
                osDelay(1);  // Pequeño delay antes de reintentar
            }
        }
        
        if (*bytesReceived == 0) {
            result = UART_TIMEOUT;
        }
    } 
    // Para otros UARTs, usar modo polling tradicional
    else {
        uint32_t startTime = HAL_GetTick();
        uint16_t index = 0;
        
        // Esperar al menos un byte con timeout
        while (index == 0 && (HAL_GetTick() - startTime) < timeout) {
            // Intentar recibir 1 byte sin bloqueo (timeout muy corto)
            HAL_StatusTypeDef halResult = HAL_UART_Receive(huart, &pData[index], 1, 1);
            
            if (halResult == HAL_OK) {
                index++;
                break;  // Recibimos al menos 1 byte
            } else if (halResult == HAL_TIMEOUT) {
                osDelay(1);  // Pequeño delay antes de reintentar
            } else {
                result = halToUARTResult(halResult);
                osMutexRelease(busMutex);
                return result;
            }
        }
        
        if (index == 0) {
            // No se recibió ningún byte en el timeout
            osMutexRelease(busMutex);
            return UART_TIMEOUT;
        }
        
        // Recibir bytes adicionales mientras estén disponibles
        while (index < maxSize) {
            HAL_StatusTypeDef halResult = HAL_UART_Receive(huart, &pData[index], 1, 10);
            
            if (halResult == HAL_OK) {
                index++;
            } else if (halResult == HAL_TIMEOUT) {
                // No hay más datos disponibles
                break;
            } else {
                // Error real
                result = halToUARTResult(halResult);
                break;
            }
        }
        
        *bytesReceived = index;
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

UARTResult UARTBus::receiveUntil(uint8_t* pData,
                                 uint16_t maxSize,
                                 uint8_t delimiter,
                                 uint16_t* bytesReceived,
                                 uint32_t timeout) {
    
    if (!initialized || huart == nullptr || pData == nullptr || bytesReceived == nullptr) {
        return UART_ERROR;
    }
    
    *bytesReceived = 0;
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return UART_TIMEOUT;
    }
    
    UARTResult result = UART_OK;
    uint32_t startTime = HAL_GetTick();
    uint16_t index = 0;
    
    // Recibir bytes hasta encontrar el delimitador o llenar el buffer
    while (index < maxSize && (HAL_GetTick() - startTime) < timeout) {
        HAL_StatusTypeDef halResult = HAL_UART_Receive(huart, &pData[index], 1, 50);
        
        if (halResult == HAL_OK) {
            if (pData[index] == delimiter) {
                index++;
                break;  // Encontramos el delimitador
            }
            index++;
        } else if (halResult == HAL_TIMEOUT) {
            osDelay(1);  // Pequeño delay antes de reintentar
        } else {
            result = halToUARTResult(halResult);
            break;
        }
    }
    
    *bytesReceived = index;
    
    // Si no recibimos nada, es un timeout
    if (index == 0) {
        result = UART_TIMEOUT;
    }
    
    // Liberar mutex
    osMutexRelease(busMutex);
    
    return result;
}

UARTConfig UARTBus::getDefaultConfig() {
    return defaultConfig;
}
