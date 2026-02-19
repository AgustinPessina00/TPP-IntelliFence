/**
 ******************************************************************************
 * @file           : vibr_motor_alarm.c
 * @brief          : Non-blocking RTOS vibration motor alarm patterns implementation
 * @author         : TPP-IntelliFence Team
 * @date           : February 8, 2026
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "vibr_motor_alarm.h"
#include <string.h>
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {
    VIBR_ALARM_PHASE_OFF = 0,
    VIBR_ALARM_PHASE_ON
} VibrAlarmPhase_t;

/* Private variables ---------------------------------------------------------*/
static VibrAlarmState_t s_alarmState = {0};
static VibrAlarmConfig_t s_currentConfig = {0};
static VibrAlarmPhase_t s_currentPhase = VIBR_ALARM_PHASE_OFF;
static osTimerId_t s_alarmTimer = NULL;
static bool s_initialized = false;

/* Private function prototypes -----------------------------------------------*/
static void alarmTimerCallback(void *argument);
static void startNextPhase(void);
static bool loadPredefinedPattern(VibrAlarmPattern_t pattern);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Alarm timer callback
 */
static void alarmTimerCallback(void *argument) {
    (void)argument;
    
    if (!s_alarmState.active) {
        return;
    }
    
    // Check if we've finished all repetitions
    if (s_currentConfig.repeat_count > 0 && 
        s_alarmState.repetitions >= s_currentConfig.repeat_count) {
        VibrMotorAlarm_Stop();
        return;
    }
    
    startNextPhase();
}

/**
 * @brief Start next phase of alarm pattern
 */
static void startNextPhase(void) {
    if (s_currentPhase == VIBR_ALARM_PHASE_OFF) {
        // Turn ON phase
        VibrMotor_t motor_to_use = s_currentConfig.motor;
        
        // For alternating pattern, switch motor
        if (s_currentConfig.pattern == VIBR_ALARM_ALTERNATING) {
            if (s_alarmState.alternating_left) {
                motor_to_use = MOTOR_LEFT;
            } else {
                motor_to_use = MOTOR_RIGHT;
            }
            // RTOS_LOG_INFO("[VIBR_ALARM] Alternating: using motor %d (left=%d)\\r\\n", 
            //              motor_to_use, s_alarmState.alternating_left);
            s_alarmState.alternating_left = !s_alarmState.alternating_left;
        }
        
        VibrMotor_SetParams(motor_to_use, s_currentConfig.frequency_hz, s_currentConfig.duty_cycle);
        VibrMotor_On(motor_to_use);
        s_currentPhase = VIBR_ALARM_PHASE_ON;
        s_alarmState.running = true;
        
        // Schedule OFF phase (or continuous if on_time_ms == 0)
        if (s_currentConfig.on_time_ms > 0) {
            osTimerStart(s_alarmTimer, pdMS_TO_TICKS(s_currentConfig.on_time_ms));
        } else {
            // Continuous mode - no timer needed
            s_alarmState.running = true;
        }
        
    } else {
        // Turn OFF phase
        VibrMotor_Off(MOTOR_BOTH);
        s_currentPhase = VIBR_ALARM_PHASE_OFF;
        s_alarmState.running = false;
        s_alarmState.repetitions++;
        
        // Check if we should continue
        if (s_currentConfig.repeat_count == 0 || 
            s_alarmState.repetitions < s_currentConfig.repeat_count) {
            
            // Schedule next ON phase
            if (s_currentConfig.off_time_ms > 0) {
                osTimerStart(s_alarmTimer, pdMS_TO_TICKS(s_currentConfig.off_time_ms));
            } else {
                // No off time - restart immediately
                startNextPhase();
            }
        } else {
            // Finished all repetitions
            VibrMotorAlarm_Stop();
        }
    }
}

/**
 * @brief Load predefined pattern configuration
 */
static bool loadPredefinedPattern(VibrAlarmPattern_t pattern) {
    switch (pattern) {
        case VIBR_ALARM_PULSE_ONCE: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_PULSE_ONCE;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_PULSE_DOUBLE: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_PULSE_DOUBLE;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_PULSE_TRIPLE: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_PULSE_TRIPLE;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_WARNING: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_WARNING;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_ALERT: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_ALERT;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_CRITICAL: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_CRITICAL;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_CONTINUOUS: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_CONTINUOUS;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_PULSE_SLOW: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_PULSE_SLOW;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_PULSE_FAST: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_PULSE_FAST;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_ALTERNATING: {
            VibrAlarmConfig_t config = VIBR_ALARM_CONFIG_ALTERNATING;
            memcpy(&s_currentConfig, &config, sizeof(VibrAlarmConfig_t));
            return true;
        }
        
        case VIBR_ALARM_NONE:
        case VIBR_ALARM_CUSTOM:
        default:
            return false;
    }
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize alarm module
 */
bool VibrMotorAlarm_Init(void) {
    if (s_initialized) {
        return true;
    }
    
    // Create software timer
    s_alarmTimer = osTimerNew(alarmTimerCallback, osTimerOnce, NULL, NULL);
    if (s_alarmTimer == NULL) {
        return false;
    }
    
    // Initialize state
    memset(&s_alarmState, 0, sizeof(VibrAlarmState_t));
    memset(&s_currentConfig, 0, sizeof(VibrAlarmConfig_t));
    s_alarmState.timer = s_alarmTimer;
    s_alarmState.alternating_left = true;
    s_currentPhase = VIBR_ALARM_PHASE_OFF;
    
    s_initialized = true;
    return true;
}

/**
 * @brief Deinitialize alarm module
 */
void VibrMotorAlarm_DeInit(void) {
    if (!s_initialized) {
        return;
    }
    
    VibrMotorAlarm_Stop();
    
    if (s_alarmTimer != NULL) {
        osTimerDelete(s_alarmTimer);
        s_alarmTimer = NULL;
    }
    
    s_initialized = false;
}

/**
 * @brief Start alarm with predefined pattern
 */
bool VibrMotorAlarm_StartPattern(VibrAlarmPattern_t pattern) {
    if (!s_initialized || pattern == VIBR_ALARM_NONE) {
        return false;
    }
    
    // Stop current alarm if active
    if (s_alarmState.active) {
        VibrMotorAlarm_Stop();
    }
    
    // Load pattern configuration
    if (!loadPredefinedPattern(pattern)) {
        return false;
    }
    
    // Initialize alarm state
    s_alarmState.active = true;
    s_alarmState.running = false;
    s_alarmState.pattern = pattern;
    s_alarmState.repetitions = 0;
    s_alarmState.alternating_left = true;
    s_currentPhase = VIBR_ALARM_PHASE_OFF;
    
    // Start first phase
    startNextPhase();
    
    return true;
}

/**
 * @brief Start alarm with custom configuration
 */
bool VibrMotorAlarm_StartCustom(const VibrAlarmConfig_t* config) {
    if (!s_initialized || config == NULL) {
        return false;
    }
    
    // Stop current alarm if active
    if (s_alarmState.active) {
        VibrMotorAlarm_Stop();
    }
    
    // Copy custom configuration
    memcpy(&s_currentConfig, config, sizeof(VibrAlarmConfig_t));
    
    // Initialize alarm state
    s_alarmState.active = true;
    s_alarmState.running = false;
    s_alarmState.pattern = VIBR_ALARM_CUSTOM;
    s_alarmState.repetitions = 0;
    s_alarmState.alternating_left = true;
    s_currentPhase = VIBR_ALARM_PHASE_OFF;
    
    // Start first phase
    startNextPhase();
    
    return true;
}

/**
 * @brief Stop current alarm
 */
void VibrMotorAlarm_Stop(void) {
    if (!s_initialized) {
        return;
    }
    
    // Stop timer
    if (s_alarmTimer != NULL) {
        osTimerStop(s_alarmTimer);
    }
    
    // Turn off motors
    VibrMotor_Off(MOTOR_BOTH);
    
    // Reset state
    s_alarmState.active = false;
    s_alarmState.running = false;
    s_alarmState.repetitions = 0;
    s_alarmState.alternating_left = true;
    s_currentPhase = VIBR_ALARM_PHASE_OFF;
}

/**
 * @brief Check if alarm is active
 */
bool VibrMotorAlarm_IsActive(void) {
    return s_alarmState.active;
}

/**
 * @brief Get current alarm state
 */
const VibrAlarmState_t* VibrMotorAlarm_GetState(void) {
    return &s_alarmState;
}

/**
 * @brief Start zone-based alarm (ONLY for YELLOW_ZONE)
 */
bool VibrMotorAlarm_StartZone(zone_t zone) {
    VibrAlarmConfig_t config;
    
    switch (zone) {
        case LIGHT_BLUE_ZONE:
            // Warning zone 1 - alternating pattern with 20% duty cycle
            config = (VibrAlarmConfig_t){
                .pattern = VIBR_ALARM_ALTERNATING,
                .motor = MOTOR_LEFT,
                .frequency_hz = 1000,
                .duty_cycle = 20,
                .on_time_ms = 300,
                .off_time_ms = 0,
                .repeat_count = 0
            };
            return VibrMotorAlarm_StartCustom(&config);
            
        case BLUE_ZONE:
            // Warning zone 2 - alternating pattern with 50% duty cycle
            config = (VibrAlarmConfig_t){
                .pattern = VIBR_ALARM_ALTERNATING,
                .motor = MOTOR_LEFT,
                .frequency_hz = 1000,
                .duty_cycle = 50,
                .on_time_ms = 300,
                .off_time_ms = 0,
                .repeat_count = 0
            };
            return VibrMotorAlarm_StartCustom(&config);
            
        case DARK_BLUE_ZONE:
            // Warning zone 3 - alternating pattern with 65% duty cycle
            config = (VibrAlarmConfig_t){
                .pattern = VIBR_ALARM_ALTERNATING,
                .motor = MOTOR_LEFT,
                .frequency_hz = 1000,
                .duty_cycle = 65,
                .on_time_ms = 300,
                .off_time_ms = 0,
                .repeat_count = 0
            };
            return VibrMotorAlarm_StartCustom(&config);
            
        case YELLOW_ZONE:
            // Alert zone - alternating pattern with 80% duty cycle
            config = (VibrAlarmConfig_t){
                .pattern = VIBR_ALARM_ALTERNATING,
                .motor = MOTOR_LEFT,
                .frequency_hz = 1000,
                .duty_cycle = 80,
                .on_time_ms = 300,
                .off_time_ms = 0,
                .repeat_count = 0
            };
            return VibrMotorAlarm_StartCustom(&config);
            
        default:
            // Other zones - stop vibration
            VibrMotorAlarm_Stop();
            return true;
    }
}
