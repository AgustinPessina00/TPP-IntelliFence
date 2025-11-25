/**
 ******************************************************************************
 * @file           : buzzer.c
 * @brief          : Buzzer module implementation using TIM1 CH3 PWM
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

/* Includes ------------------------------------------------------------------*/
#include "buzzer.h"
#include "cmsis_os.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** Timer clock frequency (APB2 timer clock for TIM1) */
#define TIM_CLOCK_FREQ      48000000UL  // 48 MHz for STM32WL55

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** Buzzer configuration */
static BuzzerConfig_t buzzer_config;

/** Buzzer state */
static BuzzerState_t buzzer_state = {
    .initialized = false,
    .enabled = false,
    .current_freq = BUZZER_DEFAULT_FREQUENCY,
    .current_duty = BUZZER_DEFAULT_DUTY_CYCLE,
    .arr_value = 0,
    .ccr_value = 0
};

/** Mutex for thread-safe operation (optional, for RTOS environments) */
#ifdef BUZZER_USE_MUTEX
static osMutexId_t buzzer_mutex = NULL;
#endif

/* Private function prototypes -----------------------------------------------*/
static void Buzzer_CalculateTimerValues(uint32_t frequency_hz, uint8_t duty_cycle);
static void Buzzer_UpdateTimer(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize the buzzer module
 */
BuzzerResult_t Buzzer_Init(const BuzzerConfig_t* config)
{
    if (config == NULL || config->htim == NULL) {
        return BUZZER_ERROR_PARAM;
    }
    
    // Validate parameters
    if (config->frequency_hz < BUZZER_MIN_FREQUENCY || 
        config->frequency_hz > BUZZER_MAX_FREQUENCY) {
        return BUZZER_ERROR_PARAM;
    }
    
    if (config->duty_cycle > BUZZER_MAX_DUTY_CYCLE) {
        return BUZZER_ERROR_PARAM;
    }
    
    // Store configuration
    memcpy(&buzzer_config, config, sizeof(BuzzerConfig_t));
    
    // Initialize state
    buzzer_state.current_freq = config->frequency_hz;
    buzzer_state.current_duty = config->duty_cycle;
    buzzer_state.enabled = false;
    
    // Calculate timer values
    Buzzer_CalculateTimerValues(config->frequency_hz, config->duty_cycle);
    
    // Configure timer (but don't start yet)
    buzzer_config.htim->Instance->ARR = buzzer_state.arr_value;
    buzzer_config.htim->Instance->CCR3 = buzzer_state.ccr_value;
    
    // Set prescaler for desired frequency range
    // PSC = (TIM_CLOCK / (ARR + 1) / Frequency) - 1
    buzzer_config.htim->Instance->PSC = 0; // No prescaler for now, calculated in timer values
    
    // Initialize PWM channel
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = buzzer_state.ccr_value;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    
    if (HAL_TIM_PWM_ConfigChannel(buzzer_config.htim, &sConfigOC, buzzer_config.channel) != HAL_OK) {
        return BUZZER_ERROR_INIT;
    }
    
#ifdef BUZZER_USE_MUTEX
    // Create mutex for thread safety
    const osMutexAttr_t buzzer_mutex_attr = {
        .name = "buzzer_mutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = NULL,
        .cb_size = 0
    };
    buzzer_mutex = osMutexNew(&buzzer_mutex_attr);
    if (buzzer_mutex == NULL) {
        return BUZZER_ERROR_INIT;
    }
#endif
    
    buzzer_state.initialized = true;
    
    return BUZZER_OK;
}

/**
 * @brief Deinitialize the buzzer module
 */
BuzzerResult_t Buzzer_DeInit(void)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR;
    }
    
    // Turn off buzzer first
    Buzzer_Off();
    
#ifdef BUZZER_USE_MUTEX
    // Delete mutex
    if (buzzer_mutex != NULL) {
        osMutexDelete(buzzer_mutex);
        buzzer_mutex = NULL;
    }
#endif
    
    buzzer_state.initialized = false;
    
    return BUZZER_OK;
}

/**
 * @brief Turn buzzer ON
 */
BuzzerResult_t Buzzer_On(void)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR_INIT;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexAcquire(buzzer_mutex, osWaitForever);
#endif
    
    // Update timer values before starting
    Buzzer_UpdateTimer();
    
    // Start PWM on channel 3
    HAL_StatusTypeDef status = HAL_TIM_PWM_Start(buzzer_config.htim, buzzer_config.channel);
    
    if (status == HAL_OK) {
        buzzer_state.enabled = true;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexRelease(buzzer_mutex);
#endif
    
    return (status == HAL_OK) ? BUZZER_OK : BUZZER_ERROR;
}

/**
 * @brief Turn buzzer OFF
 */
BuzzerResult_t Buzzer_Off(void)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR_INIT;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexAcquire(buzzer_mutex, osWaitForever);
#endif
    
    // Stop PWM on channel 3
    HAL_StatusTypeDef status = HAL_TIM_PWM_Stop(buzzer_config.htim, buzzer_config.channel);
    
    if (status == HAL_OK) {
        buzzer_state.enabled = false;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexRelease(buzzer_mutex);
#endif
    
    return (status == HAL_OK) ? BUZZER_OK : BUZZER_ERROR;
}

/**
 * @brief Toggle buzzer ON/OFF
 */
BuzzerResult_t Buzzer_Toggle(void)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR_INIT;
    }
    
    return buzzer_state.enabled ? Buzzer_Off() : Buzzer_On();
}

/**
 * @brief Set buzzer frequency
 */
BuzzerResult_t Buzzer_SetFrequency(uint32_t frequency_hz)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR_INIT;
    }
    
    if (frequency_hz < BUZZER_MIN_FREQUENCY || frequency_hz > BUZZER_MAX_FREQUENCY) {
        return BUZZER_ERROR_PARAM;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexAcquire(buzzer_mutex, osWaitForever);
#endif
    
    buzzer_state.current_freq = frequency_hz;
    Buzzer_CalculateTimerValues(frequency_hz, buzzer_state.current_duty);
    
    if (buzzer_state.enabled) {
        Buzzer_UpdateTimer();
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexRelease(buzzer_mutex);
#endif
    
    return BUZZER_OK;
}

/**
 * @brief Set buzzer duty cycle (volume)
 */
BuzzerResult_t Buzzer_SetDutyCycle(uint8_t duty_cycle)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR_INIT;
    }
    
    if (duty_cycle > BUZZER_MAX_DUTY_CYCLE) {
        return BUZZER_ERROR_PARAM;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexAcquire(buzzer_mutex, osWaitForever);
#endif
    
    buzzer_state.current_duty = duty_cycle;
    Buzzer_CalculateTimerValues(buzzer_state.current_freq, duty_cycle);
    
    if (buzzer_state.enabled) {
        Buzzer_UpdateTimer();
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexRelease(buzzer_mutex);
#endif
    
    return BUZZER_OK;
}

/**
 * @brief Set both frequency and duty cycle
 */
BuzzerResult_t Buzzer_SetParams(uint32_t frequency_hz, uint8_t duty_cycle)
{
    if (!buzzer_state.initialized) {
        return BUZZER_ERROR_INIT;
    }
    
    if (frequency_hz < BUZZER_MIN_FREQUENCY || frequency_hz > BUZZER_MAX_FREQUENCY) {
        return BUZZER_ERROR_PARAM;
    }
    
    if (duty_cycle > BUZZER_MAX_DUTY_CYCLE) {
        return BUZZER_ERROR_PARAM;
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexAcquire(buzzer_mutex, osWaitForever);
#endif
    
    buzzer_state.current_freq = frequency_hz;
    buzzer_state.current_duty = duty_cycle;
    Buzzer_CalculateTimerValues(frequency_hz, duty_cycle);
    
    if (buzzer_state.enabled) {
        Buzzer_UpdateTimer();
    }
    
#ifdef BUZZER_USE_MUTEX
    osMutexRelease(buzzer_mutex);
#endif
    
    return BUZZER_OK;
}

/**
 * @brief Get current buzzer state
 */
const BuzzerState_t* Buzzer_GetState(void)
{
    return &buzzer_state;
}

/**
 * @brief Check if buzzer is initialized
 */
bool Buzzer_IsInitialized(void)
{
    return buzzer_state.initialized;
}

/**
 * @brief Check if buzzer is enabled (ON)
 */
bool Buzzer_IsEnabled(void)
{
    return buzzer_state.enabled;
}

/**
 * @brief Play a beep with specified duration
 */
BuzzerResult_t Buzzer_Beep(uint32_t frequency_hz, uint8_t duty_cycle, uint32_t duration_ms)
{
    BuzzerResult_t result;
    
    // Set parameters
    result = Buzzer_SetParams(frequency_hz, duty_cycle);
    if (result != BUZZER_OK) {
        return result;
    }
    
    // Turn on
    result = Buzzer_On();
    if (result != BUZZER_OK) {
        return result;
    }
    
    // Wait
    HAL_Delay(duration_ms);
    
    // Turn off
    return Buzzer_Off();
}

/**
 * @brief Play a sequence of beeps
 */
BuzzerResult_t Buzzer_BeepSequence(uint8_t count, uint32_t on_time_ms, uint32_t off_time_ms)
{
    BuzzerResult_t result;
    
    for (uint8_t i = 0; i < count; i++) {
        // Turn on
        result = Buzzer_On();
        if (result != BUZZER_OK) {
            return result;
        }
        
        HAL_Delay(on_time_ms);
        
        // Turn off
        result = Buzzer_Off();
        if (result != BUZZER_OK) {
            return result;
        }
        
        // Don't delay after last beep
        if (i < count - 1) {
            HAL_Delay(off_time_ms);
        }
    }
    
    return BUZZER_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Calculate timer ARR and CCR values for desired frequency and duty cycle
 * @param frequency_hz Desired frequency in Hz
 * @param duty_cycle Desired duty cycle in percentage (0-100%)
 * @note Formula:
 *       ARR = (TIM_CLOCK / Frequency) - 1
 *       CCR = ARR * (DutyCycle / 100)
 */
static void Buzzer_CalculateTimerValues(uint32_t frequency_hz, uint8_t duty_cycle)
{
    // Calculate ARR value for desired frequency
    // ARR = (TIM_CLOCK / Frequency) - 1
    buzzer_state.arr_value = (TIM_CLOCK_FREQ / frequency_hz) - 1;
    
    // Calculate CCR value for desired duty cycle
    // CCR = ARR * (DutyCycle / 100)
    buzzer_state.ccr_value = (buzzer_state.arr_value * duty_cycle) / 100;
}

/**
 * @brief Update timer registers with current values
 */
static void Buzzer_UpdateTimer(void)
{
    // Update ARR (frequency)
    __HAL_TIM_SET_AUTORELOAD(buzzer_config.htim, buzzer_state.arr_value);
    
    // Update CCR3 (duty cycle)
    __HAL_TIM_SET_COMPARE(buzzer_config.htim, TIM_CHANNEL_3, buzzer_state.ccr_value);
}
