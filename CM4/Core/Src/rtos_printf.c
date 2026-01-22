/**
 ******************************************************************************
 * @file    rtos_printf.c
 * @brief   Thread-safe printf implementation for FreeRTOS applications
 * @author  TPP-IntelliFence Team
 * @date    2025
 ******************************************************************************
 */

#include "rtos_printf.h"
#include "cmsis_os.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Private Variables
 * ============================================================================ */

/**
 * @brief Static buffer for printf formatting
 * @note Using static buffer to avoid dynamic allocation
 */
static char printf_buffer[RTOS_PRINTF_BUFFER_SIZE];

/**
 * @brief Mutex handle for thread-safe printf operations
 */
static osMutexId_t printf_mutex = NULL;

/**
 * @brief Mutex attributes
 */
static const osMutexAttr_t printf_mutex_attr = {
    .name = "printf_mutex",
    .attr_bits = osMutexRecursive,  // Allow recursive locking from same task
    .cb_mem = NULL,
    .cb_size = 0U
};

/**
 * @brief Initialization flag
 */
static uint8_t printf_initialized = 0;

/**
 * @brief Scheduler running flag
 */
static volatile uint8_t scheduler_running = 0;

/* ============================================================================
 * Private Functions
 * ============================================================================ */

/**
 * @brief Internal function to write string to output (weak linkage for override)
 * @param str String to output
 * @param len Length of string
 * @retval Number of characters written
 * @note Override this function to redirect output to different peripheral
 */
__attribute__((weak)) int _rtos_printf_write(const char *str, size_t len)
{
    // Use shorter timeout to avoid WWDG issues during UART transmission
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, (uint8_t*)str, len, 100); // 100ms timeout instead of HAL_MAX_DELAY
    return (status == HAL_OK) ? len : 0;
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

/**
 * @brief Initialize the RTOS printf system
 */
int rtos_printf_init(void)
{
    if (printf_initialized) {
        return 0; // Already initialized
    }

    // Create mutex
    printf_mutex = osMutexNew(&printf_mutex_attr);
    
    if (printf_mutex == NULL) {
        return -1; // Failed to create mutex
    }

    printf_initialized = 1;
    
    // Mark scheduler as running (will be confirmed when osKernelStart is called)
    scheduler_running = 1;
    
    return 0;
}

/**
 * @brief Check if RTOS printf system is active
 */
int rtos_printf_is_active(void)
{
    // Check if initialized and scheduler is running
    return (printf_initialized && scheduler_running);
}

/**
 * @brief Thread-safe printf function
 */
int rtos_printf(const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = rtos_vprintf(format, args);
    va_end(args);

    return result;
}

/**
 * @brief Thread-safe vprintf function
 */
int rtos_vprintf(const char *format, va_list args)
{
    int result = -1;
    int formatted_length;

    // Check if initialized
    if (!printf_initialized || printf_mutex == NULL) {
        // If not initialized, use standard vprintf (not thread-safe)
        return vprintf(format, args);
    }

    // Acquire mutex with timeout
    osStatus_t status = osMutexAcquire(printf_mutex, RTOS_PRINTF_MUTEX_TIMEOUT);
    
    if (status != osOK) {
        // Mutex acquisition failed
        return -1;
    }

    // Format string into buffer
    formatted_length = vsnprintf(printf_buffer, RTOS_PRINTF_BUFFER_SIZE, format, args);

    if (formatted_length < 0) {
        // Formatting error
        result = -1;
    } else if (formatted_length >= RTOS_PRINTF_BUFFER_SIZE) {
        // Buffer overflow - output truncated message
        printf_buffer[RTOS_PRINTF_BUFFER_SIZE - 1] = '\0';
        result = _rtos_printf_write(printf_buffer, RTOS_PRINTF_BUFFER_SIZE - 1);
    } else {
        // Normal case - output formatted string
        result = _rtos_printf_write(printf_buffer, formatted_length);
    }

    // Release mutex
    osMutexRelease(printf_mutex);

    return result;
}

/**
 * @brief Smart printf that automatically routes to correct implementation
 */
int smart_printf(const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    
    // If RTOS is active, use thread-safe version
    if (rtos_printf_is_active()) {
        result = rtos_vprintf(format, args);
    } else {
        // Before RTOS starts, use standard vprintf
        result = vprintf(format, args);
    }
    
    va_end(args);
    return result;
}

/* ============================================================================
 * Optional: Direct UART output implementation
 * ============================================================================ */

#ifdef RTOS_PRINTF_USE_UART
#include "usart.h"  // Adjust to your UART header

/**
 * @brief Override the write function to use UART directly
 * @note Uncomment and configure for direct UART output
 */
int _rtos_printf_write(const char *str, size_t len)
{
    // Replace 'huart1' with your UART handle
    // HAL_UART_Transmit(&huart1, (uint8_t*)str, len, HAL_MAX_DELAY);
    
    // For now, use standard output
    return fwrite(str, 1, len, stdout);
}
#endif
