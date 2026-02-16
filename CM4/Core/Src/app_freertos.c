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

#include "app_lorawan.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "rtos_printf.h"
#include "EmbeddedMessage.h"

// Forward declarations para threads del sistema FreeRTOS
extern void dispatcherTask(void *argument);
extern void fsmTask(void *argument);
extern void sensorAcqTask(void *argument);
extern void imuTask(void *argument);
extern void gpsTask(void *argument);
extern void inaTask(void *argument);
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

// Cola para adquisición de sensores (LEGACY - usar colas individuales)
osMessageQueueId_t sensorAcqQueueHandle;
const osMessageQueueAttr_t sensorAcqQueue_attributes = {
  .name = "sensorAcqQueue"
};

// Colas para sensores individuales
osMessageQueueId_t imuQueueHandle;
const osMessageQueueAttr_t imuQueue_attributes = {
  .name = "imuQueue"
};

osMessageQueueId_t gpsQueueHandle;
const osMessageQueueAttr_t gpsQueue_attributes = {
  .name = "gpsQueue"
};

osMessageQueueId_t inaQueueHandle;
const osMessageQueueAttr_t inaQueue_attributes = {
  .name = "inaQueue"
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

osMessageQueueId_t loraRxQueueHandle;
const osMessageQueueAttr_t loraRxQueue_attributes = {
  .name = "loraRxQueue"
};

osMessageQueueId_t distanceToLimitQueueHandle;
const osMessageQueueAttr_t distanceToLimitQueue_attributes = {
  .name = "distanceToLimitQueue"
};

osMessageQueueId_t fenceUpdateQueueHandle;
const osMessageQueueAttr_t fenceUpdateQueue_attributes = {
  .name = "fenceUpdateQueue"
};

// Thread dispatcher - rutea mensajes entre módulos
osThreadId_t dispatcher_TaskHandle;

// Buffers estáticos en RAM1 para dispatcher (COMENTADO - ahora usa heap dinámico)
// __attribute__((section(".RAM1_region"))) static StaticTask_t dispatcher_TaskBuffer;
// __attribute__((section(".RAM1_region"))) static StackType_t dispatcher_TaskStack[192];  // 768 bytes / 4 bytes per word

const osThreadAttr_t dispatcher_Task_attributes = {
  .name = "dispatcher_Task",
  .stack_size = 128 * 5,  // 512 bytes - ruteo simple
  .priority = (osPriority_t) osPriorityNormal,
  // .cb_mem = &dispatcher_TaskBuffer,
  // .cb_size = sizeof(dispatcher_TaskBuffer),
  // .stack_mem = dispatcher_TaskStack,
};

// Thread FSM - máquina de estados principal
osThreadId_t fsm_TaskHandle;

// Buffers estáticos en RAM1 para FSM (COMENTADO - ahora usa heap dinámico)
// __attribute__((section(".RAM1_region"))) static StaticTask_t fsm_TaskBuffer;
// __attribute__((section(".RAM1_region"))) static StackType_t fsm_TaskStack[256];  // 1024 bytes / 4 bytes per word

const osThreadAttr_t fsm_Task_attributes = {
  .name = "fsm_Task",
  .stack_size = 312 * 4,  // 2048 bytes - FSMs complejas anidadas
  .priority = (osPriority_t) osPriorityNormal,
  // .cb_mem = &fsm_TaskBuffer,
  // .cb_size = sizeof(fsm_TaskBuffer),
  // .stack_mem = fsm_TaskStack,
};

osThreadId_t stimulus_TaskHandle;

// Buffers estáticos en RAM1 para stimulus (COMENTADO - ahora usa heap dinámico)
// __attribute__((section(".RAM1_region"))) static StaticTask_t stimulus_TaskBuffer;
// __attribute__((section(".RAM1_region"))) static StackType_t stimulus_TaskStack[128];  // 512 bytes / 4 bytes per word

const osThreadAttr_t stimulus_Task_attributes = {
  .name = "stimulus_Task",
  .stack_size = 256 * 4,  // 1024 bytes - buzzer + vibration motors + alarms
  .priority = (osPriority_t) osPriorityNormal,
  // .cb_mem = &stimulus_TaskBuffer,
  // .cb_size = sizeof(stimulus_TaskBuffer),
  // .stack_mem = stimulus_TaskStack,
};

// Thread sensor acquisition - adquisición de datos de sensores (LEGACY)
osThreadId_t sensorAcq_TaskHandle;

// Buffers estáticos en RAM1 para sensorAcq (COMENTADO - ahora usa heap dinámico)
// __attribute__((section(".RAM1_region"))) static StaticTask_t sensorAcq_TaskBuffer;
// __attribute__((section(".RAM1_region"))) static StackType_t sensorAcq_TaskStack[256];  // 1024 bytes / 4 bytes per word

const osThreadAttr_t sensorAcq_Task_attributes = {
  .name = "sensorAcq_Task",
  .stack_size = 512 * 4,  // 2048 bytes - objetos C++ grandes (GPS, IMU, INA)
  .priority = (osPriority_t) osPriorityNormal,
  // .cb_mem = &sensorAcq_TaskBuffer,
  // .cb_size = sizeof(sensorAcq_TaskBuffer),
  // .stack_mem = sensorAcq_TaskStack,
};

// Thread IMU - adquisición de acelerómetro
osThreadId_t imu_TaskHandle;
const osThreadAttr_t imu_Task_attributes = {
  .name = "imu_Task",
  .stack_size = 192 * 4,  // 2048 bytes - burst buffers + objetos C++ LSM6DSO
  .priority = (osPriority_t) osPriorityNormal,
};

// Thread GPS - adquisición de posición
osThreadId_t gps_TaskHandle;
const osThreadAttr_t gps_Task_attributes = {
  .name = "gps_Task",
  .stack_size = 292 * 4,  // 1536 bytes - objeto C++ SAM-M10Q
  .priority = (osPriority_t) osPriorityNormal,
};

// Thread INA - adquisición de corrientes
osThreadId_t ina_TaskHandle;
const osThreadAttr_t ina_Task_attributes = {
  .name = "ina_Task",
  .stack_size = 228 * 4,  // 1536 bytes - 3 objetos C++ INA226
  .priority = (osPriority_t) osPriorityNormal,
};

// Thread LoRa TX - transmisión LoRa
osThreadId_t lora_TaskHandle;

// Buffers estáticos en RAM1 para lora (COMENTADO - ahora usa heap dinámico)
// __attribute__((section(".RAM1_region"))) static StaticTask_t lora_TaskBuffer;
// __attribute__((section(".RAM1_region"))) static StackType_t lora_TaskStack[192];  // 768 bytes / 4 bytes per word

const osThreadAttr_t lora_Task_attributes = {
  .name = "lora_Task",
  .stack_size = 128 * 4,  // 512 bytes - Tx simple
  .priority = (osPriority_t) osPriorityNormal,
  // .cb_mem = &lora_TaskBuffer,
  // .cb_size = sizeof(lora_TaskBuffer),
  // .stack_mem = lora_TaskStack,
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityLow,  // Baja prioridad - solo monitoring
  .stack_size = 128 * 4  // 512 bytes - suficiente para monitoring simple
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/**
 * @brief Inicializar todas las colas FreeRTOS del sistema
 * @details Crea las colas para comunicación entre threads usando EmbeddedMessage_t*
 */
void initialize_message_queues(void) {
    printf("[QUEUES] Inicializando colas de mensajes FreeRTOS...\r\n");
    
    // Cola principal del dispatcher (más grande, recibe todos los mensajes)
    dispatcherQueueHandle = osMessageQueueNew(32, sizeof(void*), &dispatcherQueue_attributes);
    if (dispatcherQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación dispatcherQueue\r\n");
        Error_Handler();
    }

    // Cola principal del dispatcher (más grande, recibe todos los mensajes)
    fsmQueueHandle = osMessageQueueNew(16, sizeof(void*), &fsmQueue_attributes);
    if (fsmQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación fsmQueue\r\n");
        Error_Handler();
    }
    
    //Cola para adquisición de sensores (LEGACY - no se usa más)
    sensorAcqQueueHandle = osMessageQueueNew(16, sizeof(void*), &sensorAcqQueue_attributes);
    if (sensorAcqQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación sensorAcqQueue\r\n");
        Error_Handler();
    }
    
    // Colas para sensores individuales
    imuQueueHandle = osMessageQueueNew(12, sizeof(void*), &imuQueue_attributes);
    if (imuQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación imuQueue\r\n");
        Error_Handler();
    }
    
    gpsQueueHandle = osMessageQueueNew(12, sizeof(void*), &gpsQueue_attributes);
    if (gpsQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación gpsQueue\r\n");
        Error_Handler();
    }
    
    inaQueueHandle = osMessageQueueNew(12, sizeof(void*), &inaQueue_attributes);
    if (inaQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación inaQueue\r\n");
        Error_Handler();
    }

    //Cola para estímulos
    stimulusQueueHandle = osMessageQueueNew(16, sizeof(void*), &stimulusQueue_attributes);
    if (stimulusQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación stimulusQueue\r\n");
        Error_Handler();
    }
    
    // Colas adicionales (tamaños más pequeños para funciones futuras)
    //gpsQueueHandle = osMessageQueueNew(8, sizeof(void*), &gpsQueue_attributes);
    loraTxQueueHandle = osMessageQueueNew(16, sizeof(void*), &loraTxQueue_attributes);
    if (loraTxQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación loraTxQueue\r\n");
        Error_Handler();
    }
    
    loraRxQueueHandle = osMessageQueueNew(12, sizeof(void*), &loraRxQueue_attributes);
    if (loraRxQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación loraRxQueue\r\n");
        Error_Handler();
    }
    
    distanceToLimitQueueHandle = osMessageQueueNew(8, sizeof(void*), &distanceToLimitQueue_attributes);
    if (distanceToLimitQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación distanceToLimitQueue\r\n");
        Error_Handler();
    }
    
    fenceUpdateQueueHandle = osMessageQueueNew(4, sizeof(void*), &fenceUpdateQueue_attributes);
    if (fenceUpdateQueueHandle == NULL) {
        printf("[QUEUES] ERROR - Fallo creación fenceUpdateQueue\r\n");
        Error_Handler();
    }
    
    printf("[QUEUES] OK - Todas las colas creadas exitosamente\r\n");
    printf("[QUEUES] - DispatcherQueue: 32 slots\r\n");
    printf("[QUEUES] - FSMQueue: 16 slots\r\n");
    printf("[QUEUES] - IMUQueue: 12 slots\r\n");
    printf("[QUEUES] - GPSQueue: 12 slots\r\n");
    printf("[QUEUES] - INAQueue: 12 slots\r\n");
    printf("[QUEUES] - SensorAcqQueue (legacy): 16 slots\r\n");
    printf("[QUEUES] - Colas adicionales: 4-12 slots c/u\r\n");
    printf("[QUEUES] - Tamaño por slot: %u bytes (puntero EmbeddedMessage_t*)\r\n", 
           (unsigned int)sizeof(void*));
}

/**
 * @brief Inicializar todos los threads FreeRTOS del sistema
 * @details Crea los threads principales: dispatcher, FSM, sensor acquisition
 */
void initialize_system_threads(void) {
    printf("[THREADS] Inicializando threads del sistema FreeRTOS...\r\n");
    
    //Thread dispatcher - alta prioridad (ruteo de mensajes crítico)
    dispatcher_TaskHandle = osThreadNew(dispatcherTask, NULL, &dispatcher_Task_attributes);
    if (dispatcher_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación dispatcher_Task\r\n");
        Error_Handler();
    }
    
    //Thread FSM - prioridad normal (lógica de aplicación)
    fsm_TaskHandle = osThreadNew(fsmTask, NULL, &fsm_Task_attributes);
    if (fsm_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación fsm_Task\r\n");
        Error_Handler();
    }

    //Thread stimulus - prioridad normal (lógica de aplicación)
    stimulus_TaskHandle = osThreadNew(stimulusTask, NULL, &stimulus_Task_attributes);
    if (stimulus_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación stimulus_Task\r\n");
        Error_Handler();
    }

    //Thread sensor acquisition - prioridad normal (LEGACY - comentado)
// #ifdef ENABLE_TEST_MODE
//     // TEST MODE: Usar tarea mock con datos predefinidos
//     sensorAcq_TaskHandle = osThreadNew(sensorAcqTask_Test, NULL, &sensorAcq_Task_attributes);
//     printf("[THREADS] ** TEST MODE ** - Using mock sensor task\r\n");
// #else
//     // PRODUCTION MODE: Usar tarea real con sensores de hardware
//     sensorAcq_TaskHandle = osThreadNew(sensorAcqTask, NULL, &sensorAcq_Task_attributes);
// #endif
//     if (sensorAcq_TaskHandle == NULL) {
//         printf("[THREADS] ERROR - Fallo creación sensorAcq_Task\r\n");
//         Error_Handler();
//     }
    
    // Thread IMU - acelerómetro LSM6DSO
    imu_TaskHandle = osThreadNew(imuTask, NULL, &imu_Task_attributes);
    if (imu_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación imu_Task\r\n");
        Error_Handler();
    }
    
    // Thread GPS - SAM-M10Q
    gps_TaskHandle = osThreadNew(gpsTask, NULL, &gps_Task_attributes);
    if (gps_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación gps_Task\r\n");
        Error_Handler();
    }
    
    // Thread INA - INA226 (GPS, IMU, MCU)
    ina_TaskHandle = osThreadNew(inaTask, NULL, &ina_Task_attributes);
    if (ina_TaskHandle == NULL) {
        printf("[THREADS] ERROR - Fallo creación ina_Task\r\n");
        Error_Handler();
    }


    printf("[THREADS] OK - Todos los threads creados exitosamente\r\n");
    printf("[THREADS] - dispatcher_Task: Stack 512B\r\n");
    printf("[THREADS] - fsm_Task: Stack 2KB (FSMs complejas)\r\n");
    printf("[THREADS] - stimulus_Task: Stack 1KB (buzzer + motors + alarms)\r\n");
    printf("[THREADS] - imu_Task: Stack 2KB (burst buffers + LSM6DSO)\r\n");
    printf("[THREADS] - gps_Task: Stack 1.5KB (SAM-M10Q)\r\n");
    printf("[THREADS] - ina_Task: Stack 1.5KB (3x INA226)\r\n");
    printf("[THREADS] - lora_Task: Stack 512B\r\n");
    printf("[THREADS] Total stack allocated: ~8.5KB\r\n");
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
  printf("\r\n");
  printf("========================================\r\n");
  printf("     TEST MODE ENABLED\r\n");
  printf("========================================\r\n");
  printf("Using mock sensor data for FSM testing\r\n");
  printf("GPS samples: %d | IMU samples: %d\r\n", TEST_GPS_DATA_COUNT, TEST_IMU_DATA_COUNT);
  printf("========================================\r\n");
  printf("\r\n");
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
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  //initialize_system_threads();
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
  /* init code for LoRaWAN */
  MX_LoRaWAN_Init();
  /* USER CODE BEGIN StartDefaultTask */
  initialize_system_threads();
  
  // NOTA: NO terminar esta tarea - causa problemas de double-free en heap
  osThreadTerminate (defaultTaskHandle);
  
  /* Infinite loop */
  // static uint32_t stackMonitorCounter = 0;
  // for(;;)
  // {
  //   // Monitorear stack cada 10 segundos
  //   if (++stackMonitorCounter >= 10) {
  //     UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
  //     rtos_printf("[DEFAULT] Stack libre: %u words (%u bytes)\r\n", 
  //                stackLeft, stackLeft * 4);
  //     stackMonitorCounter = 0;
  //   }
  //   osDelay(1000);
  // }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief  Configure the DWT cycle counter for runtime stats
 * @note   Called automatically by FreeRTOS during initialization
 */
void vConfigureTimerForRunTimeStats(void)
{
    /* Enable TRC (Trace) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    /* Reset the cycle counter */
    DWT->CYCCNT = 0;
    
    /* Enable the cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Get current value of the runtime counter
 * @retval Current cycle count value
 * @note   Called by FreeRTOS to measure task execution time
 */
uint32_t vGetRunTimeCounterValue(void)
{
    /* Return current cycle count */
    return DWT->CYCCNT;
}

/* USER CODE END Application */
