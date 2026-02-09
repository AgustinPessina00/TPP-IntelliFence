/**
 ******************************************************************************
 * @file           : vibr_motor_alarm.h
 * @brief          : Non-blocking RTOS vibration motor alarm patterns
 * @author         : TPP-IntelliFence Team
 * @date           : February 8, 2026
 ******************************************************************************
 * @attention
 *
 * RTOS-based alarm module for non-blocking vibration motor patterns
 * - Uses FreeRTOS software timers
 * - Multiple predefined alarm patterns
 * - Zone-based alarms for fence system
 * - Independent left/right motor control
 * - Thread-safe operations
 *
 ******************************************************************************
 */

#ifndef VIBR_MOTOR_ALARM_H
#define VIBR_MOTOR_ALARM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "vibr_motor.h"
#include "zone.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Alarm pattern types
 */
typedef enum {
    VIBR_ALARM_NONE = 0,           /**< No alarm (silent) */
    VIBR_ALARM_PULSE_ONCE,         /**< Single pulse */
    VIBR_ALARM_PULSE_DOUBLE,       /**< Double pulse */
    VIBR_ALARM_PULSE_TRIPLE,       /**< Triple pulse */
    VIBR_ALARM_WARNING,            /**< Slow repeating pulses */
    VIBR_ALARM_ALERT,              /**< Fast repeating pulses */
    VIBR_ALARM_CRITICAL,           /**< Very fast repeating pulses */
    VIBR_ALARM_CONTINUOUS,         /**< Continuous vibration */
    VIBR_ALARM_PULSE_SLOW,         /**< Slow pulsating */
    VIBR_ALARM_PULSE_FAST,         /**< Fast pulsating */
    VIBR_ALARM_ALTERNATING,        /**< Left-right alternating */
    VIBR_ALARM_CUSTOM              /**< Custom pattern */
} VibrAlarmPattern_t;

/**
 * @brief Alarm configuration structure
 */
typedef struct {
    VibrAlarmPattern_t pattern;   /**< Alarm pattern type */
    VibrMotor_t motor;            /**< Which motor(s) to use */
    uint32_t frequency_hz;        /**< Motor frequency in Hz */
    uint8_t duty_cycle;           /**< Motor duty cycle 0-100% */
    uint32_t on_time_ms;          /**< On time in milliseconds */
    uint32_t off_time_ms;         /**< Off time in milliseconds */
    uint16_t repeat_count;        /**< Number of repetitions (0 = infinite) */
} VibrAlarmConfig_t;

/**
 * @brief Alarm state structure
 */
typedef struct {
    bool active;                  /**< Alarm is active */
    bool running;                 /**< Alarm is currently running */
    VibrAlarmPattern_t pattern;   /**< Current pattern */
    uint16_t repetitions;         /**< Current repetition count */
    osTimerId_t timer;            /**< Software timer handle */
    bool alternating_left;        /**< For alternating pattern - current is left */
} VibrAlarmState_t;

/* Exported constants --------------------------------------------------------*/

/** Predefined alarm configurations */

// Single pulse: 1kHz, 70%, 200ms
#define VIBR_ALARM_CONFIG_PULSE_ONCE  { \
    .pattern = VIBR_ALARM_PULSE_ONCE, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1000, \
    .duty_cycle = 70, \
    .on_time_ms = 200, \
    .off_time_ms = 0, \
    .repeat_count = 1 \
}

// Double pulse: 1kHz, 70%, 100ms on, 100ms off, repeat 2x
#define VIBR_ALARM_CONFIG_PULSE_DOUBLE  { \
    .pattern = VIBR_ALARM_PULSE_DOUBLE, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1000, \
    .duty_cycle = 70, \
    .on_time_ms = 100, \
    .off_time_ms = 100, \
    .repeat_count = 2 \
}

// Triple pulse: 1kHz, 70%, 100ms on, 100ms off, repeat 3x
#define VIBR_ALARM_CONFIG_PULSE_TRIPLE  { \
    .pattern = VIBR_ALARM_PULSE_TRIPLE, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1000, \
    .duty_cycle = 70, \
    .on_time_ms = 100, \
    .off_time_ms = 100, \
    .repeat_count = 3 \
}

// Warning: 800Hz, 50%, 150ms on, 1s off, infinite
#define VIBR_ALARM_CONFIG_WARNING  { \
    .pattern = VIBR_ALARM_WARNING, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 800, \
    .duty_cycle = 50, \
    .on_time_ms = 150, \
    .off_time_ms = 1000, \
    .repeat_count = 0 \
}

// Alert: 1kHz, 80%, 100ms on, 300ms off, infinite
#define VIBR_ALARM_CONFIG_ALERT  { \
    .pattern = VIBR_ALARM_ALERT, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1000, \
    .duty_cycle = 80, \
    .on_time_ms = 100, \
    .off_time_ms = 300, \
    .repeat_count = 0 \
}

// Critical: 1.5kHz, 90%, 80ms on, 80ms off, infinite
#define VIBR_ALARM_CONFIG_CRITICAL  { \
    .pattern = VIBR_ALARM_CRITICAL, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1500, \
    .duty_cycle = 90, \
    .on_time_ms = 80, \
    .off_time_ms = 80, \
    .repeat_count = 0 \
}

// Continuous: 1kHz, 75%, continuous
#define VIBR_ALARM_CONFIG_CONTINUOUS  { \
    .pattern = VIBR_ALARM_CONTINUOUS, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1000, \
    .duty_cycle = 75, \
    .on_time_ms = 0, \
    .off_time_ms = 0, \
    .repeat_count = 0 \
}

// Pulse slow: 800Hz, 60%, 500ms on, 500ms off, infinite
#define VIBR_ALARM_CONFIG_PULSE_SLOW  { \
    .pattern = VIBR_ALARM_PULSE_SLOW, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 800, \
    .duty_cycle = 60, \
    .on_time_ms = 500, \
    .off_time_ms = 500, \
    .repeat_count = 0 \
}

// Pulse fast: 1.2kHz, 70%, 200ms on, 200ms off, infinite
#define VIBR_ALARM_CONFIG_PULSE_FAST  { \
    .pattern = VIBR_ALARM_PULSE_FAST, \
    .motor = MOTOR_BOTH, \
    .frequency_hz = 1200, \
    .duty_cycle = 70, \
    .on_time_ms = 200, \
    .off_time_ms = 200, \
    .repeat_count = 0 \
}

// Alternating: 1kHz, 50%, 300ms each side, infinite
#define VIBR_ALARM_CONFIG_ALTERNATING  { \
    .pattern = VIBR_ALARM_ALTERNATING, \
    .motor = MOTOR_LEFT, \
    .frequency_hz = 1000, \
    .duty_cycle = 50, \
    .on_time_ms = 300, \
    .off_time_ms = 0, \
    .repeat_count = 0 \
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize alarm module
 * @return true if successful, false otherwise
 * @note Must be called before using any other alarm function
 */
bool VibrMotorAlarm_Init(void);

/**
 * @brief Deinitialize alarm module
 */
void VibrMotorAlarm_DeInit(void);

/**
 * @brief Start an alarm with predefined pattern
 * @param pattern Alarm pattern type
 * @return true if alarm started, false otherwise
 */
bool VibrMotorAlarm_StartPattern(VibrAlarmPattern_t pattern);

/**
 * @brief Start an alarm with custom configuration
 * @param config Pointer to alarm configuration
 * @return true if alarm started, false otherwise
 */
bool VibrMotorAlarm_StartCustom(const VibrAlarmConfig_t* config);

/**
 * @brief Stop the current alarm
 */
void VibrMotorAlarm_Stop(void);

/**
 * @brief Check if alarm is active
 * @return true if active, false otherwise
 */
bool VibrMotorAlarm_IsActive(void);

/**
 * @brief Get current alarm state
 * @return Pointer to alarm state (read-only)
 */
const VibrAlarmState_t* VibrMotorAlarm_GetState(void);

/**
 * @brief Start zone-based alarm
 * @param zone Fence zone
 * @return true if alarm started, false otherwise
 * @note Automatically selects appropriate alarm pattern for zone
 */
bool VibrMotorAlarm_StartZone(zone_t zone);

#ifdef __cplusplus
}
#endif

#endif /* VIBR_MOTOR_ALARM_H */
