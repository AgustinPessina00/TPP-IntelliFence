
#include "stimulusTask.h"

void stimulusTask(void *argument) {
    /* USER CODE BEGIN StartStimulusTask */
    /* Infinite loop */
    zone_t zone;

    for (;;) 
    {
        if (xQueueReceive(StimulusQueueHandle, &zone, portMAX_DELAY) == pdPASS)
        {
            switch (zone)
            {
                case BLUE_ZONE:
                    activateSoundStimulus();
                    break;

                case YELLOW_ZONE:
                    activateSoundStimulus();
                    activateVibrationStimulus();
                    break;

                case RED_ZONE:
                    activateShockStimulus();
                    break;
            }
        }
    }
    /* USER CODE END StartStimulusTask */
}
