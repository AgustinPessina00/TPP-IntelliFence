/**
 ******************************************************************************
 * @file    system_init.cpp
 * @brief   C++ system initialization functions callable from C
 * @author  TPP-IntelliFence Team
 * @date    2025
 ******************************************************************************
 */

#include "I2CManager.h"
#include "UARTManager.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all C++ managers (I2C, UART, etc.)
 * @retval 0 on success, -1 on failure
 * @note This function must be called from main() before using any sensors
 */
int initialize_cpp_managers(void) {
    printf("[MANAGERS] Initializing C++ managers...\r\n");
    
    // Initialize I2C Manager
    if (!I2CManager::isInitialized()) {
        printf("[I2C] Initializing I2CManager...\r\n");
        if (!I2CManager::initializeAll()) {
            printf("[I2C] ERROR - Failed to initialize I2CManager\r\n");
            return -1;
        }
        printf("[I2C] OK - I2CManager initialized successfully\r\n");
    } else {
        printf("[I2C] Already initialized\r\n");
    }
    
    // Initialize UART Manager
    if (!UARTManager::isInitialized()) {
        printf("[UART] Initializing UARTManager...\r\n");
        if (!UARTManager::initializeAll()) {
            printf("[UART] ERROR - Failed to initialize UARTManager\r\n");
            return -1;
        }
        printf("[UART] OK - UARTManager initialized successfully\r\n");
    } else {
        printf("[UART] Already initialized\r\n");
    }
    
    printf("[MANAGERS] All C++ managers initialized successfully\r\n");
    return 0;
}

#ifdef __cplusplus
}
#endif
