/**
 ******************************************************************************
 * @file           : buzzer_alarm.h
 * @brief          : Non-blocking RTOS buzzer alarm patterns
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 * @attention
 *
 * RTOS-based alarm module for non-blocking buzzer patterns
 * - Uses FreeRTOS software timers
 * - Multiple predefined alarm patterns
 * - Zone-based alarms for fence system
 * - Thread-safe operations
 *
 ******************************************************************************
 */

#ifndef BUZZER_ALARM_H
#define BUZZER_ALARM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "buzzer.h"
#include "zone.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Alarm pattern types
 */
typedef enum {
    ALARM_NONE = 0,           /**< No alarm (silent) */
    ALARM_BEEP_ONCE,          /**< Single beep */
    ALARM_BEEP_DOUBLE,        /**< Double beep */
    ALARM_BEEP_TRIPLE,        /**< Triple beep */
    ALARM_WARNING,            /**< Slow repeating beeps */
    ALARM_ALERT,              /**< Fast repeating beeps */
    ALARM_CRITICAL,           /**< Very fast repeating beeps */
    ALARM_CONTINUOUS,         /**< Continuous sound */
    ALARM_PULSE_SLOW,         /**< Slow pulsating */
    ALARM_PULSE_FAST,         /**< Fast pulsating */
    ALARM_CUSTOM              /**< Custom pattern */
} AlarmPattern_t;

/**
 * @brief Alarm configuration structure
 */
typedef struct {
    AlarmPattern_t pattern;   /**< Alarm pattern type */
    uint32_t frequency_hz;    /**< Buzzer frequency in Hz */
    uint8_t duty_cycle;       /**< Buzzer duty cycle 0-100% */
    uint32_t on_time_ms;      /**< On time in milliseconds */
    uint32_t off_time_ms;     /**< Off time in milliseconds */
    uint16_t repeat_count;    /**< Number of repetitions (0 = infinite) */
} AlarmConfig_t;

/**
 * @brief Alarm state structure
 */
typedef struct {
    bool active;              /**< Alarm is active */
    bool running;             /**< Alarm is currently running */
    AlarmPattern_t pattern;   /**< Current pattern */
    uint16_t repetitions;     /**< Current repetition count */
    osTimerId_t timer;        /**< Software timer handle */
} AlarmState_t;

/* Exported constants --------------------------------------------------------*/

/** Predefined alarm configurations */

// Single beep: 4kHz, 50%, 200ms
#define ALARM_CONFIG_BEEP_ONCE  { \
    .pattern = ALARM_BEEP_ONCE, \
    .frequency_hz = 4000, \
    .duty_cycle = 50, \
    .on_time_ms = 200, \
    .off_time_ms = 0, \
    .repeat_count = 1 \
}

// Double beep: 4kHz, 50%, 100ms on, 100ms off, repeat 2x
#define ALARM_CONFIG_BEEP_DOUBLE  { \
    .pattern = ALARM_BEEP_DOUBLE, \
    .frequency_hz = 4000, \
    .duty_cycle = 50, \
    .on_time_ms = 100, \
    .off_time_ms = 100, \
    .repeat_count = 2 \
}

// Triple beep: 4kHz, 50%, 100ms on, 100ms off, repeat 3x
#define ALARM_CONFIG_BEEP_TRIPLE  { \
    .pattern = ALARM_BEEP_TRIPLE, \
    .frequency_hz = 4000, \
    .duty_cycle = 50, \
    .on_time_ms = 100, \
    .off_time_ms = 100, \
    .repeat_count = 3 \
}

// Warning: 3.5kHz, 40%, 100ms on, 1s off, infinite
#define ALARM_CONFIG_WARNING  { \
    .pattern = ALARM_WARNING, \
    .frequency_hz = 3500, \
    .duty_cycle = 40, \
    .on_time_ms = 100, \
    .off_time_ms = 1000, \
    .repeat_count = 0 \
}

// Alert: 4kHz, 60%, 80ms on, 200ms off, infinite
#define ALARM_CONFIG_ALERT  { \
    .pattern = ALARM_ALERT, \
    .frequency_hz = 4000, \
    .duty_cycle = 60, \
    .on_time_ms = 80, \
    .off_time_ms = 200, \
    .repeat_count = 0 \
}

// Critical: 5kHz, 80%, 50ms on, 50ms off, infinite
#define ALARM_CONFIG_CRITICAL  { \
    .pattern = ALARM_CRITICAL, \
    .frequency_hz = 5000, \
    .duty_cycle = 80, \
    .on_time_ms = 50, \
    .off_time_ms = 50, \
    .repeat_count = 0 \
}

// Continuous: 4.5kHz, 70%, continuous
#define ALARM_CONFIG_CONTINUOUS  { \
    .pattern = ALARM_CONTINUOUS, \
    .frequency_hz = 4500, \
    .duty_cycle = 70, \
    .on_time_ms = 0, \
    .off_time_ms = 0, \
    .repeat_count = 0 \
}

// Pulse slow: 3kHz, 50%, 500ms on, 500ms off, infinite
#define ALARM_CONFIG_PULSE_SLOW  { \
    .pattern = ALARM_PULSE_SLOW, \
    .frequency_hz = 3000, \
    .duty_cycle = 50, \
    .on_time_ms = 500, \
    .off_time_ms = 500, \
    .repeat_count = 0 \
}

// Pulse fast: 4kHz, 60%, 200ms on, 200ms off, infinite
#define ALARM_CONFIG_PULSE_FAST  { \
    .pattern = ALARM_PULSE_FAST, \
    .frequency_hz = 4000, \
    .duty_cycle = 60, \
    .on_time_ms = 200, \
    .off_time_ms = 200, \
    .repeat_count = 0 \
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize alarm module
 * @return true if successful, false otherwise
 * @note Must be called before using any other alarm function
 */
bool BuzzerAlarm_Init(void);

/**
 * @brief Deinitialize alarm module
 */
void BuzzerAlarm_DeInit(void);

/**
 * @brief Start an alarm with predefined pattern
 * @param pattern Alarm pattern type
 * @return true if alarm started, false otherwise
 */
bool BuzzerAlarm_StartPattern(AlarmPattern_t pattern);

/**
 * @brief Start an alarm with custom configuration
 * @param config Pointer to alarm configuration
 * @return true if alarm started, false otherwise
 */
bool BuzzerAlarm_StartCustom(const AlarmConfig_t* config);

/**
 * @brief Stop the current alarm
 */
void BuzzerAlarm_Stop(void);

/**
 * @brief Check if alarm is active
 * @return true if active, false otherwise
 */
bool BuzzerAlarm_IsActive(void);

/**
 * @brief Get current alarm state
 * @return Pointer to alarm state (read-only)
 */
const AlarmState_t* BuzzerAlarm_GetState(void);

/**
 * @brief Start zone-based alarm
 * @param zone Fence zone
 * @return true if alarm started, false otherwise
 * @note Automatically selects appropriate alarm pattern for zone
 */
bool BuzzerAlarm_StartZone(zone_t zone);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_ALARM_H */
