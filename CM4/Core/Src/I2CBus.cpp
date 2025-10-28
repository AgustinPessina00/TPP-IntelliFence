#include "I2CBus.h"
#include <string.h>

// Configuración por defecto
static const I2CBusConfig defaultConfig = {
    .maxRetries = 3,
    .retryDelay = 10,     // 10ms entre reintentos
    .busTimeout = 1000,   // 1 segundo timeout por operación
    .mutexTimeout = 5000  // 5 segundos para adquirir mutex
};

I2CBus::I2CBus(I2C_HandleTypeDef* hi2c, const I2CBusConfig* config) 
    : hi2c(hi2c), busMutex(nullptr), initialized(false) {
    
    if (config != nullptr) {
        this->config = *config;
    } else {
        this->config = defaultConfig;
    }
}

I2CBus::~I2CBus() {
    if (busMutex != nullptr) {
        osMutexDelete(busMutex);
    }
}

I2CResult I2CBus::initialize() {
    if (hi2c == nullptr) {
        return I2C_ERROR;
    }
    
    // Crear mutex para thread safety
    const osMutexAttr_t mutexAttr = {
        .name = "I2CBusMutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = nullptr,
        .cb_size = 0U
    };
    
    busMutex = osMutexNew(&mutexAttr);
    if (busMutex == nullptr) {
        return I2C_ERROR;
    }
    
    initialized = true;
    return I2C_OK;
}

bool I2CBus::recoverBus() {
    if (hi2c == nullptr) {
        return false;
    }
    
    // Intentar reset del peripheral I2C
    __HAL_I2C_DISABLE(hi2c);
    osDelay(10);  // Pequeño delay
    __HAL_I2C_ENABLE(hi2c);
    
    // Limpiar flags de error
    __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_BERR);
    __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_ARLO); 
    __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_OVR);
    
    // Verificar que el bus esté libre
    return (hi2c->State == HAL_I2C_STATE_READY);
}

I2CResult I2CBus::halToI2CResult(HAL_StatusTypeDef halStatus) {
    switch (halStatus) {
        case HAL_OK:
            return I2C_OK;
        case HAL_TIMEOUT:
            return I2C_TIMEOUT;
        case HAL_BUSY:
            return I2C_BUSY;
        case HAL_ERROR:
            // Verificar si es NACK específicamente
            if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_AF)) {
                __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);
                return I2C_NACK;
            }
            return I2C_ERROR;
        default:
            return I2C_ERROR;
    }
}

bool I2CBus::requiresBusRecovery(I2CResult error) {
    return (error == I2C_ERROR || error == I2C_BUSY);
}

I2CResult I2CBus::memRead(uint16_t deviceAddr, 
                         uint16_t memAddr, 
                         uint16_t memAddrSize,
                         uint8_t* pData, 
                         uint16_t size, 
                         uint32_t timeout) {
    
    if (!initialized || hi2c == nullptr || pData == nullptr) {
        return I2C_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return I2C_TIMEOUT;
    }
    
    I2CResult result = I2C_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        HAL_StatusTypeDef halResult = HAL_I2C_Mem_Read(
            hi2c, 
            deviceAddr << 1,  // HAL espera dirección de 8-bit
            memAddr, 
            memAddrSize, 
            pData, 
            size, 
            timeout
        );
        
        result = halToI2CResult(halResult);
        
        if (result == I2C_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
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

I2CResult I2CBus::memWrite(uint16_t deviceAddr, 
                          uint16_t memAddr, 
                          uint16_t memAddrSize,
                          const uint8_t* pData, 
                          uint16_t size, 
                          uint32_t timeout) {
    
    if (!initialized || hi2c == nullptr || pData == nullptr) {
        return I2C_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return I2C_TIMEOUT;
    }
    
    I2CResult result = I2C_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        HAL_StatusTypeDef halResult = HAL_I2C_Mem_Write(
            hi2c, 
            deviceAddr << 1,  // HAL espera dirección de 8-bit
            memAddr, 
            memAddrSize, 
            const_cast<uint8_t*>(pData),  // HAL no usa const
            size, 
            timeout
        );
        
        result = halToI2CResult(halResult);
        
        if (result == I2C_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
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

I2CResult I2CBus::writeRead(uint16_t deviceAddr, 
                           const uint8_t* pWriteData, 
                           uint16_t writeSize,
                           uint8_t* pReadData, 
                           uint16_t readSize, 
                           uint32_t timeout) {
    
    if (!initialized || hi2c == nullptr || 
        pWriteData == nullptr || pReadData == nullptr) {
        return I2C_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return I2C_TIMEOUT;
    }
    
    I2CResult result = I2C_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        // Primero escribir
        HAL_StatusTypeDef halResult = HAL_I2C_Master_Transmit(
            hi2c, 
            deviceAddr << 1,  // HAL espera dirección de 8-bit
            const_cast<uint8_t*>(pWriteData), 
            writeSize, 
            timeout
        );
        
        if (halResult == HAL_OK) {
            // Si la escritura fue exitosa, intentar lectura
            halResult = HAL_I2C_Master_Receive(
                hi2c, 
                deviceAddr << 1, 
                pReadData, 
                readSize, 
                timeout
            );
        }
        
        result = halToI2CResult(halResult);
        
        if (result == I2C_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
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

I2CResult I2CBus::transmit(uint16_t deviceAddr,
                          const uint8_t* pData,
                          uint16_t size,
                          uint32_t timeout) {
    
    if (!initialized || hi2c == nullptr || pData == nullptr) {
        return I2C_ERROR;
    }
    
    // Adquirir mutex
    if (osMutexAcquire(busMutex, config.mutexTimeout) != osOK) {
        return I2C_TIMEOUT;
    }
    
    I2CResult result = I2C_ERROR;
    uint8_t attempts = 0;
    
    while (attempts <= config.maxRetries) {
        HAL_StatusTypeDef halResult = HAL_I2C_Master_Transmit(
            hi2c, 
            deviceAddr << 1,  // HAL espera dirección de 8-bit
            const_cast<uint8_t*>(pData),  // HAL no usa const
            size, 
            timeout
        );
        
        result = halToI2CResult(halResult);
        
        if (result == I2C_OK) {
            break;  // Operación exitosa
        }
        
        attempts++;
        
        // Si no es el último intento y el error requiere recuperación
        if (attempts <= config.maxRetries && requiresBusRecovery(result)) {
            recoverBus();
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

I2CBusConfig I2CBus::getDefaultConfig() {
    return defaultConfig;
}