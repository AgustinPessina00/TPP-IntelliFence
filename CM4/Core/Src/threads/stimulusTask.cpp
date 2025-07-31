#include "stimulusTask.h"
#include "messages.h"
//#include "main.h"
extern osMessageQueueId_t stimulusQueueHandle;
extern TIM_HandleTypeDef htim1; // BUZZER
extern LED_ELECTRICAL_Pin;
extern GPIOB;
extern TIM_HandleTypeDef htim16;  // VIB_MOTOR_R
extern TIM_HandleTypeDef htim17;  // VIB_MOTOR_L

static zone_t currentZone = GREEN_ZONE;
static zone_t previousZone = GREEN_ZONE;
static uint16_t vibrationFreq = 100;  // Frecuencia efectiva para motores
static uint8_t vibrationDuty = 90;  // Duty Cycle para motores

void stimulusTask(void *argument) {
    Message* msg;
    uint8_t duty;
    while (1) {
        if (osMessageQueueGet(stimulusQueueHandle, &msg, NULL, 0) == osOK) {
            if (msg->id == MSG_ID_ZONE_CHANGE) {
                currentZone = (zone_t) msg->payload[0];
                delete msg;
            }
        }
        // Configurar PWM si corresponde
        if (previousZone != currentZone) {
            switch (currentZone) {
            case LIGHT_BLUE_ZONE:
                duty = 30;
                configureBuzzerPWM(duty);
                stopVibration();
                stopShock();
                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(float));
                std::memcpy(msgToSend->payload, &(duty), sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case BLUE_ZONE:
                duty = 50;
                configureBuzzerPWM(duty);
                stopVibration();
                stopShock();
                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM,sizeof(float));
                std::memcpy(msgToSend->payload, &(duty), sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case DARK_BLUE_ZONE:
                duty = 70;
                configureBuzzerPWM(duty);
                stopVibration();
                stopShock();
                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM,sizeof(float));
                std::memcpy(msgToSend->payload, &(duty), sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case YELLOW_ZONE:
                duty = 90;
                configureBuzzerPWM(duty);
                configureVibrationPWM(vibrationDuty, vibrationDuty);
                stopShock();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM,sizeof(float));
                std::memcpy(msgToSend->payload, &(duty), sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, ,2 * sizeof(float));
                std::memcpy(msgToSend->payload, &vibrationDuty, sizeof(float));
                std::memcpy(msgToSend->payload + sizeof(float), &vibrationDuty, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case RED_ZONE:
                stopBuzzer();
                stopVibration();
                activateShockStimulus();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, 0);
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case BLACK_ZONE:
                stopBuzzer();
                stopVibration();
                stopShock();
                sendScapedMessage();
                break;
            default:
                stopBuzzer();
                stopVibration();
                stopShock();
                break;
            }
        }
        previousZone = currentZone;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void configureBuzzerPWM(uint8_t dutyPercent) {
    const uint32_t freqHz = 4000;  // frecuencia óptima del ST-0402T
    uint32_t clk = HAL_RCC_GetPCLK2Freq();
    uint32_t period = clk / freqHz;
    uint32_t pulse = (period * dutyPercent) / 100;

    __HAL_TIM_SET_AUTORELOAD(&htim1, period - 1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, pulse);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
}

void stopBuzzer(void) {
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
}

void configureVibrationPWM(uint8_t leftDutyPercent, uint8_t rightDutyPercent) {
    uint32_t clk = HAL_RCC_GetPCLK1Freq();  // Asume que TIM16 y TIM17 están en PCLK1
    uint32_t period = clk / vibrationFreq;

    // Motor izquierdo - TIM17_CH1
    __HAL_TIM_SET_AUTORELOAD(&htim17, period - 1);
    __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, (period * leftDutyPercent) / 100);
    HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

    // Motor derecho - TIM16_CH1
    __HAL_TIM_SET_AUTORELOAD(&htim16, period - 1);
    __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, (period * rightDutyPercent) / 100);
    HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
}

void stopVibration(void) {
    HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);  // Left
    HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);  // Right
}

void activateShockStimulus(void) {
    HAL_GPIO_WritePin(GPIOB, LED_ELECTRICAL_Pin, GPIO_PIN_SET);
}

void stopShock(void) {
    HAL_GPIO_WritePin(GPIOB, LED_ELECTRICAL_Pin, GPIO_PIN_RESET);
}
