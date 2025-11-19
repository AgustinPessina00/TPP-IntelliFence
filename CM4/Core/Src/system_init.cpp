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
    printf("[MANAGERS] Initializing C++ managers...\n");
    
    // Initialize I2C Manager
    if (!I2CManager::isInitialized()) {
        printf("[I2C] Initializing I2CManager...\n");
        if (!I2CManager::initializeAll()) {
            printf("[I2C] ERROR - Failed to initialize I2CManager\n");
            return -1;
        }
        printf("[I2C] OK - I2CManager initialized successfully\n");
    } else {
        printf("[I2C] Already initialized\n");
    }
    
    // Initialize UART Manager
    if (!UARTManager::isInitialized()) {
        printf("[UART] Initializing UARTManager...\n");
        if (!UARTManager::initializeAll()) {
            printf("[UART] ERROR - Failed to initialize UARTManager\n");
            return -1;
        }
        printf("[UART] OK - UARTManager initialized successfully\n");
    } else {
        printf("[UART] Already initialized\n");
    }
    
    printf("[MANAGERS] All C++ managers initialized successfully\n");
    return 0;
}

#ifdef __cplusplus
}
#endif
