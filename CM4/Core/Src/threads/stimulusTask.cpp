
/**
 ******************************************************************************
 * @file           : stimulusTask.cpp
 * @brief          : Stimulus Task implementation - zone-based stimulus control
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 */

#include "threads/stimulusTask.h"
#include "Modules/Buzzer/buzzer.h"
#include "Modules/Buzzer/buzzer_alarm.h"
#include "Modules/VibrMotor/vibr_motor.h"
#include "Modules/VibrMotor/vibr_motor_alarm.h"
#include "Modules/Messages/EmbeddedMessage.h"
#include "projdefs.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"

/* External handles ----------------------------------------------------------*/
extern osMessageQueueId_t stimulusQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;
extern TIM_HandleTypeDef htim1;   // BUZZER
extern TIM_HandleTypeDef htim16;  // VIB_MOTOR_R
extern TIM_HandleTypeDef htim17;  // VIB_MOTOR_L

/* Private variables ---------------------------------------------------------*/
static zone_t currentZone = GREEN_ZONE;
static zone_t previousZone = GREEN_ZONE;
static bool buzzerInitialized = false;
static bool vibrMotorInitialized = false;
static bool alarmInitialized = false;
static bool vibrAlarmInitialized = false;
static bool waitingForRedZoneAlarm = false;
static uint32_t redZoneAlarmStartTime = 0;

/* Private function prototypes -----------------------------------------------*/
static void initializeBuzzer(void);
static void initializeVibrMotor(void);
static void handleZoneChange(zone_t newZone);
static void sendStimulusFeedback(void);

/**
 * @brief Initialize buzzer and alarm modules
 */
static void initializeBuzzer(void) {
    if (!buzzerInitialized) {
        BuzzerConfig_t config = {
            .htim = &htim1,
            .channel = TIM_CHANNEL_3,
            .frequency_hz = BUZZER_DEFAULT_FREQUENCY,  // 4000 Hz
            .duty_cycle = 0  // Start off
        };
        
        if (Buzzer_Init(&config) == BUZZER_OK) {
            buzzerInitialized = true;
            // LED_BLUE is already initialized in MX_GPIO_Init() as output
            // Start with LED off
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            
            // Initialize RTOS alarm system
            if (BuzzerAlarm_Init()) {
                alarmInitialized = true;
            }
        }
    }
}

/**
 * @brief Initialize vibration motor and alarm modules
 */
static void initializeVibrMotor(void) {
    if (!vibrMotorInitialized) {
        // **CRITICAL FIX**: Ensure PB9 is configured as Alternate Function for TIM17
        // This is necessary because something may reconfigure it as GPIO output
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF14_TIM17;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        VibrMotorConfig_t config = {
            .htim_left = &htim17,
            .htim_right = &htim16,
            .channel_left = TIM_CHANNEL_1,
            .channel_right = TIM_CHANNEL_1,
            .frequency_hz = VIBR_MOTOR_DEFAULT_FREQUENCY,  // 1000 Hz
            .duty_cycle = 0  // Start off
        };
        
        if (VibrMotor_Init(&config) == VIBR_MOTOR_OK) {
            vibrMotorInitialized = true;
            
            // Initialize RTOS alarm system
            if (VibrMotorAlarm_Init()) {
                vibrAlarmInitialized = true;
            }
        }
    }
}

/**
 * @brief Send stimulus feedback message to FSM
 */
static void sendStimulusFeedback(void) {
    EmbeddedMessage_t* msgToSend = MessagePool_Allocate();
    if (msgToSend != NULL) {
        EmbeddedMessage_Create(msgToSend, 
                             MSG_ID_STIMULUS_FEEDBACK, 
                             MODULE_STIMULUS, 
                             MODULE_FSM);
        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
    }
}

/**
 * @brief Handle zone change and apply appropriate stimulus using RTOS alarms
 * @param newZone New fence zone
 */
static void handleZoneChange(zone_t newZone) {
    if (!alarmInitialized) {
        // Fallback to old method if alarm not initialized
        switch (newZone) {
            case GREEN_ZONE:
                Buzzer_Off();
                HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
                break;
            default:
                Buzzer_SetParams(4000, 50);
                Buzzer_On();
                break;
        }
        sendStimulusFeedback();
        return;
    }
    // Use RTOS alarm system - non-blocking!
    switch (newZone) {
        case GREEN_ZONE:
            // Safe zone - stop all alarms
            BuzzerAlarm_Stop();
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_Stop();
            }
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case LIGHT_BLUE_ZONE:
            // Warning zone 1 - slow pulse (500ms on/off) + vibration motors 20% duty
            BuzzerAlarm_StartZone(LIGHT_BLUE_ZONE);
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_StartZone(LIGHT_BLUE_ZONE);
            }
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case BLUE_ZONE:
            // Warning zone 2 - warning pattern (100ms on, 1s off) + vibration motors 50% duty
            BuzzerAlarm_StartZone(BLUE_ZONE);
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_StartZone(BLUE_ZONE);
            }
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case DARK_BLUE_ZONE:
            // Warning zone 3 - fast pulse (200ms on/off) + vibration motors 65% duty
            BuzzerAlarm_StartZone(DARK_BLUE_ZONE);
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_StartZone(DARK_BLUE_ZONE);
            }
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case YELLOW_ZONE:
            // Alert zone - alert pattern (80ms on, 200ms off) + vibration motors alternating
            BuzzerAlarm_StartZone(YELLOW_ZONE);
            
            // Start vibration motor alarm (alternating left-right)
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_StartZone(YELLOW_ZONE);
            }
            
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case RED_ZONE:
            // Critical zone - play alarm sequence FIRST, then LED
            // Stop vibration motors in RED zone
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_Stop();
            }
            
            // Create custom alarm with 10 repetitions (50ms on/off = 1s total)
            {
                AlarmConfig_t redAlarm = {
                    .pattern = ALARM_CUSTOM,
                    .frequency_hz = 5000,
                    .duty_cycle = 80,
                    .on_time_ms = 50,
                    .off_time_ms = 50,
                    .repeat_count = 10  // 10 beeps = 1 second total
                };
                BuzzerAlarm_StartCustom(&redAlarm);
                waitingForRedZoneAlarm = true;
                redZoneAlarmStartTime = osKernelGetTickCount();
                HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);  // LED off during alarm
            }
            break;
            
        case BLACK_ZONE:
            // Escape zone - silent mode (all off)
            BuzzerAlarm_Stop();
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_Stop();
            }
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            // TODO: Send escape alert via LoRa
            break;
            
        default:
            // Unknown zone - stop everything
            BuzzerAlarm_Stop();
            if (vibrAlarmInitialized) {
                VibrMotorAlarm_Stop();
            }
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
    }
    
    
}

/**
 * @brief Stimulus task main function
 * @param argument Task parameters (unused)
 */
void stimulusTask(void *argument) {
    EmbeddedMessage_t* msg = NULL;
    
    // Initialize buzzer module
    initializeBuzzer();
    
    // Initialize vibration motor module
    initializeVibrMotor();
    
    // // Hardware test: vibrate motors individually to verify functionality
    // if (vibrMotorInitialized) {
    //     RTOS_LOG_INFO("[STIMULUS] *** HARDWARE TEST START ***\r\n");
    //     RTOS_LOG_INFO("[STIMULUS] TIM17 (LEFT) ARR=%lu CCR1=%lu\r\n", 
    //                  htim17.Instance->ARR, htim17.Instance->CCR1);
    //     RTOS_LOG_INFO("[STIMULUS] TIM16 (RIGHT) ARR=%lu CCR1=%lu\r\n", 
    //                  htim16.Instance->ARR, htim16.Instance->CCR1);
        
    //     RTOS_LOG_INFO("[STIMULUS] Testing LEFT motor (PB9/TIM17)...\r\n");
    //     VibrMotor_SetParams(MOTOR_LEFT, 1000, 80);  // 1kHz, 80% duty
    //     RTOS_LOG_INFO("[STIMULUS] After SetParams: TIM17 ARR=%lu CCR1=%lu\r\n", 
    //                  htim17.Instance->ARR, htim17.Instance->CCR1);
    //     VibrMotor_On(MOTOR_LEFT);
    //     RTOS_LOG_INFO("[STIMULUS] After On: TIM17 CCR1=%lu CR1=0x%08lX CCER=0x%08lX\r\n", 
    //                  htim17.Instance->CCR1, htim17.Instance->CR1, htim17.Instance->CCER);
    //     osDelay(500);  // Vibrate for 500ms
    //     VibrMotor_Off(MOTOR_LEFT);
    //     RTOS_LOG_INFO("[STIMULUS] After Off: TIM17 CCR1=%lu\r\n", htim17.Instance->CCR1);
        
    //     osDelay(200);  // Small pause between motors
        
    //     RTOS_LOG_INFO("[STIMULUS] Testing RIGHT motor (PB8/TIM16)...\r\n");
    //     VibrMotor_SetParams(MOTOR_RIGHT, 1000, 80);  // 1kHz, 80% duty
    //     RTOS_LOG_INFO("[STIMULUS] After SetParams: TIM16 ARR=%lu CCR1=%lu\r\n", 
    //                  htim16.Instance->ARR, htim16.Instance->CCR1);
    //     VibrMotor_On(MOTOR_RIGHT);
    //     RTOS_LOG_INFO("[STIMULUS] After On: TIM16 CCR1=%lu CR1=0x%08lX CCER=0x%08lX\r\n", 
    //                  htim16.Instance->CCR1, htim16.Instance->CR1, htim16.Instance->CCER);
    //     osDelay(500);  // Vibrate for 500ms
    //     VibrMotor_Off(MOTOR_RIGHT);
    //     RTOS_LOG_INFO("[STIMULUS] After Off: TIM16 CCR1=%lu\r\n", htim16.Instance->CCR1);
        
    //     RTOS_LOG_INFO("[STIMULUS] *** HARDWARE TEST COMPLETED ***\r\n");
    //}
    
    static uint32_t stackMonitorCounter = 0;
    while (1) {
        // Monitorear stack cada ~10 segundos (cada 100 iteraciones × 100ms delay)
        if (++stackMonitorCounter >= 100) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            RTOS_LOG_INFO("[STIMULUS] Stack libre: %u words (%u bytes)\r\n", 
                         stackLeft, stackLeft * 4);
            stackMonitorCounter = 0;
        }
        
        // Check for zone change messages
        if (osMessageQueueGet(stimulusQueueHandle, &msg, NULL, 0) == osOK) {
            if (msg != NULL && msg->id == MSG_ID_ZONE_CHANGE) {
                // Extract zone from payload
                if (msg->length >= sizeof(uint8_t)) {
                    zone_t newZone = (zone_t)msg->payload[0];
                    currentZone = newZone;
                    sendStimulusFeedback();
                }
                
                // Free message back to pool
                MessagePool_Free(msg);
                msg = NULL;
            }
        }
        
        // Apply stimulus based on zone change
        if (previousZone != currentZone) {
            handleZoneChange(currentZone);
            previousZone = currentZone;
        }
        
        // Check if RED_ZONE alarm finished to turn on LED
        if (waitingForRedZoneAlarm) {
            if (!BuzzerAlarm_IsActive()) {
                // Alarm finished - turn on LED and play 2kHz continuous tone
                HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
                // Create custom alarm: 2kHz continuous for 1 second
                AlarmConfig_t finalTone = {
                    .pattern = ALARM_CUSTOM,
                    .frequency_hz = 2000,
                    .duty_cycle = 50,
                    .on_time_ms = 1000,  // 1 second continuous
                    .off_time_ms = 0,
                    .repeat_count = 1    // Play once
                };
                BuzzerAlarm_StartCustom(&finalTone);
                waitingForRedZoneAlarm = false;
            } else {
                // Safety timeout - if alarm takes too long (>2 seconds)
                uint32_t elapsed = osKernelGetTickCount() - redZoneAlarmStartTime;
                if (elapsed > 2000) {
                    BuzzerAlarm_Stop();
                    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
                    AlarmConfig_t finalTone = {
                        .pattern = ALARM_CUSTOM,
                        .frequency_hz = 2000,
                        .duty_cycle = 50,
                        .on_time_ms = 1000,
                        .off_time_ms = 0,
                        .repeat_count = 1
                    };
                    BuzzerAlarm_StartCustom(&finalTone);
                    waitingForRedZoneAlarm = false;
                }
            }
        }
        
        osDelay(1000);
    }
}