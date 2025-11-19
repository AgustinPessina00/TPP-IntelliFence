/**
 ******************************************************************************
 * @file    system_init.h
 * @brief   Header for C++ system initialization functions
 * @author  TPP-IntelliFence Team
 * @date    2025
 ******************************************************************************
 */

#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all C++ managers (I2C, UART, etc.)
 * @retval 0 on success, -1 on failure
 * @note Call this from main() after HAL initialization but before starting RTOS
 */
int initialize_cpp_managers(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_INIT_H */
