/**
 ******************************************************************************
 * @file           : buzzer_alarm_example.c
 * @brief          : RTOS buzzer alarm usage examples
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 * @attention
 *
 * These examples demonstrate NON-BLOCKING alarm patterns using FreeRTOS
 * - All alarms run in background using software timers
 * - No blocking delays in application code
 * - Easy zone-based integration
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "buzzer_alarm.h"
#include "buzzer.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;  // Timer handle (defined in main.c)

/* Example 1: Basic alarm patterns ------------------------------------------*/
void alarm_example_basic_patterns(void)
{
    printf("\n=== RTOS Alarm Example 1: Basic Patterns ===\r\n");
    
    // Initialize buzzer
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = BUZZER_DEFAULT_FREQUENCY,
        .duty_cycle = BUZZER_DEFAULT_DUTY_CYCLE
    };
    Buzzer_Init(&config);
    
    // Initialize alarm system
    if (!BuzzerAlarm_Init()) {
        printf("ERROR: Failed to initialize alarm system\r\n");
        return;
    }
    printf("✓ Alarm system initialized\r\n");
    
    // Single beep - NON-BLOCKING!
    printf("Starting single beep (non-blocking)...\r\n");
    BuzzerAlarm_StartPattern(ALARM_BEEP_ONCE);
    osDelay(500);  // Task can do other work here!
    
    // Double beep
    printf("Starting double beep...\r\n");
    BuzzerAlarm_StartPattern(ALARM_BEEP_DOUBLE);
    osDelay(500);
    
    // Triple beep
    printf("Starting triple beep...\r\n");
    BuzzerAlarm_StartPattern(ALARM_BEEP_TRIPLE);
    osDelay(1000);
    
    printf("✓ Basic patterns completed\n\r\n");
}

/* Example 2: Continuous alarms (infinite repeat) ---------------------------*/
void alarm_example_continuous(void)
{
    printf("\n=== RTOS Alarm Example 2: Continuous Alarms ===\r\n");
    
    // Warning alarm - runs forever until stopped
    printf("Starting warning alarm (infinite)...\r\n");
    BuzzerAlarm_StartPattern(ALARM_WARNING);
    printf("  Doing other work while alarm runs...\r\n");
    osDelay(3000);  // Alarm keeps running!
    BuzzerAlarm_Stop();
    printf("  Stopped warning alarm\r\n");
    osDelay(500);
    
    // Alert alarm
    printf("Starting alert alarm (infinite)...\r\n");
    BuzzerAlarm_StartPattern(ALARM_ALERT);
    osDelay(2000);
    BuzzerAlarm_Stop();
    printf("  Stopped alert alarm\r\n");
    osDelay(500);
    
    // Critical alarm
    printf("Starting critical alarm (infinite)...\r\n");
    BuzzerAlarm_StartPattern(ALARM_CRITICAL);
    osDelay(2000);
    BuzzerAlarm_Stop();
    printf("  Stopped critical alarm\r\n");
    
    printf("✓ Continuous alarms test completed\n\r\n");
}

/* Example 3: Pulse patterns ------------------------------------------------*/
void alarm_example_pulse(void)
{
    printf("\n=== RTOS Alarm Example 3: Pulse Patterns ===\r\n");
    
    // Slow pulse (500ms on/off)
    printf("Starting slow pulse...\r\n");
    BuzzerAlarm_StartPattern(ALARM_PULSE_SLOW);
    osDelay(3000);
    BuzzerAlarm_Stop();
    osDelay(500);
    
    // Fast pulse (200ms on/off)
    printf("Starting fast pulse...\r\n");
    BuzzerAlarm_StartPattern(ALARM_PULSE_FAST);
    osDelay(2000);
    BuzzerAlarm_Stop();
    
    printf("✓ Pulse patterns test completed\n\r\n");
}

/* Example 4: Custom alarm configuration ------------------------------------*/
void alarm_example_custom(void)
{
    printf("\n=== RTOS Alarm Example 4: Custom Alarms ===\r\n");
    
    // Custom pattern 1: SOS signal (3 short, 3 long, 3 short)
    printf("Custom pattern: SOS signal\r\n");
    
    // Short beeps (3x)
    AlarmConfig_t sos_short = {
        .pattern = ALARM_CUSTOM,
        .frequency_hz = 4000,
        .duty_cycle = 60,
        .on_time_ms = 100,
        .off_time_ms = 100,
        .repeat_count = 3
    };
    BuzzerAlarm_StartCustom(&sos_short);
    osDelay(800);
    
    // Long beeps (3x)
    AlarmConfig_t sos_long = {
        .pattern = ALARM_CUSTOM,
        .frequency_hz = 4000,
        .duty_cycle = 60,
        .on_time_ms = 300,
        .off_time_ms = 100,
        .repeat_count = 3
    };
    BuzzerAlarm_StartCustom(&sos_long);
    osDelay(1500);
    
    // Short beeps again (3x)
    BuzzerAlarm_StartCustom(&sos_short);
    osDelay(1000);
    
    // Custom pattern 2: Accelerating beeps
    printf("Custom pattern: Accelerating beeps\r\n");
    
    uint32_t intervals[] = {500, 400, 300, 200, 100};
    for (int i = 0; i < 5; i++) {
        AlarmConfig_t accel = {
            .pattern = ALARM_CUSTOM,
            .frequency_hz = 4000 + (i * 200),  // Increasing frequency
            .duty_cycle = 50,
            .on_time_ms = 100,
            .off_time_ms = intervals[i],
            .repeat_count = 1
        };
        BuzzerAlarm_StartCustom(&accel);
        osDelay(intervals[i] + 200);
    }
    
    printf("✓ Custom alarms test completed\n\r\n");
}

/* Example 5: Zone-based alarms (fence integration) -------------------------*/
void alarm_example_zone_based(void)
{
    printf("\n=== RTOS Alarm Example 5: Zone-based Alarms ===\r\n");
    
    // Simulate cow moving through zones
    zone_t zones[] = {
        GREEN_ZONE,
        LIGHT_BLUE_ZONE,
        BLUE_ZONE,
        DARK_BLUE_ZONE,
        YELLOW_ZONE,
        RED_ZONE,
        BLACK_ZONE
    };
    
    const char* zone_names[] = {
        "GREEN (Safe)",
        "LIGHT_BLUE (Warning 1)",
        "BLUE (Warning 2)",
        "DARK_BLUE (Warning 3)",
        "YELLOW (Alert)",
        "RED (Critical)",
        "BLACK (Escape)"
    };
    
    for (int i = 0; i < 7; i++) {
        printf("Zone: %s\r\n", zone_names[i]);
        BuzzerAlarm_StartZone(zones[i]);
        osDelay(2000);  // Alarm runs in background!
    }
    
    printf("✓ Zone-based alarms test completed\n\r\n");
}

/* Example 6: Alarm state monitoring ----------------------------------------*/
void alarm_example_state_monitoring(void)
{
    printf("\n=== RTOS Alarm Example 6: State Monitoring ===\r\n");
    
    // Start an alarm and monitor its state
    printf("Starting warning alarm...\r\n");
    BuzzerAlarm_StartPattern(ALARM_WARNING);
    
    const AlarmState_t* state = BuzzerAlarm_GetState();
    printf("Alarm state:\r\n");
    printf("  Active: %s\r\n", state->active ? "Yes" : "No");
    printf("  Running: %s\r\n", state->running ? "Yes" : "No");
    printf("  Pattern: %d\r\n", state->pattern);
    printf("  Repetitions: %u\r\n", state->repetitions);
    
    osDelay(2000);
    
    // Stop and check state
    BuzzerAlarm_Stop();
    state = BuzzerAlarm_GetState();
    printf("\nAfter stopping:\r\n");
    printf("  Active: %s\r\n", state->active ? "Yes" : "No");
    printf("  IsActive(): %s\r\n", BuzzerAlarm_IsActive() ? "Yes" : "No");
    
    printf("✓ State monitoring test completed\n\r\n");
}

/* Example 7: Multi-tasking demonstration -----------------------------------*/
void alarm_example_multitasking(void)
{
    printf("\n=== RTOS Alarm Example 7: Multi-tasking ===\r\n");
    printf("This demonstrates the MAIN BENEFIT of RTOS alarms:\r\n");
    printf("Your task can do other work while alarms run!\n\r\n");
    
    // Start a long-running alarm
    printf("Starting continuous alarm in background...\r\n");
    BuzzerAlarm_StartPattern(ALARM_PULSE_SLOW);
    
    // Simulate task doing other work
    printf("Task is now FREE to do other work:\r\n");
    for (int i = 1; i <= 5; i++) {
        printf("  Working... step %d/5\r\n", i);
        osDelay(500);  // Alarm keeps running during this delay!
    }
    
    printf("Task work completed, stopping alarm\r\n");
    BuzzerAlarm_Stop();
    
    printf("\n✓ Multi-tasking demonstration completed\r\n");
    printf("Notice: No blocking in your code!\n\r\n");
}

/* Run all examples ---------------------------------------------------------*/
void alarm_run_all_examples(void)
{
    printf("\r\n");
    printf("╔════════════════════════════════════════════════════════════╗\r\n");
    printf("║     RTOS BUZZER ALARM - COMPREHENSIVE EXAMPLES             ║\r\n");
    printf("║     (Non-blocking, FreeRTOS-based patterns)                ║\r\n");
    printf("╚════════════════════════════════════════════════════════════╝\r\n");
    
    alarm_example_basic_patterns();
    alarm_example_continuous();
    alarm_example_pulse();
    alarm_example_custom();
    alarm_example_zone_based();
    alarm_example_state_monitoring();
    alarm_example_multitasking();
    
    // Cleanup
    BuzzerAlarm_DeInit();
    
    printf("╔════════════════════════════════════════════════════════════╗\r\n");
    printf("║          ALL RTOS ALARM EXAMPLES COMPLETED                 ║\r\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\r\n");
}
