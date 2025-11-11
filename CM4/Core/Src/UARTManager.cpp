#include "../../Modules/UART/UARTManager.h"

// Inicialización del singleton
UARTManager* UARTManager::instance = nullptr;

/**
 * @brief Constructor privado
 */
UARTManager::UARTManager() {
    // Los UARTBus se construyen automáticamente con configuración por defecto
}

/**
 * @brief Obtiene la instancia singleton del UARTManager
 */
UARTManager& UARTManager::getInstance() {
    if (instance == nullptr) {
        instance = new UARTManager();
    }
    return *instance;
}

/**
 * @brief Inicializa el bus UART1
 */
UARTResult UARTManager::initUART1(UART_HandleTypeDef* huart1) {
    if (huart1 == nullptr) {
        return UART_ERROR;
    }
    
    // Configuración específica para UART1 (GPS)
    UARTConfig config;
    config.mutexTimeout = 1000;  // 1 segundo
    config.retryDelay = 10;      // 10ms entre reintentos
    config.maxRetries = 3;       // Máximo 3 reintentos
    
    return uart1Bus.init(huart1, config);
}

/**
 * @brief Inicializa el bus UART2
 */
UARTResult UARTManager::initUART2(UART_HandleTypeDef* huart2) {
    if (huart2 == nullptr) {
        return UART_ERROR;
    }
    
    // Configuración específica para UART2
    UARTConfig config;
    config.mutexTimeout = 500;   // 500ms
    config.retryDelay = 5;       // 5ms entre reintentos
    config.maxRetries = 2;       // Máximo 2 reintentos
    
    return uart2Bus.init(huart2, config);
}