/**
 ******************************************************************************
 * @file           : vibr_motor.h
 * @brief          : Vibration Motor module using TIM16/TIM17 PWM
 * @author         : TPP-IntelliFence Team
 * @date           : February 8, 2026
 ******************************************************************************
 * @attention
 *
 * Vibration motor control module for TPP-IntelliFence project
 * - Uses TIM16 Channel 1 for LEFT motor (PB8)
 * - Uses TIM17 Channel 1 for RIGHT motor (PB9)
 * - Nominal frequency: 1 kHz
 * - Configurable duty cycle (intensity control)
 * - Independent motor control
 * - Thread-safe operation
 *
 ******************************************************************************
 */

#ifndef VIBR_MOTOR_H
#define VIBR_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wlxx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Motor selection
 */
typedef enum {
    MOTOR_LEFT = 0,     /**< Left motor (TIM16) */
    MOTOR_RIGHT = 1,    /**< Right motor (TIM17) */
    MOTOR_BOTH = 2      /**< Both motors */
} VibrMotor_t;

/**
 * @brief Motor operation result codes
 */
typedef enum {
    VIBR_MOTOR_OK = 0,          /**< Operation successful */
    VIBR_MOTOR_ERROR,           /**< General error */
    VIBR_MOTOR_ERROR_INIT,      /**< Initialization error */
    VIBR_MOTOR_ERROR_PARAM,     /**< Invalid parameter */
    VIBR_MOTOR_ERROR_TIMEOUT    /**< Operation timeout */
} VibrMotorResult_t;

/**
 * @brief Motor configuration structure
 */
typedef struct {
    TIM_HandleTypeDef* htim_left;   /**< Timer handle for left motor (TIM16) */
    TIM_HandleTypeDef* htim_right;  /**< Timer handle for right motor (TIM17) */
    uint32_t channel_left;          /**< Timer channel for left motor (TIM_CHANNEL_1) */
    uint32_t channel_right;         /**< Timer channel for right motor (TIM_CHANNEL_1) */
    uint32_t frequency_hz;          /**< PWM frequency in Hz (default: 1000) */
    uint8_t duty_cycle;             /**< Initial duty cycle 0-100% (default: 50) */
} VibrMotorConfig_t;

/**
 * @brief Motor state structure
 */
typedef struct {
    bool initialized;           /**< Initialization flag */
    bool enabled_left;          /**< Left motor enabled/disabled */
    bool enabled_right;         /**< Right motor enabled/disabled */
    uint32_t current_freq;      /**< Current frequency in Hz */
    uint8_t current_duty_left;  /**< Current duty cycle left 0-100% */
    uint8_t current_duty_right; /**< Current duty cycle right 0-100% */
    uint32_t arr_value;         /**< Auto-reload register value */
    uint32_t ccr_value_left;    /**< Capture/compare register value left */
    uint32_t ccr_value_right;   /**< Capture/compare register value right */
} VibrMotorState_t;

/* Exported constants --------------------------------------------------------*/

/** Default motor frequency (1 kHz) */
#define VIBR_MOTOR_DEFAULT_FREQUENCY    1000

/** Minimum frequency (100 Hz) */
#define VIBR_MOTOR_MIN_FREQUENCY        100

/** Maximum frequency (5 kHz) */
#define VIBR_MOTOR_MAX_FREQUENCY        5000

/** Default duty cycle (50%) */
#define VIBR_MOTOR_DEFAULT_DUTY_CYCLE   50

/** Minimum duty cycle (0% - off) */
#define VIBR_MOTOR_MIN_DUTY_CYCLE       0

/** Maximum duty cycle (100%) */
#define VIBR_MOTOR_MAX_DUTY_CYCLE       100

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize the vibration motor module
 * @param config Pointer to motor configuration structure
 * @return VibrMotorResult_t Result code
 * @note Must be called before any other motor function
 */
VibrMotorResult_t VibrMotor_Init(const VibrMotorConfig_t* config);

/**
 * @brief Deinitialize the vibration motor module
 * @return VibrMotorResult_t Result code
 */
VibrMotorResult_t VibrMotor_DeInit(void);

/**
 * @brief Turn motor(s) ON
 * @param motor Which motor(s) to turn on
 * @return VibrMotorResult_t Result code
 * @note Uses current frequency and duty cycle settings
 */
VibrMotorResult_t VibrMotor_On(VibrMotor_t motor);

/**
 * @brief Turn motor(s) OFF
 * @param motor Which motor(s) to turn off
 * @return VibrMotorResult_t Result code
 */
VibrMotorResult_t VibrMotor_Off(VibrMotor_t motor);

/**
 * @brief Toggle motor(s) ON/OFF
 * @param motor Which motor(s) to toggle
 * @return VibrMotorResult_t Result code
 */
VibrMotorResult_t VibrMotor_Toggle(VibrMotor_t motor);

/**
 * @brief Set motor frequency
 * @param frequency_hz Frequency in Hz (100-5000 Hz)
 * @return VibrMotorResult_t Result code
 * @note Applies to both motors
 */
VibrMotorResult_t VibrMotor_SetFrequency(uint32_t frequency_hz);

/**
 * @brief Set motor duty cycle (intensity)
 * @param motor Which motor(s) to set
 * @param duty_cycle Duty cycle in percentage (0-100%)
 * @return VibrMotorResult_t Result code
 * @note 0% = off, 100% = maximum intensity
 */
VibrMotorResult_t VibrMotor_SetDutyCycle(VibrMotor_t motor, uint8_t duty_cycle);

/**
 * @brief Set both frequency and duty cycle
 * @param motor Which motor(s) to set
 * @param frequency_hz Frequency in Hz (100-5000 Hz)
 * @param duty_cycle Duty cycle in percentage (0-100%)
 * @return VibrMotorResult_t Result code
 * @note More efficient than calling SetFrequency + SetDutyCycle separately
 */
VibrMotorResult_t VibrMotor_SetParams(VibrMotor_t motor, uint32_t frequency_hz, uint8_t duty_cycle);

/**
 * @brief Get current motor state
 * @return Pointer to motor state structure (read-only)
 */
const VibrMotorState_t* VibrMotor_GetState(void);

/**
 * @brief Check if motor module is initialized
 * @return true if initialized, false otherwise
 */
bool VibrMotor_IsInitialized(void);

/**
 * @brief Check if motor is enabled (ON)
 * @param motor Which motor to check
 * @return true if enabled, false otherwise
 */
bool VibrMotor_IsEnabled(VibrMotor_t motor);

/**
 * @brief Vibrate motor with specified duration
 * @param motor Which motor(s) to vibrate
 * @param frequency_hz Frequency in Hz
 * @param duty_cycle Duty cycle in percentage (0-100%)
 * @param duration_ms Duration in milliseconds
 * @return VibrMotorResult_t Result code
 * @note This is a blocking function
 */
VibrMotorResult_t VibrMotor_Pulse(VibrMotor_t motor, uint32_t frequency_hz, uint8_t duty_cycle, uint32_t duration_ms);

/**
 * @brief Vibrate motor in a sequence
 * @param motor Which motor(s) to vibrate
 * @param count Number of pulses
 * @param on_time_ms Pulse duration in milliseconds
 * @param off_time_ms Silence duration between pulses in milliseconds
 * @return VibrMotorResult_t Result code
 * @note This is a blocking function
 */
VibrMotorResult_t VibrMotor_PulseSequence(VibrMotor_t motor, uint8_t count, uint32_t on_time_ms, uint32_t off_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* VIBR_MOTOR_H */
