#ifndef MODULES_UART_UARTMANAGER_H_
#define MODULES_UART_UARTMANAGER_H_

#include "UARTBus.h"
#include "stm32wlxx_hal.h"

/**
 * @brief Singleton Manager para múltiples buses UART thread-safe
 * 
 * Esta clase gestiona múltiples instancias de UARTBus de forma centralizada,
 * proporcionando acceso thread-safe a diferentes puertos UART del sistema.
 * Sigue el patrón Singleton para garantizar una única instancia global.
 */
class UARTManager {
private:
    static UARTManager* instance;
    
    UARTBus uart1Bus;    // Bus UART1
    UARTBus uart2Bus;    // Bus UART2 (si es necesario)
    
    // Constructor privado para Singleton
    UARTManager();
    
    // Prevenir copia y asignación
    UARTManager(const UARTManager&) = delete;
    UARTManager& operator=(const UARTManager&) = delete;

public:
    /**
     * @brief Obtiene la instancia singleton del UARTManager
     * @return Referencia a la instancia única
     */
    static UARTManager& getInstance();
    
    /**
     * @brief Inicializa el bus UART1
     * @param huart1 Handle HAL del UART1
     * @return UARTResult código de resultado
     */
    UARTResult initUART1(UART_HandleTypeDef* huart1);
    
    /**
     * @brief Inicializa el bus UART2
     * @param huart2 Handle HAL del UART2
     * @return UARTResult código de resultado
     */
    UARTResult initUART2(UART_HandleTypeDef* huart2);
    
    /**
     * @brief Obtiene referencia al bus UART1 thread-safe
     * @return Referencia al UARTBus del UART1
     */
    UARTBus& getUART1() { return uart1Bus; }
    
    /**
     * @brief Obtiene referencia al bus UART2 thread-safe
     * @return Referencia al UARTBus del UART2
     */
    UARTBus& getUART2() { return uart2Bus; }
    
    /**
     * @brief Verifica si UART1 está inicializado
     * @return true si está inicializado, false en caso contrario
     */
    bool isUART1Ready() const { return uart1Bus.isInitialized(); }
    
    /**
     * @brief Verifica si UART2 está inicializado
     * @return true si está inicializado, false en caso contrario
     */
    bool isUART2Ready() const { return uart2Bus.isInitialized(); }

    
};

#endif /* MODULES_UART_UARTMANAGER_H_ */