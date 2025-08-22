
#include "threads/stimulusTask.h"

extern osMessageQueueId_t stimulusQueueHandle;
extern TIM_HandleTypeDef htim1; // BUZZER
//extern LED_ELECTRICAL_Pin;
//extern GPIOB;
extern TIM_HandleTypeDef htim16;  // VIB_MOTOR_R
extern TIM_HandleTypeDef htim17;  // VIB_MOTOR_L

static zone_t currentZone = GREEN_ZONE;
static zone_t previousZone = GREEN_ZONE;
static uint16_t vibrationFreq = 100;  // Frecuencia efectiva para motores
static uint8_t vibrationDuty = 90;  // Duty Cycle para motores

void stimulusTask(void *argument) {
    Message* msg;
    uint8_t buzzerDuty = 0;
    uint8_t soundStimulus = 0;  // 0=OFF 1=ON
    uint8_t vibrationStimulus = 0;
    uint8_t shockStimulus = 0;
    
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
                buzzerDuty = 30;
                soundStimulus = 1;
                vibrationStimulus = 0;
                shockStimulus = 0;

                configureBuzzerPWM(buzzerDuty);
                stopVibration();
                stopShock();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t) + sizeof(float));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                memcpy(msgToSend->payload + sizeof(uint8_t), &buzzerDuty, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case BLUE_ZONE:
                buzzerDuty = 50;
                soundStimulus = 1;
                vibrationStimulus = 0;
                shockStimulus = 0;

                configureBuzzerPWM(buzzerDuty);
                stopVibration();
                stopShock();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t) + sizeof(float));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                memcpy(msgToSend->payload + sizeof(uint8_t), &buzzerDuty, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case DARK_BLUE_ZONE:
                buzzerDuty = 70;
                soundStimulus = 1;
                vibrationStimulus = 0;
                shockStimulus = 0;
                configureBuzzerPWM(buzzerDuty);
                stopVibration();
                stopShock();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t) + sizeof(float));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                memcpy(msgToSend->payload + sizeof(uint8_t), &buzzerDuty, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case YELLOW_ZONE:
                buzzerDuty = 90;
                soundStimulus = 1;
                vibrationStimulus = 1;
                shockStimulus = 0;

                configureBuzzerPWM(buzzerDuty);
                configureVibrationPWM(vibrationDuty, vibrationDuty);
                stopShock();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t) + sizeof(float));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                memcpy(msgToSend->payload + sizeof(uint8_t), &buzzerDuty, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                memcpy(msgToSend->payload + sizeof(uint8_t), &vibrationDuty, sizeof(float));
                memcpy(msgToSend->payload + sizeof(uint8_t) + sizeof(float), &vibrationDuty, sizeof(float));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            case RED_ZONE:
                soundStimulus = 0;
                vibrationStimulus = 0;
                shockStimulus = 1;
                stopBuzzer();
                stopVibration();
                activateShockStimulus();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                break;
            case BLACK_ZONE:
                soundStimulus = 0;
                vibrationStimulus = 0;
                shockStimulus = 0;

                stopBuzzer();
                stopVibration();
                stopShock();
                sendScapedMessage();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
                break;
            default:
                soundStimulus = 0;
                vibrationStimulus = 0;
                shockStimulus = 0;

                stopBuzzer();
                stopVibration();
                stopShock();

                Message* msgToSend = new Message(MSG_ID_STIMULUS_SOUND_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &soundStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_VIBRATION_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &vibrationStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);

                Message* msgToSend = new Message(MSG_ID_STIMULUS_ELECTRIC_FEEDBACK, ModuleId_t::STIMULUS, ModuleId_t::FSM, sizeof(uint8_t));
                memcpy(msgToSend->payload, &shockStimulus, sizeof(uint8_t));
                osMessageQueuePut(dispatcherQueueHandle, msgToSend, 0, 0);
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
