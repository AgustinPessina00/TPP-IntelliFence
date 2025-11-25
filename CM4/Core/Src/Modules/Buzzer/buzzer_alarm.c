/**
 ******************************************************************************
 * @file           : buzzer_alarm.c
 * @brief          : Non-blocking RTOS buzzer alarm patterns implementation
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "buzzer_alarm.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum {
    ALARM_PHASE_OFF = 0,
    ALARM_PHASE_ON
} AlarmPhase_t;

/* Private variables ---------------------------------------------------------*/
static AlarmState_t s_alarmState = {0};
static AlarmConfig_t s_currentConfig = {0};
static AlarmPhase_t s_currentPhase = ALARM_PHASE_OFF;
static osTimerId_t s_alarmTimer = NULL;
static bool s_initialized = false;

/* Private function prototypes -----------------------------------------------*/
static void alarmTimerCallback(void *argument);
static void startNextPhase(void);
static bool loadPredefinedPattern(AlarmPattern_t pattern);

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
        BuzzerAlarm_Stop();
        return;
    }
    
    startNextPhase();
}

/**
 * @brief Start next phase of alarm pattern
 */
static void startNextPhase(void) {
    if (s_currentPhase == ALARM_PHASE_OFF) {
        // Turn ON phase
        Buzzer_SetParams(s_currentConfig.frequency_hz, s_currentConfig.duty_cycle);
        Buzzer_On();
        s_currentPhase = ALARM_PHASE_ON;
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
        Buzzer_Off();
        s_currentPhase = ALARM_PHASE_OFF;
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
            BuzzerAlarm_Stop();
        }
    }
}

/**
 * @brief Load predefined pattern configuration
 */
static bool loadPredefinedPattern(AlarmPattern_t pattern) {
    switch (pattern) {
        case ALARM_BEEP_ONCE: {
            AlarmConfig_t config = ALARM_CONFIG_BEEP_ONCE;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_BEEP_DOUBLE: {
            AlarmConfig_t config = ALARM_CONFIG_BEEP_DOUBLE;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_BEEP_TRIPLE: {
            AlarmConfig_t config = ALARM_CONFIG_BEEP_TRIPLE;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_WARNING: {
            AlarmConfig_t config = ALARM_CONFIG_WARNING;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_ALERT: {
            AlarmConfig_t config = ALARM_CONFIG_ALERT;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_CRITICAL: {
            AlarmConfig_t config = ALARM_CONFIG_CRITICAL;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_CONTINUOUS: {
            AlarmConfig_t config = ALARM_CONFIG_CONTINUOUS;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_PULSE_SLOW: {
            AlarmConfig_t config = ALARM_CONFIG_PULSE_SLOW;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_PULSE_FAST: {
            AlarmConfig_t config = ALARM_CONFIG_PULSE_FAST;
            memcpy(&s_currentConfig, &config, sizeof(AlarmConfig_t));
            return true;
        }
        
        case ALARM_NONE:
        case ALARM_CUSTOM:
        default:
            return false;
    }
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize alarm module
 */
bool BuzzerAlarm_Init(void) {
    if (s_initialized) {
        return true;
    }
    
    // Create software timer
    s_alarmTimer = osTimerNew(alarmTimerCallback, osTimerOnce, NULL, NULL);
    if (s_alarmTimer == NULL) {
        return false;
    }
    
    // Initialize state
    memset(&s_alarmState, 0, sizeof(AlarmState_t));
    memset(&s_currentConfig, 0, sizeof(AlarmConfig_t));
    s_alarmState.timer = s_alarmTimer;
    s_currentPhase = ALARM_PHASE_OFF;
    
    s_initialized = true;
    return true;
}

/**
 * @brief Deinitialize alarm module
 */
void BuzzerAlarm_DeInit(void) {
    if (!s_initialized) {
        return;
    }
    
    BuzzerAlarm_Stop();
    
    if (s_alarmTimer != NULL) {
        osTimerDelete(s_alarmTimer);
        s_alarmTimer = NULL;
    }
    
    s_initialized = false;
}

/**
 * @brief Start alarm with predefined pattern
 */
bool BuzzerAlarm_StartPattern(AlarmPattern_t pattern) {
    if (!s_initialized || pattern == ALARM_NONE) {
        return false;
    }
    
    // Stop current alarm if active
    if (s_alarmState.active) {
        BuzzerAlarm_Stop();
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
    s_currentPhase = ALARM_PHASE_OFF;
    
    // Start first phase
    startNextPhase();
    
    return true;
}

/**
 * @brief Start alarm with custom configuration
 */
bool BuzzerAlarm_StartCustom(const AlarmConfig_t* config) {
    if (!s_initialized || config == NULL) {
        return false;
    }
    
    // Stop current alarm if active
    if (s_alarmState.active) {
        BuzzerAlarm_Stop();
    }
    
    // Copy custom configuration
    memcpy(&s_currentConfig, config, sizeof(AlarmConfig_t));
    
    // Initialize alarm state
    s_alarmState.active = true;
    s_alarmState.running = false;
    s_alarmState.pattern = ALARM_CUSTOM;
    s_alarmState.repetitions = 0;
    s_currentPhase = ALARM_PHASE_OFF;
    
    // Start first phase
    startNextPhase();
    
    return true;
}

/**
 * @brief Stop current alarm
 */
void BuzzerAlarm_Stop(void) {
    if (!s_initialized) {
        return;
    }
    
    // Stop timer
    if (s_alarmTimer != NULL) {
        osTimerStop(s_alarmTimer);
    }
    
    // Turn off buzzer
    Buzzer_Off();
    
    // Reset state
    s_alarmState.active = false;
    s_alarmState.running = false;
    s_alarmState.repetitions = 0;
    s_currentPhase = ALARM_PHASE_OFF;
}

/**
 * @brief Check if alarm is active
 */
bool BuzzerAlarm_IsActive(void) {
    return s_alarmState.active;
}

/**
 * @brief Get current alarm state
 */
const AlarmState_t* BuzzerAlarm_GetState(void) {
    return &s_alarmState;
}

/**
 * @brief Start zone-based alarm
 */
bool BuzzerAlarm_StartZone(zone_t zone) {
    AlarmPattern_t pattern;
    
    switch (zone) {
        case GREEN_ZONE:
            // No alarm in safe zone
            BuzzerAlarm_Stop();
            return true;
            
        case LIGHT_BLUE_ZONE:
            // Gentle warning - slow pulse
            pattern = ALARM_PULSE_SLOW;
            break;
            
        case BLUE_ZONE:
            // Medium warning
            pattern = ALARM_WARNING;
            break;
            
        case DARK_BLUE_ZONE:
            // Strong warning - fast pulse
            pattern = ALARM_PULSE_FAST;
            break;
            
        case YELLOW_ZONE:
            // Alert - repeating beeps
            pattern = ALARM_ALERT;
            break;
            
        case RED_ZONE:
            // Critical - rapid beeps
            pattern = ALARM_CRITICAL;
            break;
            
        case BLACK_ZONE:
            // Escape - stop all (silent alert mode)
            BuzzerAlarm_Stop();
            return true;
            
        default:
            return false;
    }
    
    return BuzzerAlarm_StartPattern(pattern);
}
