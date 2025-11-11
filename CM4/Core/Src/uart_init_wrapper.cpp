#include "../Inc/uart_init_wrapper.h"
#include "../../Modules/UART/UARTManager.h"

extern "C" {

/**
 * @brief Inicializa el UARTManager y configura UART1 para GPS
 */
int init_uart_manager_uart1(UART_HandleTypeDef* huart1) {
    if (huart1 == nullptr) {
        return -1;
    }
    
    UARTResult result = UARTManager::getInstance().initUART1(huart1);
    
    return (result == UART_OK) ? 0 : -1;
}

/**
 * @brief Verifica si UART1 está inicializado correctamente
 */
int is_uart1_ready(void) {
    return UARTManager::getInstance().isUART1Ready() ? 1 : 0;
}

} // extern "C"