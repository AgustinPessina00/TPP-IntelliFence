/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rtos_printf.h"
#include "EmbeddedMessage.h"

// Forward declarations para threads del sistema FreeRTOS
extern void dispatcherTask(void *argument);
extern void fsmTask(void *argument);
extern void sensorAcqTask(void *argument);
extern void stimulusTask(void *argument);
extern void loraTask(void *argument);

// Test mode support
#ifdef ENABLE_TEST_MODE
#include "Test/test_data_c_wrapper.h"
extern void sensorAcqTask_Test(void *argument);
#endif

//extern void MessagePool_Init(void);
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
/* USER CODE BEGIN Variables */

osMessageQueueId_t dispatcherQueueHandle;
const osMessageQueueAttr_t dispatcherQueue_attributes = {
  .name = "dispatcherQueue"
};

// Cola para adquisición de sensores
osMessageQueueId_t sensorAcqQueueHandle;
const osMessageQueueAttr_t sensorAcqQueue_attributes = {
  .name = "sensorAcqQueue"
};

// Cola para la máquina de estados finite state machine
osMessageQueueId_t fsmQueueHandle;
const osMessageQueueAttr_t fsmQueue_attributes = {
  .name = "fsmQueue"
};

// Colas adicionales (para futuras implementaciones)
osMessageQueueId_t stimulusQueueHandle;
const osMessageQueueAttr_t stimulusQueue_attributes = {
  .name = "stimulusQueue"
};

// osMessageQueueId_t gpsQueueHandle;
// const osMessageQueueAttr_t gpsQueue_attributes = {
//   .name = "gpsQueue"
// };

osMessageQueueId_t loraTxQueueHandle;
const osMessageQueueAttr_t loraTxQueue_attributes = {
  .name = "loraTxQueue"
};

// osMessageQueueId_t loraRxQueueHandle;
// const osMessageQueueAttr_t loraRxQueue_attributes = {
//   .name = "loraRxQueue"
// };

// osMessageQueueId_t distanceToLimitQueueHandle;
// const osMessageQueueAttr_t distanceToLimitQueue_attributes = {
//   .name = "distanceToLimitQueue"
// };

// osMessageQueueId_t fenceUpdateQueueHandle;
// const osMessageQueueAttr_t fenceUpdateQueue_attributes = {
//   .name = "fenceUpdateQueue"
// };

// Thread dispatcher - rutea mensajes entre módulos
osThreadId_t dispatcher_TaskHandle;
const osThreadAttr_t dispatcher_Task_attributes = {
  .name = "dispatcher_Task",
  .stack_size = 256 * 3,  // 768 bytes dispatcher
  .priority = (osPriority_t) osPriorityNormal,
};

// Thread FSM - máquina de estados principal
osThreadId_t fsm_TaskHandle;
const osThreadAttr_t fsm_Task_attributes = {
  .name = "fsm_Task",
  .stack_size = 256 * 4,  // 1024 bytes FSM
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t stimulus_TaskHandle;
const osThreadAttr_t stimulus_Task_attributes = {
  .name = "stimulus_Task",
  .stack_size = 128 * 4,  // 512 bytes stimulus
  .priority = (osPriority_t) osPriorityNormal,
};

// Thread sensor acquisition - adquisición de datos de sensores
osThreadId_t sensorAcq_TaskHandle;
const osThreadAttr_t sensorAcq_Task_attributes = {
  .name = "sensorAcq_Task",
  .stack_size = 256 * 4,  // 1024 bytes sensorAcq
  .priority = (osPriority_t) osPriorityNormal,
};

// Thread LoRa TX - transmisión LoRa
osThreadId_t lora_TaskHandle;
const osThreadAttr_t lora_Task_attributes = {
  .name = "lora_Task",
  .stack_size = 256 * 3,  // 768 bytes LoRa
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 2  // 256 bytes suficiente
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/**
 * @brief Inicializar todas las colas FreeRTOS del sistema
 * @details Crea las colas para comunicación entre threads usando EmbeddedMessage_t*
 */
void initialize_message_queues(void) {
    printf("[QUEUES] Inicializando colas de mensajes FreeRTOS...\n");
    
    // Cola principal del dispatcher (más grande, recibe todos los mensajes)
    dispatcherQueueHandle = osMessageQueueNew(32, sizeof(void*), &dispatcherQueue_attributes);
    if (dispatcherQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación dispatcherQueue\n");
        Error_Handler();
    }

    // Cola principal del dispatcher (más grande, recibe todos los mensajes)
    fsmQueueHandle = osMessageQueueNew(16, sizeof(void*), &fsmQueue_attributes);
    if (fsmQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación fsmQueue\n");
        Error_Handler();
    }
    
    // Cola para adquisición de sensores
    sensorAcqQueueHandle = osMessageQueueNew(16, sizeof(void*), &sensorAcqQueue_attributes);
    if (sensorAcqQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación sensorAcqQueue\n");
        Error_Handler();
    }

    // Cola para estímulos
    stimulusQueueHandle = osMessageQueueNew(16, sizeof(void*), &stimulusQueue_attributes);
    if (stimulusQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación stimulusQueue\n");
        Error_Handler();
    }
    
    // Colas adicionales (tamaños más pequeños para funciones futuras)
    //gpsQueueHandle = osMessageQueueNew(8, sizeof(void*), &gpsQueue_attributes);
    loraTxQueueHandle = osMessageQueueNew(12, sizeof(void*), &loraTxQueue_attributes);
    //loraRxQueueHandle = osMessageQueueNew(12, sizeof(void*), &loraRxQueue_attributes);
    //distanceToLimitQueueHandle = osMessageQueueNew(8, sizeof(void*), &distanceToLimitQueue_attributes);
    //fenceUpdateQueueHandle = osMessageQueueNew(4, sizeof(void*), &fenceUpdateQueue_attributes);
    
    printf("[QUEUES] OK - Todas las colas creadas exitosamente\n");
    printf("[QUEUES] - DispatcherQueue: 32 slots\n");
    printf("[QUEUES] - SensorAcqQueue: 16 slots\n");
    printf("[QUEUES] - FSMQueue: 16 slots\n");
    printf("[QUEUES] - Colas adicionales: 4-12 slots c/u\n");
    printf("[QUEUES] - Tamaño por slot: %u bytes (puntero EmbeddedMessage_t*)\n", 
           (unsigned int)sizeof(void*));
}

/**
 * @brief Inicializar todos los threads FreeRTOS del sistema
 * @details Crea los threads principales: dispatcher, FSM, sensor acquisition
 */
void initialize_system_threads(void) {
    printf("[THREADS] Inicializando threads del sistema FreeRTOS...\n");
    
    // Thread dispatcher - alta prioridad (ruteo de mensajes crítico)
    dispatcher_TaskHandle = osThreadNew(dispatcherTask, NULL, &dispatcher_Task_attributes);
    if (dispatcher_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación dispatcher_Task\n");
        Error_Handler();
    }
    
    //Thread FSM - prioridad normal (lógica de aplicación)
    fsm_TaskHandle = osThreadNew(fsmTask, NULL, &fsm_Task_attributes);
    if (fsm_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación fsm_Task\n");
        Error_Handler();
    }

    //Thread FSM - prioridad normal (lógica de aplicación)
    stimulus_TaskHandle = osThreadNew(stimulusTask, NULL, &stimulus_Task_attributes);
    if (stimulus_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación stimulus_Task\n");
        Error_Handler();
    }

    

    //Thread sensor acquisition - prioridad normal (adquisición periódica)
#ifdef ENABLE_TEST_MODE
    // TEST MODE: Usar tarea mock con datos predefinidos
    sensorAcq_TaskHandle = osThreadNew(sensorAcqTask_Test, NULL, &sensorAcq_Task_attributes);
    printf("[THREADS] ** TEST MODE ** - Using mock sensor task\n");
#else
    // PRODUCTION MODE: Usar tarea real con sensores de hardware
    sensorAcq_TaskHandle = osThreadNew(sensorAcqTask, NULL, &sensorAcq_Task_attributes);
#endif
    if (sensorAcq_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación sensorAcq_Task\n");
        Error_Handler();
    }

    //Thread LoRa TX - prioridad normal (transmisión LoRa)
    lora_TaskHandle = osThreadNew(loraTask, NULL, &lora_Task_attributes);
    if (lora_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación lora_Task\n");
        Error_Handler();
    }

    printf("[THREADS] OK - Todos los threads creados exitosamente\n");
    printf("[THREADS] - dispatcher_Task: Prioridad ALTA, Stack 768B\n");
    printf("[THREADS] - fsm_Task: Prioridad NORMAL, Stack 1KB\n");
    printf("[THREADS] - stimulus_Task: Prioridad NORMAL, Stack 512B\n");
    printf("[THREADS] - sensorAcq_Task: Prioridad NORMAL, Stack 1KB\n");
    printf("[THREADS] - lora_Task: Prioridad NORMAL, Stack 768B\n");
    printf("[THREADS] Total stack allocated: ~4KB\n");
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  MessagePool_Init();  // Inicializar el pool de mensajes antes de cualquier uso
  
  // Inicializar sistema de printf thread-safe
  if (rtos_printf_init() != 0) {
    Error_Handler();  // Fallo crítico en inicialización de printf
  }
  
#ifdef ENABLE_TEST_MODE
  // Inicializar datos de prueba para sensores mock
  TestData_Init();
  TestMode_Enable();
  printf("\n");
  printf("========================================\n");
  printf("     TEST MODE ENABLED\n");
  printf("========================================\n");
  printf("Using mock sensor data for FSM testing\n");
  printf("GPS samples: %d | IMU samples: %d\n", TEST_GPS_DATA_COUNT, TEST_IMU_DATA_COUNT);
  printf("========================================\n");
  printf("\n");
#endif
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  initialize_message_queues();
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
 //defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  initialize_system_threads();
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
//   /* Infinite loop */
//   for(;;)
//   {
//     osDelay(1);
//   }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
