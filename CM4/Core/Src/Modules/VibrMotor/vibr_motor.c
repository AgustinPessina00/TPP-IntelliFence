/**
 ******************************************************************************
 * @file           : vibr_motor.c
 * @brief          : Vibration Motor module implementation using TIM16/TIM17 PWM
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

/* Includes ------------------------------------------------------------------*/
#include "vibr_motor.h"
#include "cmsis_os.h"
#include <string.h>
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** Timer clock frequency (APB2 timer clock for TIM16/TIM17) */
#define TIM_CLOCK_FREQ      48000000UL  // 48 MHz for STM32WL55

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** Motor configuration */
static VibrMotorConfig_t motor_config;

/** Motor state */
static VibrMotorState_t motor_state = {
    .initialized = false,
    .enabled_left = false,
    .enabled_right = false,
    .current_freq = VIBR_MOTOR_DEFAULT_FREQUENCY,
    .current_duty_left = VIBR_MOTOR_DEFAULT_DUTY_CYCLE,
    .current_duty_right = VIBR_MOTOR_DEFAULT_DUTY_CYCLE,
    .arr_value = 0,
    .ccr_value_left = 0,
    .ccr_value_right = 0
};

/** Mutex for thread-safe operation (optional, for RTOS environments) */
#ifdef VIBR_MOTOR_USE_MUTEX
static osMutexId_t motor_mutex = NULL;
#endif

/* Private function prototypes -----------------------------------------------*/
static void VibrMotor_CalculateTimerValues(uint32_t frequency_hz, uint8_t duty_cycle_left, uint8_t duty_cycle_right);
static void VibrMotor_UpdateTimer(VibrMotor_t motor);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize the vibration motor module
 */
VibrMotorResult_t VibrMotor_Init(const VibrMotorConfig_t* config)
{
    if (config == NULL || config->htim_left == NULL || config->htim_right == NULL) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
    // Validate parameters
    if (config->frequency_hz < VIBR_MOTOR_MIN_FREQUENCY || 
        config->frequency_hz > VIBR_MOTOR_MAX_FREQUENCY) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
    if (config->duty_cycle > VIBR_MOTOR_MAX_DUTY_CYCLE) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
    // Store configuration
    memcpy(&motor_config, config, sizeof(VibrMotorConfig_t));
    
    // Initialize state
    motor_state.current_freq = config->frequency_hz;
    motor_state.current_duty_left = config->duty_cycle;
    motor_state.current_duty_right = config->duty_cycle;
    motor_state.enabled_left = false;
    motor_state.enabled_right = false;
    
    // Calculate timer values
    VibrMotor_CalculateTimerValues(config->frequency_hz, config->duty_cycle, config->duty_cycle);
    
    // Configure LEFT motor timer (TIM17) - ensure CCR1 is 0 for start
    motor_config.htim_left->Instance->ARR = motor_state.arr_value;
    motor_config.htim_left->Instance->CCR1 = 0;  // Force 0 to ensure motor is off
    motor_config.htim_left->Instance->PSC = 0; // No prescaler
    
    TIM_OC_InitTypeDef sConfigOC_Left = {0};
    sConfigOC_Left.OCMode = TIM_OCMODE_PWM1;
    sConfigOC_Left.Pulse = 0;  // Force 0 to ensure motor is off
    sConfigOC_Left.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC_Left.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC_Left.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC_Left.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC_Left.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    
    if (HAL_TIM_PWM_ConfigChannel(motor_config.htim_left, &sConfigOC_Left, motor_config.channel_left) != HAL_OK) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
    // Configure RIGHT motor timer (TIM16) - ensure CCR1 is 0 for start
    motor_config.htim_right->Instance->ARR = motor_state.arr_value;
    motor_config.htim_right->Instance->CCR1 = 0;  // Force 0 to ensure motor is off
    motor_config.htim_right->Instance->PSC = 0; // No prescaler
    
    TIM_OC_InitTypeDef sConfigOC_Right = {0};
    sConfigOC_Right.OCMode = TIM_OCMODE_PWM1;
    sConfigOC_Right.Pulse = 0;  // Force 0 to ensure motor is off
    sConfigOC_Right.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC_Right.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC_Right.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC_Right.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC_Right.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    
    if (HAL_TIM_PWM_ConfigChannel(motor_config.htim_right, &sConfigOC_Right, motor_config.channel_right) != HAL_OK) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    // Create mutex for thread safety
    const osMutexAttr_t motor_mutex_attr = {
        .name = "vibr_motor_mutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = NULL,
        .cb_size = 0
    };
    motor_mutex = osMutexNew(&motor_mutex_attr);
    if (motor_mutex == NULL) {
        return VIBR_MOTOR_ERROR_INIT;
    }
#endif
    
    motor_state.initialized = true;
    
    // Ensure motors are completely OFF at initialization
    HAL_TIM_PWM_Stop(motor_config.htim_left, motor_config.channel_left);
    HAL_TIM_PWM_Stop(motor_config.htim_right, motor_config.channel_right);
    
    // RTOS_LOG_INFO("[VIBR] Init OK - motors OFF (CCR_L=%lu CCR_R=%lu)\\r\\n", 
    //               motor_config.htim_left->Instance->CCR1,
    //               motor_config.htim_right->Instance->CCR1);
    
    return VIBR_MOTOR_OK;
}

/**
 * @brief Deinitialize the vibration motor module
 */
VibrMotorResult_t VibrMotor_DeInit(void)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR;
    }
    
    // Turn off motors first
    VibrMotor_Off(MOTOR_BOTH);
    
#ifdef VIBR_MOTOR_USE_MUTEX
    // Delete mutex
    if (motor_mutex != NULL) {
        osMutexDelete(motor_mutex);
        motor_mutex = NULL;
    }
#endif
    
    motor_state.initialized = false;
    
    return VIBR_MOTOR_OK;
}

/**
 * @brief Turn motor(s) ON
 */
VibrMotorResult_t VibrMotor_On(VibrMotor_t motor)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexAcquire(motor_mutex, osWaitForever);
#endif
    
    HAL_StatusTypeDef status = HAL_OK;
    
    // RTOS_LOG_INFO("[VIBR] Turning ON motor=%d (L=%d R=%d duty_L=%d duty_R=%d)\r\n", 
    //               motor, motor_state.current_duty_left, motor_state.current_duty_right,
    //               motor_state.ccr_value_left, motor_state.ccr_value_right);
    
    // Update timer values before starting
    VibrMotor_UpdateTimer(motor);
    
    if (motor == MOTOR_LEFT || motor == MOTOR_BOTH) {
        status = HAL_TIM_PWM_Start(motor_config.htim_left, motor_config.channel_left);
        //RTOS_LOG_INFO("[VIBR] TIM17 (LEFT) start status=%d\r\n", status);
        if (status == HAL_OK) {
            motor_state.enabled_left = true;
        }
    }
    
    if (motor == MOTOR_RIGHT || motor == MOTOR_BOTH) {
        status = HAL_TIM_PWM_Start(motor_config.htim_right, motor_config.channel_right);
        //RTOS_LOG_INFO("[VIBR] TIM16 (RIGHT) start status=%d\r\n", status);
        if (status == HAL_OK) {
            motor_state.enabled_right = true;
        }
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexRelease(motor_mutex);
#endif
    
    return (status == HAL_OK) ? VIBR_MOTOR_OK : VIBR_MOTOR_ERROR;
}

/**
 * @brief Turn motor(s) OFF
 */
VibrMotorResult_t VibrMotor_Off(VibrMotor_t motor)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexAcquire(motor_mutex, osWaitForever);
#endif
    
    HAL_StatusTypeDef status = HAL_OK;
    
    if (motor == MOTOR_LEFT || motor == MOTOR_BOTH) {
        status = HAL_TIM_PWM_Stop(motor_config.htim_left, motor_config.channel_left);
        if (status == HAL_OK) {
            motor_state.enabled_left = false;
        }
    }
    
    if (motor == MOTOR_RIGHT || motor == MOTOR_BOTH) {
        status = HAL_TIM_PWM_Stop(motor_config.htim_right, motor_config.channel_right);
        if (status == HAL_OK) {
            motor_state.enabled_right = false;
        }
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexRelease(motor_mutex);
#endif
    
    return (status == HAL_OK) ? VIBR_MOTOR_OK : VIBR_MOTOR_ERROR;
}

/**
 * @brief Toggle motor(s) ON/OFF
 */
VibrMotorResult_t VibrMotor_Toggle(VibrMotor_t motor)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
    if (motor == MOTOR_LEFT) {
        return motor_state.enabled_left ? VibrMotor_Off(MOTOR_LEFT) : VibrMotor_On(MOTOR_LEFT);
    } else if (motor == MOTOR_RIGHT) {
        return motor_state.enabled_right ? VibrMotor_Off(MOTOR_RIGHT) : VibrMotor_On(MOTOR_RIGHT);
    } else if (motor == MOTOR_BOTH) {
        bool any_enabled = motor_state.enabled_left || motor_state.enabled_right;
        return any_enabled ? VibrMotor_Off(MOTOR_BOTH) : VibrMotor_On(MOTOR_BOTH);
    }
    
    return VIBR_MOTOR_ERROR_PARAM;
}

/**
 * @brief Set motor frequency
 */
VibrMotorResult_t VibrMotor_SetFrequency(uint32_t frequency_hz)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
    if (frequency_hz < VIBR_MOTOR_MIN_FREQUENCY || frequency_hz > VIBR_MOTOR_MAX_FREQUENCY) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexAcquire(motor_mutex, osWaitForever);
#endif
    
    motor_state.current_freq = frequency_hz;
    VibrMotor_CalculateTimerValues(frequency_hz, motor_state.current_duty_left, motor_state.current_duty_right);
    
    if (motor_state.enabled_left || motor_state.enabled_right) {
        VibrMotor_UpdateTimer(MOTOR_BOTH);
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexRelease(motor_mutex);
#endif
    
    return VIBR_MOTOR_OK;
}

/**
 * @brief Set motor duty cycle (intensity)
 */
VibrMotorResult_t VibrMotor_SetDutyCycle(VibrMotor_t motor, uint8_t duty_cycle)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
    if (duty_cycle > VIBR_MOTOR_MAX_DUTY_CYCLE) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexAcquire(motor_mutex, osWaitForever);
#endif
    
    if (motor == MOTOR_LEFT || motor == MOTOR_BOTH) {
        motor_state.current_duty_left = duty_cycle;
    }
    
    if (motor == MOTOR_RIGHT || motor == MOTOR_BOTH) {
        motor_state.current_duty_right = duty_cycle;
    }
    
    VibrMotor_CalculateTimerValues(motor_state.current_freq, 
                                   motor_state.current_duty_left, 
                                   motor_state.current_duty_right);
    
    if (motor_state.enabled_left || motor_state.enabled_right) {
        VibrMotor_UpdateTimer(motor);
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexRelease(motor_mutex);
#endif
    
    return VIBR_MOTOR_OK;
}

/**
 * @brief Set both frequency and duty cycle
 */
VibrMotorResult_t VibrMotor_SetParams(VibrMotor_t motor, uint32_t frequency_hz, uint8_t duty_cycle)
{
    if (!motor_state.initialized) {
        return VIBR_MOTOR_ERROR_INIT;
    }
    
    if (frequency_hz < VIBR_MOTOR_MIN_FREQUENCY || frequency_hz > VIBR_MOTOR_MAX_FREQUENCY) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
    if (duty_cycle > VIBR_MOTOR_MAX_DUTY_CYCLE) {
        return VIBR_MOTOR_ERROR_PARAM;
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexAcquire(motor_mutex, osWaitForever);
#endif
    
    motor_state.current_freq = frequency_hz;
    
    if (motor == MOTOR_LEFT || motor == MOTOR_BOTH) {
        motor_state.current_duty_left = duty_cycle;
    }
    
    if (motor == MOTOR_RIGHT || motor == MOTOR_BOTH) {
        motor_state.current_duty_right = duty_cycle;
    }
    
    VibrMotor_CalculateTimerValues(frequency_hz, 
                                   motor_state.current_duty_left, 
                                   motor_state.current_duty_right);
    
    if (motor_state.enabled_left || motor_state.enabled_right) {
        VibrMotor_UpdateTimer(motor);
    }
    
#ifdef VIBR_MOTOR_USE_MUTEX
    osMutexRelease(motor_mutex);
#endif
    
    return VIBR_MOTOR_OK;
}

/**
 * @brief Get current motor state
 */
const VibrMotorState_t* VibrMotor_GetState(void)
{
    return &motor_state;
}

/**
 * @brief Check if motor module is initialized
 */
bool VibrMotor_IsInitialized(void)
{
    return motor_state.initialized;
}

/**
 * @brief Check if motor is enabled (ON)
 */
bool VibrMotor_IsEnabled(VibrMotor_t motor)
{
    if (motor == MOTOR_LEFT) {
        return motor_state.enabled_left;
    } else if (motor == MOTOR_RIGHT) {
        return motor_state.enabled_right;
    } else if (motor == MOTOR_BOTH) {
        return motor_state.enabled_left && motor_state.enabled_right;
    }
    return false;
}

/**
 * @brief Vibrate motor with specified duration
 */
VibrMotorResult_t VibrMotor_Pulse(VibrMotor_t motor, uint32_t frequency_hz, uint8_t duty_cycle, uint32_t duration_ms)
{
    VibrMotorResult_t result;
    
    // Set parameters
    result = VibrMotor_SetParams(motor, frequency_hz, duty_cycle);
    if (result != VIBR_MOTOR_OK) {
        return result;
    }
    
    // Turn on
    result = VibrMotor_On(motor);
    if (result != VIBR_MOTOR_OK) {
        return result;
    }
    
    // Wait
    HAL_Delay(duration_ms);
    
    // Turn off
    return VibrMotor_Off(motor);
}

/**
 * @brief Vibrate motor in a sequence
 */
VibrMotorResult_t VibrMotor_PulseSequence(VibrMotor_t motor, uint8_t count, uint32_t on_time_ms, uint32_t off_time_ms)
{
    VibrMotorResult_t result;
    
    for (uint8_t i = 0; i < count; i++) {
        // Turn on
        result = VibrMotor_On(motor);
        if (result != VIBR_MOTOR_OK) {
            return result;
        }
        
        HAL_Delay(on_time_ms);
        
        // Turn off
        result = VibrMotor_Off(motor);
        if (result != VIBR_MOTOR_OK) {
            return result;
        }
        
        // Don't delay after last pulse
        if (i < count - 1) {
            HAL_Delay(off_time_ms);
        }
    }
    
    return VIBR_MOTOR_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Calculate timer ARR and CCR values for desired frequency and duty cycles
 * @param frequency_hz Desired frequency in Hz
 * @param duty_cycle_left Desired duty cycle for left motor in percentage (0-100%)
 * @param duty_cycle_right Desired duty cycle for right motor in percentage (0-100%)
 * @note Formula:
 *       ARR = (TIM_CLOCK / Frequency) - 1
 *       CCR = ARR * (DutyCycle / 100)
 */
static void VibrMotor_CalculateTimerValues(uint32_t frequency_hz, uint8_t duty_cycle_left, uint8_t duty_cycle_right)
{
    // Calculate ARR value for desired frequency
    // ARR = (TIM_CLOCK / Frequency) - 1
    motor_state.arr_value = (TIM_CLOCK_FREQ / frequency_hz) - 1;
    
    // Calculate CCR values for desired duty cycles
    // CCR = ARR * (DutyCycle / 100)
    motor_state.ccr_value_left = (motor_state.arr_value * duty_cycle_left) / 100;
    motor_state.ccr_value_right = (motor_state.arr_value * duty_cycle_right) / 100;
}

/**
 * @brief Update timer registers with current values
 */
static void VibrMotor_UpdateTimer(VibrMotor_t motor)
{
    if (motor == MOTOR_LEFT || motor == MOTOR_BOTH) {
        // Update ARR (frequency) and CCR1 (duty cycle) for LEFT motor
        __HAL_TIM_SET_AUTORELOAD(motor_config.htim_left, motor_state.arr_value);
        __HAL_TIM_SET_COMPARE(motor_config.htim_left, TIM_CHANNEL_1, motor_state.ccr_value_left);
    }
    
    if (motor == MOTOR_RIGHT || motor == MOTOR_BOTH) {
        // Update ARR (frequency) and CCR1 (duty cycle) for RIGHT motor
        __HAL_TIM_SET_AUTORELOAD(motor_config.htim_right, motor_state.arr_value);
        __HAL_TIM_SET_COMPARE(motor_config.htim_right, TIM_CHANNEL_1, motor_state.ccr_value_right);
    }
}
