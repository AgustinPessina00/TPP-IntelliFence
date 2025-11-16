#include "UARTManager.h"
#include "usart.h"  // Para huart1

// ========== CONFIGURACIÓN OPTIMIZADA PARA EMBEDDED ==========

static UARTConfig getEmbeddedConfig() {
    UARTConfig config = UARTBus::getDefaultConfig();
    // Ajustar configuración para embedded (timeouts más cortos)
    config.maxRetries = 3;
    config.retryDelay = 5;      // 5ms - más rápido para MCU
    config.busTimeout = 500;    // 500ms - timeout más agresivo
    config.mutexTimeout = 2000; // 2s para mutex
    return config;
}

// ========== INICIALIZACIÓN DE MIEMBROS ESTÁTICOS ==========

bool UARTManager::initialized = false;
static UARTConfig embeddedConfig = getEmbeddedConfig();
UARTBus UARTManager::uart1(&huart1, &embeddedConfig);  // Construcción estática con configuración optimizada
uint32_t UARTManager::uart1TotalOps = 0;
uint32_t UARTManager::uart1SuccessOps = 0; 
uint32_t UARTManager::uart1ErrorCount = 0;

// ========== IMPLEMENTACIÓN DE UARTMANAGER ==========

UARTBus& UARTManager::getUart1() {
    return uart1;
}

bool UARTManager::initializeAll() {
    if (!initialized) {
        // uart1 ya tiene configuración optimizada desde su construcción estática
        
        // Inicializar bus UART1 con configuración optimizada
        if (uart1.initialize() == UART_OK) {
            initialized = true;
            
            // Resetear estadísticas
            uart1TotalOps = 0;
            uart1SuccessOps = 0;
            uart1ErrorCount = 0;
            
            return true;
        }
    }
    return initialized;
}

bool UARTManager::isInitialized() {
    return initialized;
}

bool UARTManager::reset() {
    initialized = false;
    
    // Intentar reinicializar
    return initializeAll();
}

void UARTManager::getUart1Stats(uint32_t& totalOperations, 
                               uint32_t& successfulOperations, 
                               uint32_t& errorCount) {
    totalOperations = uart1TotalOps;
    successfulOperations = uart1SuccessOps;
    errorCount = uart1ErrorCount;
}

void UARTManager::updateStats(bool success) {
    uart1TotalOps++;
    if (success) {
        uart1SuccessOps++;
    } else {
        uart1ErrorCount++;
    }
}

// ========== FUNCIONES DE CONVENIENCIA GLOBALES ==========

UARTBus& getUart1() {
    return UARTManager::getUart1();
}

bool initUartSystem() {
    return UARTManager::initializeAll();
}

bool isUartSystemReady() {
    return UARTManager::isInitialized();
}

// ========== INTERFAZ C ==========

extern "C" {

int uartSystemInit(void) {
    return initUartSystem() ? 1 : 0;
}

int uartSystemIsReady(void) {
    return isUartSystemReady() ? 1 : 0;
}

int uartReadRegister(uint16_t deviceAddr, uint16_t regAddr, 
                    uint8_t* data, uint16_t size) {
    if (!isUartSystemReady() || data == nullptr) {
        return 1; // ERROR
    }
    
    UARTBus& bus = getUart1();
    UARTResult result = bus.memRead(deviceAddr, regAddr, 
                                    1,  // memAddrSize de 1 byte (equivalente a UART_MEMADD_SIZE_8BIT)
                                    data, size, 1000);
    
    return static_cast<int>(result);
}

int uartWriteRegister(uint16_t deviceAddr, uint16_t regAddr, 
                     const uint8_t* data, uint16_t size) {
    if (!isUartSystemReady() || data == nullptr) {
        return 1; // ERROR
    }
    
    UARTBus& bus = getUart1();
    UARTResult result = bus.memWrite(deviceAddr, regAddr, 
                                     1,  // memAddrSize de 1 byte (equivalente a UART_MEMADD_SIZE_8BIT)
                                     data, size, 1000);
    
    return static_cast<int>(result);
}

void getUartStatsCWrapper(uint32_t* total, uint32_t* success, uint32_t* errors) {
    if (total && success && errors) {
        UARTManager::getUart1Stats(*total, *success, *errors);
    }
}

} // extern "C"
