/**
 ******************************************************************************
 * @file           : stimulusTask.h
 * @brief          : Stimulus Task header - zone-based stimulus control
 * @author         : TPP-IntelliFence Team
 * @date           : November 24, 2025
 ******************************************************************************
 * @attention
 *
 * Stimulus task manages zone-based feedback:
 * - Sound (Buzzer)
 * - Visual (LED_BLUE)
 * - Vibration (disabled for now)
 * - Electric (disabled for now)
 *
 ******************************************************************************
 */

#ifndef STIMULUS_TASK_H
#define STIMULUS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "zone.h"
/* Exported types ------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------*/

/**
 * @brief Stimulus task main function
 * @param argument Task parameters (unused)
 */
void stimulusTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* STIMULUS_TASK_H */
