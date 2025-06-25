/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SCL_Pin GPIO_PIN_12
#define SCL_GPIO_Port GPIOA
#define LED_ELECTRICAL_Pin GPIO_PIN_15
#define LED_ELECTRICAL_GPIO_Port GPIOB
#define PWRGD_LDO1_Pin GPIO_PIN_14
#define PWRGD_LDO1_GPIO_Port GPIOC
#define SDA_Pin GPIO_PIN_11
#define SDA_GPIO_Port GPIOA
#define VIB_MOTOR_L_Pin GPIO_PIN_9
#define VIB_MOTOR_L_GPIO_Port GPIOB
#define ENABLE_LDO1_Pin GPIO_PIN_13
#define ENABLE_LDO1_GPIO_Port GPIOC
#define AO_BUZZER_Pin GPIO_PIN_10
#define AO_BUZZER_GPIO_Port GPIOA
#define VIB_MOTOR_R_Pin GPIO_PIN_8
#define VIB_MOTOR_R_GPIO_Port GPIOB
#define PWRGD_LDO2_Pin GPIO_PIN_3
#define PWRGD_LDO2_GPIO_Port GPIOC
#define BOOST_SHDN_N_Pin GPIO_PIN_13
#define BOOST_SHDN_N_GPIO_Port GPIOB
#define BOOST_FB_Pin GPIO_PIN_2
#define BOOST_FB_GPIO_Port GPIOB
#define BOOST_5V_CTRL_Pin GPIO_PIN_1
#define BOOST_5V_CTRL_GPIO_Port GPIOB
#define LPTIM1_OUT_Pin GPIO_PIN_1
#define LPTIM1_OUT_GPIO_Port GPIOC
#define GPS_RESET_N_Pin GPIO_PIN_0
#define GPS_RESET_N_GPIO_Port GPIOC
#define GPS_EXTINT_Pin GPIO_PIN_6
#define GPS_EXTINT_GPIO_Port GPIOA
#define GPS_TIMEPULSE_Pin GPIO_PIN_6
#define GPS_TIMEPULSE_GPIO_Port GPIOC
#define ENABLE_LDO2_Pin GPIO_PIN_1
#define ENABLE_LDO2_GPIO_Port GPIOA
#define SIG_ELECTRICAL_Pin GPIO_PIN_11
#define SIG_ELECTRICAL_GPIO_Port GPIOB
#define RX_Pin GPIO_PIN_3
#define RX_GPIO_Port GPIOA
#define TX_Pin GPIO_PIN_2
#define TX_GPIO_Port GPIOA
#define IMU_INT1_Pin GPIO_PIN_7
#define IMU_INT1_GPIO_Port GPIOA
#define IMU_INT1_EXTI_IRQn EXTI9_5_IRQn
#define LPTIM2_OUT_Pin GPIO_PIN_4
#define LPTIM2_OUT_GPIO_Port GPIOA
#define GPS_SAFEBOOT_N_Pin GPIO_PIN_5
#define GPS_SAFEBOOT_N_GPIO_Port GPIOA
#define IMU_INT2_Pin GPIO_PIN_8
#define IMU_INT2_GPIO_Port GPIOA
#define IMU_INT2_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
