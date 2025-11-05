/**
 ******************************************************************************
 * @file    rtos_printf.h
 * @brief   Thread-safe printf implementation for FreeRTOS applications
 * @author  TPP-IntelliFence Team
 * @date    2025
 ******************************************************************************
 * @attention
 *
 * This module provides a thread-safe printf implementation using FreeRTOS
 * mutexes to prevent interleaved output when multiple tasks use printf.
 *
 * Features:
 * - Thread-safe printf with mutex protection
 * - Macro wrapper for easy migration (RTOS_PRINTF)
 * - Supports standard printf formatting
 * - Zero dynamic allocation (static buffer)
 *
 ******************************************************************************
 */

#ifndef RTOS_PRINTF_H
#define RTOS_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>

/* ============================================================================
 * Configuration
 * ============================================================================ */

/**
 * @brief Maximum length of a single printf message
 * @note Adjust based on your application needs and available RAM
 */
#define RTOS_PRINTF_BUFFER_SIZE     512

/**
 * @brief Mutex timeout in milliseconds
 * @note Maximum time to wait for the printf mutex
 */
#define RTOS_PRINTF_MUTEX_TIMEOUT   1000

/* ============================================================================
 * Public Functions
 * ============================================================================ */

/**
 * @brief Initialize the RTOS printf system
 * @note Must be called before using rtos_printf(), typically in main()
 *       before starting the FreeRTOS scheduler
 * @retval 0 if initialization successful, -1 otherwise
 */
int rtos_printf_init(void);

/**
 * @brief Thread-safe printf function
 * @param format Format string (same as standard printf)
 * @param ... Variable arguments
 * @retval Number of characters printed, or negative value on error
 * @note This function uses a mutex to ensure thread-safe operation
 */
int rtos_printf(const char *format, ...);

/**
 * @brief Thread-safe vprintf function (for variadic argument lists)
 * @param format Format string
 * @param args va_list of arguments
 * @retval Number of characters printed, or negative value on error
 */
int rtos_vprintf(const char *format, va_list args);

/**
 * @brief Check if RTOS printf system is initialized and scheduler is running
 * @retval 1 if RTOS is active, 0 otherwise
 */
int rtos_printf_is_active(void);

/**
 * @brief Smart printf that automatically switches between standard and RTOS printf
 * @param format Format string (same as standard printf)
 * @param ... Variable arguments
 * @retval Number of characters printed, or negative value on error
 * @note Uses standard printf before FreeRTOS starts, rtos_printf after
 */
int smart_printf(const char *format, ...);

/* ============================================================================
 * Convenience Macros
 * ============================================================================ */

/**
 * @brief Macro wrapper for automatic printf routing
 * @note Define RTOS_PRINTF_AUTO to automatically use smart_printf
 *       This will route to standard printf before RTOS starts,
 *       and to rtos_printf after scheduler is running
 */
#ifdef RTOS_PRINTF_AUTO
    #define printf(...) smart_printf(__VA_ARGS__)
#endif

/**
 * @brief Macro wrapper for forced RTOS printf (legacy compatibility)
 * @note Define RTOS_PRINTF_ENABLED to force all printf to use rtos_printf
 */
#ifdef RTOS_PRINTF_ENABLED
    #undef printf
    #define printf(...) rtos_printf(__VA_ARGS__)
#endif

/**
 * @brief Logging macros with severity levels (automatically route)
 */
#define RTOS_LOG_INFO(...)    smart_printf("[INFO] " __VA_ARGS__)
#define RTOS_LOG_WARN(...)    smart_printf("[WARN] " __VA_ARGS__)
#define RTOS_LOG_ERROR(...)   smart_printf("[ERROR] " __VA_ARGS__)
#define RTOS_LOG_DEBUG(...)   smart_printf("[DEBUG] " __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* RTOS_PRINTF_H */
