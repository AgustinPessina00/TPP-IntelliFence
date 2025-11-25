/**
 ******************************************************************************
 * @file           : buzzer.h
 * @brief          : Buzzer module using TIM1 CH3 PWM
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 * @attention
 *
 * Buzzer control module for TPP-IntelliFence project
 * - Uses TIM1 Channel 3 for PWM generation
 * - Nominal frequency: 4 kHz
 * - Configurable duty cycle (volume control)
 * - Thread-safe operation
 *
 ******************************************************************************
 */

#ifndef BUZZER_H
#define BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wlxx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Buzzer operation result codes
 */
typedef enum {
    BUZZER_OK = 0,          /**< Operation successful */
    BUZZER_ERROR,           /**< General error */
    BUZZER_ERROR_INIT,      /**< Initialization error */
    BUZZER_ERROR_PARAM,     /**< Invalid parameter */
    BUZZER_ERROR_TIMEOUT    /**< Operation timeout */
} BuzzerResult_t;

/**
 * @brief Buzzer configuration structure
 */
typedef struct {
    TIM_HandleTypeDef* htim;    /**< Timer handle (TIM1) */
    uint32_t channel;           /**< Timer channel (TIM_CHANNEL_3) */
    uint32_t frequency_hz;      /**< PWM frequency in Hz (default: 4000) */
    uint8_t duty_cycle;         /**< Initial duty cycle 0-100% (default: 50) */
} BuzzerConfig_t;

/**
 * @brief Buzzer state structure
 */
typedef struct {
    bool initialized;           /**< Initialization flag */
    bool enabled;               /**< Buzzer enabled/disabled */
    uint32_t current_freq;      /**< Current frequency in Hz */
    uint8_t current_duty;       /**< Current duty cycle 0-100% */
    uint32_t arr_value;         /**< Auto-reload register value */
    uint32_t ccr_value;         /**< Capture/compare register value */
} BuzzerState_t;

/* Exported constants --------------------------------------------------------*/

/** Default buzzer frequency (4 kHz) */
#define BUZZER_DEFAULT_FREQUENCY    4000

/** Minimum frequency (1 kHz) */
#define BUZZER_MIN_FREQUENCY        1000

/** Maximum frequency (10 kHz) */
#define BUZZER_MAX_FREQUENCY        10000

/** Default duty cycle (50%) */
#define BUZZER_DEFAULT_DUTY_CYCLE   50

/** Minimum duty cycle (0% - off) */
#define BUZZER_MIN_DUTY_CYCLE       0

/** Maximum duty cycle (100%) */
#define BUZZER_MAX_DUTY_CYCLE       100

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize the buzzer module
 * @param config Pointer to buzzer configuration structure
 * @return BuzzerResult_t Result code
 * @note Must be called before any other buzzer function
 */
BuzzerResult_t Buzzer_Init(const BuzzerConfig_t* config);

/**
 * @brief Deinitialize the buzzer module
 * @return BuzzerResult_t Result code
 */
BuzzerResult_t Buzzer_DeInit(void);

/**
 * @brief Turn buzzer ON
 * @return BuzzerResult_t Result code
 * @note Uses current frequency and duty cycle settings
 */
BuzzerResult_t Buzzer_On(void);

/**
 * @brief Turn buzzer OFF
 * @return BuzzerResult_t Result code
 */
BuzzerResult_t Buzzer_Off(void);

/**
 * @brief Toggle buzzer ON/OFF
 * @return BuzzerResult_t Result code
 */
BuzzerResult_t Buzzer_Toggle(void);

/**
 * @brief Set buzzer frequency
 * @param frequency_hz Frequency in Hz (1000-10000 Hz)
 * @return BuzzerResult_t Result code
 * @note Frequency range: 1 kHz - 10 kHz
 *       Nominal frequency: 4 kHz
 */
BuzzerResult_t Buzzer_SetFrequency(uint32_t frequency_hz);

/**
 * @brief Set buzzer duty cycle (volume)
 * @param duty_cycle Duty cycle in percentage (0-100%)
 * @return BuzzerResult_t Result code
 * @note 0% = silence, 100% = maximum volume
 *       Typical values: 25-75% for comfortable volume
 */
BuzzerResult_t Buzzer_SetDutyCycle(uint8_t duty_cycle);

/**
 * @brief Set both frequency and duty cycle
 * @param frequency_hz Frequency in Hz (1000-10000 Hz)
 * @param duty_cycle Duty cycle in percentage (0-100%)
 * @return BuzzerResult_t Result code
 * @note More efficient than calling SetFrequency + SetDutyCycle separately
 */
BuzzerResult_t Buzzer_SetParams(uint32_t frequency_hz, uint8_t duty_cycle);

/**
 * @brief Get current buzzer state
 * @return Pointer to buzzer state structure (read-only)
 */
const BuzzerState_t* Buzzer_GetState(void);

/**
 * @brief Check if buzzer is initialized
 * @return true if initialized, false otherwise
 */
bool Buzzer_IsInitialized(void);

/**
 * @brief Check if buzzer is enabled (ON)
 * @return true if enabled, false otherwise
 */
bool Buzzer_IsEnabled(void);

/**
 * @brief Play a beep with specified duration
 * @param frequency_hz Frequency in Hz
 * @param duty_cycle Duty cycle in percentage (0-100%)
 * @param duration_ms Duration in milliseconds
 * @return BuzzerResult_t Result code
 * @note This is a blocking function
 */
BuzzerResult_t Buzzer_Beep(uint32_t frequency_hz, uint8_t duty_cycle, uint32_t duration_ms);

/**
 * @brief Play a sequence of beeps
 * @param count Number of beeps
 * @param on_time_ms Beep duration in milliseconds
 * @param off_time_ms Silence duration between beeps in milliseconds
 * @return BuzzerResult_t Result code
 * @note This is a blocking function
 */
BuzzerResult_t Buzzer_BeepSequence(uint8_t count, uint32_t on_time_ms, uint32_t off_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_H */
