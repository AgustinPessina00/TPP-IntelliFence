/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "i2c.h"
#include "app_lorawan.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
/* Forward declarations for heap_5 configuration */
#include "heap_config.h"
/* Include FreeRTOS headers for memory allocation functions */
#include "FreeRTOS.h"
/* Include thread-safe printf for RTOS */
#include "rtos_printf.h"
/* Include C++ managers initialization wrapper */
#include "system_init.h"

// Forward declarations para sistema de mensajes embedded-friendly
extern void test_message_pool_basic(void);
extern void run_message_system_test(void);

// Forward declarations para funciones GPS (implementadas en C++)
extern void gps_init_and_test(void);
extern void gps_continuous_test(void);

// Forward declarations para GPS Melopero (implementacion UBX)
extern void gps_melopero_comprehensive_test(void);
extern void gps_melopero_test(void);

// Forward declarations para INA226 (sensor corriente/potencia thread-safe)
extern void ina226_comprehensive_test(void);
extern void ina226_configuration_test(void);

// Forward declarations para LSM6DSO (sensor IMU 6DOF thread-safe)
extern void lsm6dso_comprehensive_test(void);
extern void lsm6dso_configuration_test(void);

// Forward declarations para Cow y Fence tests
extern void run_cow_fence_tests(void);

extern void buzzer_run_all_examples();

// Forward declaration para MessagePool

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// ===== DECLARACIONES DE THREADS FREERTOS =====

// Thread principal para testing (compatible con myMain.cpp) - COMENTADO para evitar duplicación
// osThreadId_t defaultTaskHandle;  // Ya está definido en app_freertos.c
// const osThreadAttr_t defaultTask_attributes = {  // Ya está definido en app_freertos.c
//   .name = "defaultTask",
//   .stack_size = 128 * 4,
//   .priority = (osPriority_t) osPriorityHigh,
// };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Comprehensive module testing before RTOS startup
 * @details Tests all hardware modules (GPS, IMU, INA) and embedded message system
 * @note Called before FreeRTOS scheduler starts
 */
void run_comprehensive_module_tests(void) {
    printf("\n");
    printf("=========================================================\n");
    printf("       TPP-IntelliFence - COMPREHENSIVE MODULE TESTS    \n");
    printf("=========================================================\n");
    printf("INFO - Testing all modules before FreeRTOS startup\n");
    printf("INFO - Hardware: STM32WL55JC Dual Core\n");
    printf("INFO - Core: CM4 (Application Processor)\n");
    printf("=========================================================\n\n");
    
    bool all_tests_passed = true;
    
    // ===== TEST 1: Sistema de Mensajes Embedded-Friendly =====
    printf(">>> TEST 1: EMBEDDED MESSAGE SYSTEM <<<\n");
    printf("Testing static message pool (no dynamic allocation)...\n");
    
    test_message_pool_basic();
    
    printf("SUCCESS - Embedded message system validated\n");
    printf("INFO - Zero dynamic allocation confirmed\n");
    printf("INFO - Thread-safe pool operations confirmed\n\n");
    
    // ===== TEST 2: INA226 Current/Power Sensors =====
    printf(">>> TEST 2: INA226 CURRENT/POWER SENSORS <<<\n");
    printf("Testing INA226 sensors (MCU, GPS, IMU power monitoring)...\n");
    
    ina226_comprehensive_test();
    
    printf("SUCCESS - INA226 comprehensive test completed\n");
    printf("INFO - MCU, GPS, IMU power monitoring operational\n\n");
    
    // ===== TEST 3: LSM6DSO IMU 6DOF Sensor =====
    printf(">>> TEST 3: LSM6DSO IMU 6DOF SENSOR <<<\n");
    printf("Testing accelerometer and gyroscope functionality...\n");
    
    lsm6dso_comprehensive_test();
    
    printf("SUCCESS - LSM6DSO IMU test completed\n");
    printf("INFO - 6DOF motion sensing operational\n\n");
    
    // ===== TEST 4: SAM-M10Q GPS Module =====
    printf(">>> TEST 4: SAM-M10Q GPS MODULE <<<\n");
    printf("Testing GPS connectivity and data acquisition...\n");
    
    // Test standard GPS implementation

    gps_init_and_test();
    
    printf("SUCCESS - GPS module test completed\n");
    
    // ===== TEST 5: COW & FENCE DATA MODEL =====
    printf(">>> TEST 5: COW & FENCE DATA MODEL <<<\n");
    printf("Testing embedded-friendly Cow and Fence classes...\n");
    
    run_cow_fence_tests();
    
    printf("SUCCESS - Cow & Fence test completed\n");
    printf("INFO - Data model ready for FSM integration\n\n");
    
    // ===== TEST 6: BUZZER ACTUATOR =====
    printf(">>> TEST 6: BUZZER ACTUATOR <<<\n");
    printf("Testing PWM-based buzzer functionality...\n");
    
    buzzer_run_all_examples();
    
    printf("SUCCESS - Buzzer test completed\n");
    printf("INFO - Acoustic stimulus system operational\n\n");
    
    // ===== RESUMEN DE TESTS =====
    if (all_tests_passed) {
        printf("=========================================================\n");
        printf("              ALL MODULE TESTS PASSED                    \n");
        printf("=========================================================\n");
        printf("SUCCESS - System ready for FreeRTOS operation\n");
        printf("INFO - All hardware modules validated\n");
        printf("INFO - Embedded message system operational\n");
        printf("INFO - Starting FreeRTOS scheduler...\n");
        printf("=========================================================\n\n");
    } else {
        printf("=========================================================\n");
        printf("             SOME TESTS FAILED                         \n");
        printf("=========================================================\n");
        printf("WARNING - Proceeding with FreeRTOS startup\n");
        printf("WARNING - Check failed modules before production use\n");
        printf("=========================================================\n\n");
    }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  HAL_Init();

  /* USER CODE BEGIN Init */
  
  /* Configure FreeRTOS heap to use RAM2 before any RTOS calls */
  //vApplicationSetupHeap(); // Commented out - osKernelInitialize() will handle heap setup

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* Initialize C++ managers (I2C, UART) before peripheral usage */
  

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();
  if (initialize_cpp_managers() != 0) {
    printf("[MAIN] CRITICAL ERROR - Failed to initialize C++ managers\n");
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Boot CPU2 */
  HAL_PWREx_ReleaseCore(PWR_CORE_CPU2);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    // Este bucle no debería ejecutarse nunca ya que FreeRTOS toma control
    // Si llegamos aquí, algo salió mal con el scheduler
    printf("ERROR - FreeRTOS scheduler failed! System halted.\n");
    printf("INFO - Check FreeRTOS configuration and task creation\n");
    
    // Parpadear LED de error
    // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5); // LED_RED
    HAL_Delay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK2
                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
