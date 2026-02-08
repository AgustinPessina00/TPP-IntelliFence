/**
 ******************************************************************************
 * @file           : buzzer_example.c
 * @brief          : Example usage of buzzer module
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "buzzer.h"
#include "main.h"
#include <stdio.h>

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;  // Timer handle (defined in main.c)

/* Example 1: Basic initialization and on/off control ----------------------*/
void buzzer_example_basic(void)
{
    printf("\n=== Buzzer Example 1: Basic Control ===\r\n");
    
    // Configure buzzer with default settings (4 kHz, 50% duty cycle)
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = BUZZER_DEFAULT_FREQUENCY,  // 4000 Hz
        .duty_cycle = BUZZER_DEFAULT_DUTY_CYCLE    // 50%
    };
    
    // Initialize buzzer
    BuzzerResult_t result = Buzzer_Init(&config);
    if (result != BUZZER_OK) {
        printf("ERROR: Buzzer initialization failed\r\n");
        return;
    }
    printf("✓ Buzzer initialized (4 kHz, 50%% duty)\r\n");
    
    // Turn buzzer ON
    printf("Turning buzzer ON...\r\n");
    Buzzer_On();
    HAL_Delay(1000);  // 1 second
    
    // Turn buzzer OFF
    printf("Turning buzzer OFF...\r\n");
    Buzzer_Off();
    HAL_Delay(500);
    
    // Toggle buzzer
    printf("Toggling buzzer ON...\r\n");
    Buzzer_Toggle();
    HAL_Delay(1000);
    
    printf("Toggling buzzer OFF...\r\n");
    Buzzer_Toggle();
    
    printf("✓ Basic control test completed\n\r\n");
}

/* Example 2: Frequency control ---------------------------------------------*/
void buzzer_example_frequency(void)
{
    printf("\n=== Buzzer Example 2: Frequency Control ===\r\n");
    
    // Initialize with default config
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 2000,
        .duty_cycle = 50
    };
    
    Buzzer_Init(&config);
    
    // Test different frequencies
    uint32_t frequencies[] = {2000, 3000, 4000, 5000, 6000};
    const char* notes[] = {"Low", "Medium-Low", "Nominal", "Medium-High", "High"};
    
    for (int i = 0; i < 5; i++) {
        printf("Playing %s frequency: %lu Hz\r\n", notes[i], frequencies[i]);
        
        Buzzer_SetFrequency(frequencies[i]);
        Buzzer_On();
        HAL_Delay(500);
        Buzzer_Off();
        HAL_Delay(200);
    }
    
    printf("✓ Frequency sweep test completed\n\r\n");
}

/* Example 3: Volume (duty cycle) control -----------------------------------*/
void buzzer_example_volume(void)
{
    printf("\n=== Buzzer Example 3: Volume Control ===\r\n");
    
    // Initialize with default config
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 4000,
        .duty_cycle = 25
    };
    
    Buzzer_Init(&config);
    
    // Test different volumes (duty cycles)
    uint8_t volumes[] = {10, 25, 50, 75, 90};
    const char* levels[] = {"Very Quiet", "Quiet", "Medium", "Loud", "Very Loud"};
    
    for (int i = 0; i < 5; i++) {
        printf("Volume %s: %u%% duty cycle\r\n", levels[i], volumes[i]);
        
        Buzzer_SetDutyCycle(volumes[i]);
        Buzzer_On();
        HAL_Delay(500);
        Buzzer_Off();
        HAL_Delay(200);
    }
    
    printf("✓ Volume control test completed\n\r\n");
}

/* Example 4: Beep functions ------------------------------------------------*/
void buzzer_example_beep(void)
{
    printf("\n=== Buzzer Example 4: Beep Functions ===\r\n");
    
    // Initialize buzzer
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 4000,
        .duty_cycle = 50
    };
    
    Buzzer_Init(&config);
    
    // Single beep (4 kHz, 50%, 200ms)
    printf("Playing single beep...\r\n");
    Buzzer_Beep(4000, 50, 200);
    HAL_Delay(500);
    
    // Warning beep (3 kHz, 75%, 500ms)
    printf("Playing warning beep...\r\n");
    Buzzer_Beep(3000, 75, 500);
    HAL_Delay(500);
    
    // Beep sequence (3 beeps)
    printf("Playing beep sequence (3 beeps)...\r\n");
    Buzzer_BeepSequence(3, 100, 100);  // 3 beeps, 100ms on, 100ms off
    HAL_Delay(500);
    
    // Long beep sequence (5 beeps)
    printf("Playing long beep sequence (5 beeps)...\r\n");
    Buzzer_BeepSequence(5, 150, 150);
    
    printf("✓ Beep functions test completed\n\r\n");
}

/* Example 5: Real-world alarm patterns ------------------------------------*/
void buzzer_example_alarm_patterns(void)
{
    printf("\n=== Buzzer Example 5: Alarm Patterns ===\r\n");
    
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 4000,
        .duty_cycle = 50
    };
    
    Buzzer_Init(&config);
    
    // Pattern 1: Warning (approaching fence)
    printf("Pattern 1: Warning alarm\r\n");
    for (int i = 0; i < 3; i++) {
        Buzzer_Beep(3500, 40, 100);
        HAL_Delay(100);
    }
    HAL_Delay(500);
    
    // Pattern 2: Alert (near fence limit)
    printf("Pattern 2: Alert alarm\r\n");
    for (int i = 0; i < 5; i++) {
        Buzzer_Beep(4000, 60, 80);
        HAL_Delay(80);
    }
    HAL_Delay(500);
    
    // Pattern 3: Critical (fence breach)
    printf("Pattern 3: Critical alarm\r\n");
    for (int i = 0; i < 10; i++) {
        Buzzer_Beep(5000, 80, 50);
        HAL_Delay(50);
    }
    HAL_Delay(500);
    
    // Pattern 4: Continuous warning
    printf("Pattern 4: Continuous warning (2 seconds)\r\n");
    Buzzer_SetParams(4500, 70);
    Buzzer_On();
    HAL_Delay(2000);
    Buzzer_Off();
    
    printf("✓ Alarm patterns test completed\n\r\n");
}

/* Example 6: Zone-based stimulus control -----------------------------------*/
void buzzer_example_zone_stimulus(void)
{
    printf("\n=== Buzzer Example 6: Zone-based Stimulus ===\r\n");
    
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 4000,
        .duty_cycle = 50
    };
    
    Buzzer_Init(&config);
    
    // Simulate different fence zones
    
    // GREEN ZONE - No stimulus
    printf("GREEN ZONE - No stimulus\r\n");
    HAL_Delay(500);
    
    // LIGHT_BLUE ZONE - Soft beep
    printf("LIGHT_BLUE ZONE - Soft warning\r\n");
    Buzzer_Beep(2500, 20, 100);
    HAL_Delay(500);
    
    // BLUE ZONE - Medium beep
    printf("BLUE ZONE - Medium warning\r\n");
    Buzzer_Beep(3000, 35, 150);
    HAL_Delay(500);
    
    // DARK_BLUE ZONE - Stronger beep
    printf("DARK_BLUE ZONE - Strong warning\r\n");
    Buzzer_Beep(3500, 50, 200);
    HAL_Delay(500);
    
    // YELLOW ZONE - Intense beep sequence
    printf("YELLOW ZONE - Intense warning\r\n");
    Buzzer_BeepSequence(3, 150, 100);
    HAL_Delay(500);
    
    // RED ZONE - Critical alert
    printf("RED ZONE - Critical alert\r\n");
    for (int i = 0; i < 5; i++) {
        Buzzer_Beep(5000, 75, 100);
        HAL_Delay(50);
    }
    HAL_Delay(500);
    
    // BLACK ZONE - Escape detected (continuous alarm)
    printf("BLACK ZONE - Escape alarm (continuous 3 seconds)\r\n");
    Buzzer_SetParams(4500, 80);
    Buzzer_On();
    HAL_Delay(3000);
    Buzzer_Off();
    
    printf("✓ Zone-based stimulus test completed\n\r\n");
}

/* Example 7: State monitoring ----------------------------------------------*/
void buzzer_example_state_monitoring(void)
{
    printf("\n=== Buzzer Example 7: State Monitoring ===\r\n");
    
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 4000,
        .duty_cycle = 50
    };
    
    Buzzer_Init(&config);
    
    // Get initial state
    const BuzzerState_t* state = Buzzer_GetState();
    printf("Initial state:\r\n");
    printf("  Initialized: %s\r\n", state->initialized ? "Yes" : "No");
    printf("  Enabled: %s\r\n", state->enabled ? "Yes" : "No");
    printf("  Frequency: %lu Hz\r\n", state->current_freq);
    printf("  Duty Cycle: %u%%\r\n", state->current_duty);
    printf("  ARR value: %lu\r\n", state->arr_value);
    printf("  CCR value: %lu\n\r\n", state->ccr_value);
    
    // Change parameters
    printf("Changing to 5 kHz, 75%% duty...\r\n");
    Buzzer_SetParams(5000, 75);
    
    state = Buzzer_GetState();
    printf("Updated state:\r\n");
    printf("  Frequency: %lu Hz\r\n", state->current_freq);
    printf("  Duty Cycle: %u%%\r\n", state->current_duty);
    printf("  ARR value: %lu\r\n", state->arr_value);
    printf("  CCR value: %lu\n\r\n", state->ccr_value);
    
    // Turn on and check state
    Buzzer_On();
    state = Buzzer_GetState();
    printf("After turning ON:\r\n");
    printf("  Enabled: %s\r\n", state->enabled ? "Yes" : "No");
    printf("  IsEnabled(): %s\r\n", Buzzer_IsEnabled() ? "Yes" : "No");
    
    HAL_Delay(1000);
    
    // Turn off and check state
    Buzzer_Off();
    state = Buzzer_GetState();
    printf("After turning OFF:\r\n");
    printf("  Enabled: %s\r\n", state->enabled ? "Yes" : "No");
    printf("  IsEnabled(): %s\r\n", Buzzer_IsEnabled() ? "Yes" : "No");
    
    printf("✓ State monitoring test completed\n\r\n");
}

/* Run all examples ---------------------------------------------------------*/
void buzzer_run_all_examples(void)
{
    printf("\r\n");
    printf("╔════════════════════════════════════════════════════════════╗\r\n");
    printf("║        BUZZER MODULE - COMPREHENSIVE EXAMPLES              ║\r\n");
    printf("╚════════════════════════════════════════════════════════════╝\r\n");
    
    buzzer_example_basic();
    buzzer_example_frequency();
    buzzer_example_volume();
    buzzer_example_beep();
    buzzer_example_alarm_patterns();
    buzzer_example_zone_stimulus();
    buzzer_example_state_monitoring();
    
    printf("╔════════════════════════════════════════════════════════════╗\r\n");
    printf("║             ALL EXAMPLES COMPLETED                         ║\r\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\r\n");
}
