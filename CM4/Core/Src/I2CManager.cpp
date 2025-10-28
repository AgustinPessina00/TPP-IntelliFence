#include "I2CManager.h"
#include "i2c.h"  // Para hi2c2

// ========== CONFIGURACIÓN OPTIMIZADA PARA EMBEDDED ==========

static I2CBusConfig getEmbeddedConfig() {
    I2CBusConfig config = I2CBus::getDefaultConfig();
    // Ajustar configuración para embedded (timeouts más cortos)
    config.maxRetries = 3;
    config.retryDelay = 5;      // 5ms - más rápido para MCU
    config.busTimeout = 500;    // 500ms - timeout más agresivo
    config.mutexTimeout = 2000; // 2s para mutex
    return config;
}

// ========== INICIALIZACIÓN DE MIEMBROS ESTÁTICOS ==========

bool I2CManager::initialized = false;
static I2CBusConfig embeddedConfig = getEmbeddedConfig();
I2CBus I2CManager::bus2(&hi2c2, &embeddedConfig);  // Construcción estática con configuración optimizada
uint32_t I2CManager::bus2TotalOps = 0;
uint32_t I2CManager::bus2SuccessOps = 0; 
uint32_t I2CManager::bus2ErrorCount = 0;

// ========== IMPLEMENTACIÓN DE I2CMANAGER ==========

I2CBus& I2CManager::getBus2() {
    return bus2;
}

bool I2CManager::initializeAll() {
    if (!initialized) {
        // Bus2 ya tiene configuración optimizada desde su construcción estática
        
        // Inicializar bus I2C2 con configuración optimizada
        if (bus2.initialize() == I2C_OK) {
            initialized = true;
            
            // Resetear estadísticas
            bus2TotalOps = 0;
            bus2SuccessOps = 0;
            bus2ErrorCount = 0;
            
            return true;
        }
    }
    return initialized;
}

bool I2CManager::isInitialized() {
    return initialized;
}

bool I2CManager::reset() {
    initialized = false;
    
    // Intentar reinicializar
    return initializeAll();
}

void I2CManager::getBus2Stats(uint32_t& totalOperations, 
                             uint32_t& successfulOperations, 
                             uint32_t& errorCount) {
    totalOperations = bus2TotalOps;
    successfulOperations = bus2SuccessOps;
    errorCount = bus2ErrorCount;
}

void I2CManager::updateStats(bool success) {
    bus2TotalOps++;
    if (success) {
        bus2SuccessOps++;
    } else {
        bus2ErrorCount++;
    }
}

// ========== FUNCIONES DE CONVENIENCIA GLOBALES ==========

I2CBus& getI2cBus2() {
    return I2CManager::getBus2();
}

bool initI2cSystem() {
    return I2CManager::initializeAll();
}

bool isI2cSystemReady() {
    return I2CManager::isInitialized();
}

// ========== INTERFAZ C ==========

extern "C" {

int i2cSystemInit(void) {
    return initI2cSystem() ? 1 : 0;
}

int i2cSystemIsReady(void) {
    return isI2cSystemReady() ? 1 : 0;
}

int i2cReadRegister(uint16_t deviceAddr, uint16_t regAddr, 
                   uint8_t* data, uint16_t size) {
    if (!isI2cSystemReady() || data == nullptr) {
        return 1; // ERROR
    }
    
    I2CBus& bus = getI2cBus2();
    I2CResult result = bus.memRead(deviceAddr, regAddr, 
                                  I2C_MEMADD_SIZE_8BIT, 
                                  data, size, 1000);
    
    return static_cast<int>(result);
}

int i2cWriteRegister(uint16_t deviceAddr, uint16_t regAddr, 
                    const uint8_t* data, uint16_t size) {
    if (!isI2cSystemReady() || data == nullptr) {
        return 1; // ERROR
    }
    
    I2CBus& bus = getI2cBus2();
    I2CResult result = bus.memWrite(deviceAddr, regAddr, 
                                   I2C_MEMADD_SIZE_8BIT, 
                                   data, size, 1000);
    
    return static_cast<int>(result);
}

void getI2cStatsCWrapper(uint32_t* total, uint32_t* success, uint32_t* errors) {
    if (total && success && errors) {
        I2CManager::getBus2Stats(*total, *success, *errors);
    }
}

} // extern "C"