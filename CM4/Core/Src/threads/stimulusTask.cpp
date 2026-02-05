
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
extern TIM_HandleTypeDef htim1;  // BUZZER

/* Private variables ---------------------------------------------------------*/
static zone_t currentZone = GREEN_ZONE;
static zone_t previousZone = GREEN_ZONE;
static bool buzzerInitialized = false;
static bool alarmInitialized = false;
static bool waitingForRedZoneAlarm = false;
static uint32_t redZoneAlarmStartTime = 0;

/* Private function prototypes -----------------------------------------------*/
static void initializeBuzzer(void);
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
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case LIGHT_BLUE_ZONE:
            // Warning zone 1 - slow pulse (500ms on/off)
            BuzzerAlarm_StartZone(LIGHT_BLUE_ZONE);
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case BLUE_ZONE:
            // Warning zone 2 - warning pattern (100ms on, 1s off)
            BuzzerAlarm_StartZone(BLUE_ZONE);
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case DARK_BLUE_ZONE:
            // Warning zone 3 - fast pulse (200ms on/off)
            BuzzerAlarm_StartZone(DARK_BLUE_ZONE);
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case YELLOW_ZONE:
            // Alert zone - alert pattern (80ms on, 200ms off)
            BuzzerAlarm_StartZone(YELLOW_ZONE);
            //HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            break;
            
        case RED_ZONE:
            // Critical zone - play alarm sequence FIRST, then LED
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
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
            // TODO: Send escape alert via LoRa
            break;
            
        default:
            // Unknown zone - stop everything
            BuzzerAlarm_Stop();
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
    
    static uint32_t stackMonitorCounter = 0;
    while (1) {
        // Monitorear stack cada ~10 segundos (cada 100 iteraciones × 100ms delay)
        if (++stackMonitorCounter >= 100) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            RTOS_LOG_INFO("[STIMULUS] Stack libre: %u words (%u bytes)\n", 
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
        
        osDelay(100);
    }
}