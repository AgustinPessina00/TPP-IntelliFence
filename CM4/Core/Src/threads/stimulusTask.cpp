
#include "stimulusTask.h"

void stimulusTask(void *argument) {
    /* USER CODE BEGIN StartStimulusTask */
    /* Infinite loop */
    zone_t zone;

    for (;;) 
    {
        if (xQueueReceive(StimulusQueueHandle, &zone, 0) == pdPASS)
        {
            switch (zone)
            {
                case BLUE_ZONE:
                    activateSoundStimulus(); //TODO: crear función. 
                    break;

                case YELLOW_ZONE:
                    activateSoundStimulus(); 
                    activateVibrationStimulus(); //TODO: crear función. 
                    break;

                case RED_ZONE:
                    activateShockStimulus(); //TODO: crear función.
                    break;

                case BLACK_ZONE:
                    sendScapedMessage(); //TODO: crear función.
                    break;
                default:
            }
        }

        osDelay(50);  // esperar 50 ms antes de volver a revisar
    }
    /* USER CODE END StartStimulusTask */
}
