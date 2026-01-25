/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "platform.h"
#include "sys_app.h"
#include "lora_app.h"
#include "cmsis_os.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "LmHandler.h"
#include "adc_if.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"
#include "flash_if.h"
#include "mbmuxif_sys.h"
#define RTOS_PRINTF_AUTO
/* USER CODE BEGIN Includes */
#include "rtos_printf.h"

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief LoRa State Machine states
  */
typedef enum TxEventType_e
{
  /**
    * @brief Appdata Transmission issue based on timer every TxDutyCycleTime
    */
  TX_ON_TIMER,
  /**
    * @brief Appdata Transmission external event plugged on OnSendEvent( )
    */
  TX_ON_EVENT
  /* USER CODE BEGIN TxEventType_t */

  /* USER CODE END TxEventType_t */
} TxEventType_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/**
  * LEDs period value of the timer in ms
  */
#define LED_PERIOD_TIME 500

/**
  * Join switch period value of the timer in ms
  */
#define JOIN_TIME 2000

/*---------------------------------------------------------------------------*/
/*                             LoRaWAN NVM configuration                     */
/*---------------------------------------------------------------------------*/
/**
  * @brief LoRaWAN NVM Flash address
  * @note last 2 sector of a 128kBytes device
  */
#define LORAWAN_NVM_BASE_ADDRESS                    ((void *)0x0801F000UL)

/* USER CODE BEGIN PD */
static const char *slotStrings[] = { "1", "2", "C", "C_MC", "P", "P_MC" };
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  LoRa End Node send request
  */
static void SendTxData(void);

/**
  * @brief  TX timer callback function
  * @param  context ptr of timer context
  */
static void OnTxTimerEvent(void *context);

/**
  * @brief  join event callback function
  * @param  joinParams status of join
  */
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);

/**
  * @brief callback when LoRaWAN application has sent a frame
  * @brief  tx event callback function
  * @param  params status of last Tx
  */
static void OnTxData(LmHandlerTxParams_t *params);

/**
  * @brief callback when LoRaWAN application has received a frame
  * @param appData data received in the last Rx
  * @param params status of last Rx
  */
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

/**
  * @brief callback when LoRaWAN Beacon status is updated
  * @param params status of Last Beacon
  */
static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params);

/**
  * @brief callback when system time has been updated
  */
static void OnSysTimeUpdate(void);

/**
  * @brief callback when LoRaWAN application Class is changed
  * @param deviceClass new class
  */
static void OnClassChange(DeviceClass_t deviceClass);

/**
  * @brief  LoRa store context in Non Volatile Memory
  */
static void StoreContext(void);

/**
  * @brief  stop current LoRa execution to switch into non default Activation mode
  */
static void StopJoin(void);

/**
  * @brief  Join switch timer callback function
  * @param  context ptr of Join switch context
  */
static void OnStopJoinTimerEvent(void *context);

/**
  * @brief  Notifies the upper layer that the NVM context has changed
  * @param  state Indicates if we are storing (true) or restoring (false) the NVM context
  */
static void OnNvmDataChange(LmHandlerNvmContextStates_t state);

/**
  * @brief  Store the NVM Data context to the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were stored
  */
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * @brief  Restore the NVM Data context from the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were restored
  */
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * Will be called each time a Radio IRQ is handled by the MAC layer
  *
  * \remark OnMacProcessNotify not needed on dual Core as OnMacProcessNotify is processed on M0+
  */
static void OnMacProcessNotify(void);

/**
  * @brief Change the periodicity of the uplink frames
  * @param periodicity uplink frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnTxPeriodicityChanged(uint32_t periodicity);

/**
  * @brief Change the confirmation control of the uplink frames
  * @param isTxConfirmed Indicates if the uplink requires an acknowledgement
  * @note Compliance test protocol callbacks
  */
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);

/**
  * @brief Change the periodicity of the ping slot frames
  * @param pingSlotPeriodicity ping slot frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);

/**
  * @brief Will be called to reset the system
  * @note Compliance test protocol callbacks
  */
static void OnSystemReset(void);

/* USER CODE BEGIN PFP */

/**
  * @brief  LED Tx timer callback function
  * @param  context ptr of LED context
  */
static void OnTxTimerLedEvent(void *context);

/**
  * @brief  LED Rx timer callback function
  * @param  context ptr of LED context
  */
static void OnRxTimerLedEvent(void *context);

/**
  * @brief  LED Join timer callback function
  * @param  context ptr of LED context
  */
static void OnJoinTimerLedEvent(void *context);

/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief LoRaWAN default activation type
  */
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

/**
  * @brief LoRaWAN force rejoin even if the NVM context is restored
  */
static bool ForceRejoin = LORAWAN_FORCE_REJOIN_AT_BOOT;

/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =              GetBatteryLevel,
  .GetTemperature =               GetTemperatureLevel,
  .OnRestoreContextRequest =      OnRestoreContextRequest,
  .OnStoreContextRequest =        OnStoreContextRequest,
  .OnMacProcess =                 OnMacProcessNotify,
  .OnNvmDataChange =              OnNvmDataChange,
  .OnJoinRequest =                OnJoinRequest,
  .OnTxData =                     OnTxData,
  .OnRxData =                     OnRxData,
  .OnBeaconStatusChange =         OnBeaconStatusChange,
  .OnSysTimeUpdate =              OnSysTimeUpdate,
  .OnClassChange =                OnClassChange,
  .OnTxPeriodicityChanged =       OnTxPeriodicityChanged,
  .OnTxFrameCtrlChanged =         OnTxFrameCtrlChanged,
  .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
  .OnSystemReset =                OnSystemReset,
};

/**
  * @brief LoRaWAN handler parameters
  */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .IsTxConfirmed =            LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .TxPower =                  LORAWAN_DEFAULT_TX_POWER,
  .PingSlotPeriodicity =      LORAWAN_DEFAULT_PING_SLOT_PERIODICITY,
  .RxBCTimeout =              LORAWAN_DEFAULT_CLASS_B_C_RESP_TIMEOUT
};

/**
  * @brief Type of Event to generate application Tx
  */
static TxEventType_t EventType = TX_ON_TIMER;

/**
  * @brief Timer to handle the application Tx
  */
static UTIL_TIMER_Object_t TxTimer;

/**
  * @brief Tx Timer period
  */
static UTIL_TIMER_Time_t TxPeriodicity = APP_TX_DUTYCYCLE;

/**
  * @brief Join Timer period
  */
static UTIL_TIMER_Object_t StopJoinTimer;

osThreadId_t Thd_LoraSendProcessId;

const osThreadAttr_t Thd_LoraSendProcess_attr =
{
  .name = CFG_APP_LORA_PROCESS_NAME,
  .attr_bits = CFG_APP_LORA_PROCESS_ATTR_BITS,
  .cb_mem = CFG_APP_LORA_PROCESS_CB_MEM,
  .cb_size = CFG_APP_LORA_PROCESS_CB_SIZE,
  .stack_mem = CFG_APP_LORA_PROCESS_STACK_MEM,
  .priority = CFG_APP_LORA_PROCESS_PRIORITY,
  .stack_size = CFG_APP_LORA_PROCESS_STACK_SIZE
};
static void Thd_LoraSendProcess(void *argument);

osThreadId_t Thd_LoraStoreContextId;

const osThreadAttr_t Thd_LoraStoreContext_attr =
{
  .name = CFG_APP_LORA_STORE_CONTEXT_NAME,
  .attr_bits = CFG_APP_LORA_STORE_CONTEXT_ATTR_BITS,
  .cb_mem = CFG_APP_LORA_STORE_CONTEXT_CB_MEM,
  .cb_size = CFG_APP_LORA_STORE_CONTEXT_CB_SIZE,
  .stack_mem = CFG_APP_LORA_STORE_CONTEXT_STACK_MEM,
  .priority = CFG_APP_LORA_STORE_CONTEXT_PRIORITY,
  .stack_size = CFG_APP_LORA_STORE_CONTEXT_STACK_SIZE
};
static void Thd_LoraStoreContext(void *argument);

osThreadId_t Thd_LoraStopJoinId;

const osThreadAttr_t Thd_LoraStopJoin_attr =
{
  .name = CFG_APP_LORA_STOP_JOIN_NAME,
  .attr_bits = CFG_APP_LORA_STOP_JOIN_ATTR_BITS,
  .cb_mem = CFG_APP_LORA_STOP_JOIN_CB_MEM,
  .cb_size = CFG_APP_LORA_STOP_JOIN_CB_SIZE,
  .stack_mem = CFG_APP_LORA_STOP_JOIN_STACK_MEM,
  .priority = CFG_APP_LORA_STOP_JOIN_PRIORITY,
  .stack_size = CFG_APP_LORA_STOP_JOIN_STACK_SIZE
};
static void Thd_LoraStopJoin(void *argument);

/* USER CODE BEGIN PV */
/**
  * @brief User application buffer
  */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/**
  * @brief User application data structure
  */
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/**
  * @brief Timer to handle the application Tx Led to toggle
  */
static UTIL_TIMER_Object_t TxLedTimer;

/**
  * @brief Timer to handle the application Rx Led to toggle
  */
static UTIL_TIMER_Object_t RxLedTimer;

/**
  * @brief Timer to handle the application Join Led to toggle
  */
static UTIL_TIMER_Object_t JoinLedTimer;

/**
  * Temp buffer to store a FLASH page in RAM when partial replacement is needed
  */
static uint8_t FLASH_RAM_buffer[FLASH_IF_BUFFER_SIZE];

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_LV */

  /* USER CODE END LoRaWAN_Init_LV */

  /* USER CODE BEGIN LoRaWAN_Init_1 */
  /* USER CODE BEGIN LoRaWAN_Init_LV */
  FEAT_INFO_Param_t *p_cm0plus_specific_features_info;
  uint32_t feature_version = 0UL;
  /* USER CODE END LoRaWAN_Init_LV */

  /* USER CODE BEGIN LoRaWAN_Init_1 */

  /* Get CM4 LoRaWAN APP version*/
  rtos_printf("M4_APP_VERSION:      V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get CM0 LoRaWAN APP version*/
  p_cm0plus_specific_features_info = MBMUXIF_SystemGetFeatCapabInfoPtr(FEAT_INFO_SYSTEM_ID);
  feature_version = p_cm0plus_specific_features_info->Feat_Info_Feature_Version;
  rtos_printf("M0PLUS_APP_VERSION:  V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get MW LoRaWAN info */
  p_cm0plus_specific_features_info = MBMUXIF_SystemGetFeatCapabInfoPtr(FEAT_INFO_LORAWAN_ID);
  feature_version = p_cm0plus_specific_features_info->Feat_Info_Feature_Version;
  rtos_printf("MW_LORAWAN_VERSION:  V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get MW SubGhz_Phy info */
  p_cm0plus_specific_features_info = MBMUXIF_SystemGetFeatCapabInfoPtr(FEAT_INFO_RADIO_ID);
  feature_version = p_cm0plus_specific_features_info->Feat_Info_Feature_Version;
  rtos_printf("MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get LoRaWAN Link Layer info */
  LmHandlerGetVersion(LORAMAC_HANDLER_L2_VERSION, &feature_version);
  rtos_printf("L2_SPEC_VERSION:     V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get LoRaWAN Regional Parameters info */
  LmHandlerGetVersion(LORAMAC_HANDLER_REGION_VERSION, &feature_version);
  rtos_printf("RP_SPEC_VERSION:     V%X-%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8),
          (uint8_t)(feature_version));

  /* USER CODE END LoRaWAN_Init_1 */

  UTIL_TIMER_Create(&StopJoinTimer, JOIN_TIME, UTIL_TIMER_ONESHOT, OnStopJoinTimerEvent, NULL);

  Thd_LoraSendProcessId = osThreadNew(Thd_LoraSendProcess, NULL, &Thd_LoraSendProcess_attr);
  if (Thd_LoraSendProcessId == NULL)
  {
    Error_Handler();
  }
  Thd_LoraStoreContextId = osThreadNew(Thd_LoraStoreContext, NULL, &Thd_LoraStoreContext_attr);
  if (Thd_LoraStoreContextId == NULL)
  {
    Error_Handler();
  }
  Thd_LoraStopJoinId = osThreadNew(Thd_LoraStopJoin, NULL, &Thd_LoraStopJoin_attr);
  if (Thd_LoraStopJoinId == NULL)
  {
    Error_Handler();
  }

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks, APP_VERSION);

  LmHandlerConfigure(&LmHandlerParams);

  /* USER CODE BEGIN LoRaWAN_Init_2 */
  UTIL_TIMER_Create(&TxLedTimer, LED_PERIOD_TIME, UTIL_TIMER_ONESHOT, OnTxTimerLedEvent, NULL);
  UTIL_TIMER_Create(&RxLedTimer, LED_PERIOD_TIME, UTIL_TIMER_ONESHOT, OnRxTimerLedEvent, NULL);
  UTIL_TIMER_Create(&JoinLedTimer, LED_PERIOD_TIME, UTIL_TIMER_PERIODIC, OnJoinTimerLedEvent, NULL);

  if (FLASH_IF_Init(FLASH_RAM_buffer) != FLASH_IF_OK)
  {
    Error_Handler();
  }
  /* USER CODE END LoRaWAN_Init_2 */

  LmHandlerJoin(ActivationType, ForceRejoin);

  if (EventType == TX_ON_TIMER)
  {
    /* send every time timer elapses */
    UTIL_TIMER_Create(&TxTimer, TxPeriodicity, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
    UTIL_TIMER_Start(&TxTimer);
  }
  else
  {
    /* USER CODE BEGIN LoRaWAN_Init_3 */

    /* USER CODE END LoRaWAN_Init_3 */
  }

  /* USER CODE BEGIN LoRaWAN_Init_Last */

  /* USER CODE END LoRaWAN_Init_Last */
}

/* USER CODE BEGIN PB_Callbacks */

#if 0 /* User should remove the #if 0 statement and adapt the below code according with his needs*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case  BUT1_Pin:
      /* Note: when "EventType == TX_ON_TIMER" this GPIO is not initialized */
      if (EventType == TX_ON_EVENT)
      {
        osThreadFlagsSet(Thd_LoraSendProcessId, 1);
      }
      break;
    case  BUT2_Pin:
      osThreadFlagsSet(Thd_LoraStopJoinId, 1);
      break;
    case  BUT3_Pin:
      osThreadFlagsSet(Thd_LoraStoreContextId, 1);
      break;
    default:
      break;
  }
}
#endif

/* USER CODE END PB_Callbacks */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */

static void Thd_LoraSendProcess(void *argument)
{
  /* USER CODE BEGIN Thd_LoraSendProcess_1 */

  /* USER CODE END Thd_LoraSendProcess_1 */
  UNUSED(argument);
  for (;;)
  {
    osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
    SendTxData();  /*what you want to do*/
  }

  /* USER CODE BEGIN Thd_LoraSendProcess_2 */

  /* USER CODE END Thd_LoraSendProcess_2 */
}

static void Thd_LoraStoreContext(void *argument)
{
  /* USER CODE BEGIN Thd_LoraStoreContext_1 */

  /* USER CODE END Thd_LoraStoreContext_1 */
  UNUSED(argument);
  for (;;)
  {
    osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
    StoreContext();  /*what you want to do*/
  }

  /* USER CODE BEGIN Thd_LoraStoreContext_2 */

  /* USER CODE END Thd_LoraStoreContext_2 */
}

static void Thd_LoraStopJoin(void *argument)
{
  /* USER CODE BEGIN Thd_LoraStopJoin_1 */

  /* USER CODE END Thd_LoraStopJoin_1 */
  UNUSED(argument);
  for (;;)
  {
    osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
    StopJoin();  /*what you want to do*/
  }

  /* USER CODE BEGIN Thd_LoraStopJoin_2 */

  /* USER CODE END Thd_LoraStopJoin_2 */
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
  uint8_t RxPort = 0;
  
  // ⚠️ DEBUG: Parpadear LED ROJO para confirmar que la función se llama
  for (uint8_t i = 0; i < 5; i++)
  {
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
  }
  
  // ⚠️ DEBUG: Confirmar que la función se llama
  rtos_printf("\r\n*** OnRxData CALLED ***\r\n");

  if (params != NULL)
  {
    rtos_printf("params != NULL: YES\r\n");
    rtos_printf("IsMcpsIndication: %d\r\n", params->IsMcpsIndication);
    
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET); /* LED_BLUE */

    UTIL_TIMER_Start(&RxLedTimer);

    if (params->IsMcpsIndication)
    {
      if (appData != NULL)
      {
        rtos_printf("appData != NULL: YES\r\n");
        RxPort = appData->Port;
        
        // ⚠️ DEBUG: Mostrar información del downlink recibido
        rtos_printf("\r\n>>> DOWNLINK RECEIVED <<<\r\n");
        rtos_printf("Port: %d | Size: %d bytes\r\n", RxPort, appData->BufferSize);
        
        // Mostrar payload en hex
        if (appData->Buffer != NULL && appData->BufferSize > 0)
        {
          rtos_printf("Payload (hex): ");
          for (uint8_t i = 0; i < appData->BufferSize; i++)
          {
            rtos_printf("%02X ", appData->Buffer[i]);
          }
          rtos_printf("\r\n");
        }
        
        if (appData->Buffer != NULL)
        {
          switch (appData->Port)
          {
            case LORAWAN_USER_APP_PORT:  // Puerto 2
              
              // RECEPCIÓN DE HASTA 10 POSICIONES GPS
              if (appData->BufferSize >= 1)
              {
                // Primer byte: número de posiciones
                uint8_t num_positions = appData->Buffer[0];
                
                // Validar número de posiciones
                if (num_positions > 10)
                {
                  rtos_printf("ERROR: Invalid number of positions: %d (max 10)\r\n", num_positions);
                  break;
                }
                
                // Validar tamaño del mensaje
                uint8_t expected_size = 1 + (num_positions * 8);  // 1 byte contador + 8 bytes por posición (4+4 floats)
                if (appData->BufferSize != expected_size)
                {
                  rtos_printf("ERROR: Expected %d bytes for %d positions, received %d bytes\r\n", 
                          expected_size, num_positions, appData->BufferSize);
                  break;
                }
                
                // MOSTRAR ENCABEZADO COMPACTO
                rtos_printf("\r\n>>> GPS RECV: %d positions\r\n", num_positions);
                
                // PROCESAR Y MOSTRAR CADA POSICIÓN
                for (uint8_t i = 0; i < num_positions; i++)
                {
                  uint16_t offset = 1 + (i * 8);  // Offset en el buffer: 8 bytes por posición
                  
                  // Decodificar latitud como float (4 bytes)
                  union {
                    float f;
                    uint8_t bytes[4];
                  } lat_union;
                  
                  lat_union.bytes[0] = appData->Buffer[offset + 0];
                  lat_union.bytes[1] = appData->Buffer[offset + 1];
                  lat_union.bytes[2] = appData->Buffer[offset + 2];
                  lat_union.bytes[3] = appData->Buffer[offset + 3];
                  
                  // Decodificar longitud como float (4 bytes)
                  union {
                    float f;
                    uint8_t bytes[4];
                  } lon_union;
                  
                  lon_union.bytes[0] = appData->Buffer[offset + 4];
                  lon_union.bytes[1] = appData->Buffer[offset + 5];
                  lon_union.bytes[2] = appData->Buffer[offset + 6];
                  lon_union.bytes[3] = appData->Buffer[offset + 7];
                  
                  float latitude = lat_union.f;
                  float longitude = lon_union.f;
                  
                  // Convertir a enteros para display
                  int32_t lat_e6 = (int32_t)(latitude * 1000000);
                  int32_t lon_e6 = (int32_t)(longitude * 1000000);
                  
                  int32_t lat_int = lat_e6 / 1000000;
                  int32_t lat_dec = lat_e6 - (lat_int * 1000000);
                  if (lat_dec < 0) lat_dec = -lat_dec;
                  
                  int32_t lon_int = lon_e6 / 1000000;
                  int32_t lon_dec = lon_e6 - (lon_int * 1000000);
                  if (lon_dec < 0) lon_dec = -lon_dec;
                  
                  // MOSTRAR FORMATO COMPACTO
                  rtos_printf("[%d] Lat:%d.%06d Lon:%d.%06d\r\n", 
                          i + 1, lat_int, lat_dec, lon_int, lon_dec);
                  
                  // Delay para dar tiempo al UART (5ms por posición)
                  HAL_Delay(5);
                }
                
                rtos_printf("DR%d | RX%s | DL#%lu | RSSI:%d SNR:%d\r\n\r\n", 
                        params->Datarate, slotStrings[params->RxSlot], 
                        params->DownlinkCounter, params->Rssi, params->Snr);
                
                // Parpadear LED para indicar recepción exitosa (LED ROJO)
                for (uint8_t blink = 0; blink < 3; blink++)
                {
                  HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET);
                  HAL_Delay(100);
                  HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_RESET);
                  HAL_Delay(100);
                }
              }
              else
              {
                rtos_printf("ERROR: Empty payload received\r\n");
              }
              break;

            /* Comentado - no se usa cambio de clase (solo Clase A) */
            /* case LORAWAN_SWITCH_CLASS_PORT:
              if (appData->BufferSize == 1)
              {
                switch (appData->Buffer[0])
                {
                  case 0:
                  {
                    LmHandlerRequestClass(CLASS_A);
                    break;
                  }
                  case 1:
                  {
                    LmHandlerRequestClass(CLASS_B);
                    break;
                  }
                  case 2:
                  {
                    LmHandlerRequestClass(CLASS_C);
                    break;
                  }
                  default:
                    break;
                }
              }
              break; */

            // RED LED CONTROL (EXAMPLE)
            // case LORAWAN_USER_APP_PORT:
            //   if (appData->BufferSize == 1)
            //   {
            //     AppLedStateOn = appData->Buffer[0] & 0x01;
            //     if (AppLedStateOn == RESET)
            //     {
            //       APP_LOG(TS_OFF, VLEVEL_H, "LED OFF\r\n");
            //       HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* LED_RED */
            //     }
            //     else
            //     {
            //       APP_LOG(TS_OFF, VLEVEL_H, "LED ON\r\n");
            //       HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); /* LED_RED */
            //     }
            //   }
            //   break;

            default:
              APP_LOG(TS_OFF, VLEVEL_H, "WARNING: Downlink received on unhandled port %d\r\n", RxPort);
              break;
          }
        }
        else
        {
          APP_LOG(TS_OFF, VLEVEL_H, "WARNING: appData->Buffer is NULL\r\n");
        }
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_H, "WARNING: appData is NULL\r\n");
      }
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_H, "INFO: Not a McpsIndication (IsMcpsIndication = %d)\r\n", params->IsMcpsIndication);
    }
    
    if (params->RxSlot < RX_SLOT_NONE)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "###### D/L FRAME:%04d | PORT:%d | DR:%d | SLOT:%s | RSSI:%d | SNR:%d\r\n",
              params->DownlinkCounter, RxPort, params->Datarate, slotStrings[params->RxSlot],
              params->Rssi, params->Snr);
    }
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_H, "ERROR: params is NULL in OnRxData\r\n");
  }
  /* USER CODE END OnRxData_1 */
}

static void SendTxData(void)
{
  /* USER CODE BEGIN SendTxData_1 */
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;
  //uint8_t batteryLevel = GetBatteryLevel();
  //sensor_t sensor_data;
  UTIL_TIMER_Time_t nextTxIn = 0;

  if (LmHandlerIsBusy() == false) {
    float latitude = -34.603722f;   // Ejemplo: Buenos Aires
    float longitude = -58.381592f;

    uint32_t i = 0;
    AppData.Port = LORAWAN_USER_APP_PORT;  // Puerto 2

    // Serializar floats a bytes (4 bytes cada uno)
    // Método 1: Union (más directo)
    union {
      float f;
      uint8_t bytes[4]; // 4 bytes para representar el float
    } lat_union, lon_union;
    
    lat_union.f = latitude;
    lon_union.f = longitude;

    // Copiar bytes de latitud (4 bytes)
    AppData.Buffer[i++] = lat_union.bytes[0];
    AppData.Buffer[i++] = lat_union.bytes[1];
    AppData.Buffer[i++] = lat_union.bytes[2];
    AppData.Buffer[i++] = lat_union.bytes[3];
    
    // Copiar bytes de longitud (4 bytes)
    AppData.Buffer[i++] = lon_union.bytes[0];
    AppData.Buffer[i++] = lon_union.bytes[1];
    AppData.Buffer[i++] = lon_union.bytes[2];
    AppData.Buffer[i++] = lon_union.bytes[3];

    AppData.BufferSize = i;  // Total: 8 bytes

    APP_LOG(TS_OFF, VLEVEL_H, "Sending GPS: Lat=%.6f, Lon=%.6f\r\n", latitude, longitude);

    // Detiene LED de Join si ya está conectado
    if ((JoinLedTimer.IsRunning) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET)) {
      UTIL_TIMER_Stop(&JoinLedTimer);
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_RESET);
    }

    // ENVÍA EL MENSAJE
    status = LmHandlerSend(&AppData, LmHandlerParams.IsTxConfirmed, false);
    
    if (LORAMAC_HANDLER_SUCCESS == status) {
      APP_LOG(TS_OFF, VLEVEL_H, "SEND REQUEST\r\n");
    }
    else if (LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED == status) {
      nextTxIn = LmHandlerGetDutyCycleWaitTime();
      if (nextTxIn > 0)
      {
        APP_LOG(TS_OFF, VLEVEL_H, "Next Tx in  : ~%d second(s)\r\n", (nextTxIn / 1000));
      }
    }
  }
  
  // Reinicia timer para próximo envío
  if (EventType == TX_ON_TIMER) {
    UTIL_TIMER_Stop(&TxTimer);
    UTIL_TIMER_SetPeriod(&TxTimer, MAX(nextTxIn, TxPeriodicity));
    UTIL_TIMER_Start(&TxTimer);
  }

  /* Estructura del mensaje enviado:
    Byte 0-3: Latitud (float, little-endian)
    Byte 4-7: Longitud (float, little-endian)
    Total: 8 bytes
  */

  /* USER CODE END SendTxData_1 */
}

static void OnTxTimerEvent(void *context)
{
  /* USER CODE BEGIN OnTxTimerEvent_1 */

  /* USER CODE END OnTxTimerEvent_1 */
  osThreadFlagsSet(Thd_LoraSendProcessId, 1);

  /*Wait for next tx slot*/
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxTimerEvent_2 */

  /* USER CODE END OnTxTimerEvent_2 */
}

/* USER CODE BEGIN PrFD_LedEvents */

/* USER CODE END PrFD_LedEvents */

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
  if ((params != NULL))
  {
    /* Process Tx event only if its a mcps response to prevent some internal events (mlme) */
    if (params->IsMcpsConfirm != 0)
    {
      HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_SET); /* LED_GREEN */
      UTIL_TIMER_Start(&TxLedTimer);

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### ========== MCPS-Confirm =============\r\n");
      APP_LOG(TS_OFF, VLEVEL_M, "###### U/L FRAME:%04d | PORT:%d | DR:%d | PWR:%d", params->UplinkCounter,
              params->AppData.Port, params->Datarate, params->TxPower);

      APP_LOG(TS_OFF, VLEVEL_M, " | MSG TYPE:");
      if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "CONFIRMED [%s]\r\n", (params->AckReceived != 0) ? "ACK" : "NACK");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "UNCONFIRMED\r\n");
      }
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n");
    }
  }
  /* USER CODE END OnTxData_1 */
}
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
  if (joinParams != NULL)
  {
    if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
    {
      UTIL_TIMER_Stop(&JoinLedTimer);
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_RESET); /* LED_RED */

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOINED = ");
      if (joinParams->Mode == ACTIVATION_TYPE_ABP)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "ABP ======================\r\n");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "OTAA =====================\r\n");
      }
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOIN FAILED\r\n");
    }

    APP_LOG(TS_OFF, VLEVEL_H, "###### U/L FRAME:JOIN | DR:%d | PWR:%d\r\n", joinParams->Datarate, joinParams->TxPower);
  }
  /* USER CODE END OnJoinRequest_1 */
}

static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params)
{
  /* USER CODE BEGIN OnBeaconStatusChange_1 */
  if (params != NULL)
  {
    switch (params->State)
    {
      default:
      case LORAMAC_HANDLER_BEACON_LOST:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON LOST\r\n");
        break;
      }
      case LORAMAC_HANDLER_BEACON_RX:
      {
        APP_LOG(TS_OFF, VLEVEL_M,
                "\r\n###### BEACON RECEIVED | DR:%d | RSSI:%d | SNR:%d | FQ:%d | TIME:%d | DESC:%d | "
                "INFO:02X%02X%02X %02X%02X%02X\r\n",
                params->Info.Datarate, params->Info.Rssi, params->Info.Snr, params->Info.Frequency,
                params->Info.Time.Seconds, params->Info.GwSpecific.InfoDesc,
                params->Info.GwSpecific.Info[0], params->Info.GwSpecific.Info[1],
                params->Info.GwSpecific.Info[2], params->Info.GwSpecific.Info[3],
                params->Info.GwSpecific.Info[4], params->Info.GwSpecific.Info[5]);
        break;
      }
      case LORAMAC_HANDLER_BEACON_NRX:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON NOT RECEIVED\r\n");
        break;
      }
    }
  }
  /* USER CODE END OnBeaconStatusChange_1 */
}
static void OnSysTimeUpdate(void)
{
  /* USER CODE BEGIN OnSysTimeUpdate_1 */

  /* USER CODE END OnSysTimeUpdate_1 */
}

static void OnClassChange(DeviceClass_t deviceClass)
{
  /* USER CODE BEGIN OnClassChange_1 */
  APP_LOG(TS_OFF, VLEVEL_M, "Switch to Class %c done\r\n", "ABC"[deviceClass]);
  /* USER CODE END OnClassChange_1 */
}


static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */

  /* USER CODE END OnMacProcessNotify_1 */
}

static void OnTxPeriodicityChanged(uint32_t periodicity)
{
  /* USER CODE BEGIN OnTxPeriodicityChanged_1 */

  /* USER CODE END OnTxPeriodicityChanged_1 */
  TxPeriodicity = periodicity;

  if (TxPeriodicity == 0)
  {
    /* Revert to application default periodicity */
    TxPeriodicity = APP_TX_DUTYCYCLE;
  }

  /* Update timer periodicity */
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_SetPeriod(&TxTimer, TxPeriodicity);
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxPeriodicityChanged_2 */

  /* USER CODE END OnTxPeriodicityChanged_2 */
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed)
{
  /* USER CODE BEGIN OnTxFrameCtrlChanged_1 */

  /* USER CODE END OnTxFrameCtrlChanged_1 */
  LmHandlerParams.IsTxConfirmed = isTxConfirmed;
  /* USER CODE BEGIN OnTxFrameCtrlChanged_2 */

  /* USER CODE END OnTxFrameCtrlChanged_2 */
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity)
{
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_1 */

  /* USER CODE END OnPingSlotPeriodicityChanged_1 */
  LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_2 */

  /* USER CODE END OnPingSlotPeriodicityChanged_2 */
}

static void OnSystemReset(void)
{
  /* USER CODE BEGIN OnSystemReset_1 */

  /* USER CODE END OnSystemReset_1 */
  if ((LORAMAC_HANDLER_SUCCESS == LmHandlerHalt()) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
  {
    NVIC_SystemReset();
  }
  /* USER CODE BEGIN OnSystemReset_Last */

  /* USER CODE END OnSystemReset_Last */
}

static void StopJoin(void)
{
  /* USER CODE BEGIN StopJoin_1 */
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET); /* LED_BLUE */
  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_SET); /* LED_GREEN */
  HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET); /* LED_RED */
  /* USER CODE END StopJoin_1 */

  UTIL_TIMER_Stop(&TxTimer);

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerStop())
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stop on going ...\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stopped\r\n");
    if (LORAWAN_DEFAULT_ACTIVATION_TYPE == ACTIVATION_TYPE_ABP)
    {
      ActivationType = ACTIVATION_TYPE_OTAA;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to OTAA mode\r\n");
    }
    else
    {
      ActivationType = ACTIVATION_TYPE_ABP;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to ABP mode\r\n");
    }
    LmHandlerConfigure(&LmHandlerParams);
    LmHandlerJoin(ActivationType, true);
    UTIL_TIMER_Start(&TxTimer);
  }
  UTIL_TIMER_Start(&StopJoinTimer);
  /* USER CODE BEGIN StopJoin_Last */

  /* USER CODE END StopJoin_Last */
}

static void OnStopJoinTimerEvent(void *context)
{
  /* USER CODE BEGIN OnStopJoinTimerEvent_1 */

  /* USER CODE END OnStopJoinTimerEvent_1 */
  if (ActivationType == LORAWAN_DEFAULT_ACTIVATION_TYPE)
  {
    osThreadFlagsSet(Thd_LoraStopJoinId, 1);
  }
  /* USER CODE BEGIN OnStopJoinTimerEvent_Last */
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET); /* LED_BLUE */
  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET); /* LED_GREEN */
  HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_RESET); /* LED_RED */
  /* USER CODE END OnStopJoinTimerEvent_Last */
}


static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  /* USER CODE BEGIN StoreContext_1 */

  /* USER CODE END StoreContext_1 */
  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\n");
  }
  /* USER CODE BEGIN StoreContext_Last */

  /* USER CODE END StoreContext_Last */
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  /* USER CODE BEGIN OnNvmDataChange_1 */

  /* USER CODE END OnNvmDataChange_1 */
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\n");
  }
  /* USER CODE BEGIN OnNvmDataChange_Last */

  /* USER CODE END OnNvmDataChange_Last */
}

static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnStoreContextRequest_1 */

  /* USER CODE END OnStoreContextRequest_1 */
  FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);

  /* USER CODE BEGIN OnStoreContextRequest_Last */

  /* USER CODE END OnStoreContextRequest_Last */
}

static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnRestoreContextRequest_1 */

  /* USER CODE END OnRestoreContextRequest_1 */
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  /* USER CODE BEGIN OnRestoreContextRequest_Last */

  /* USER CODE END OnRestoreContextRequest_Last */
}



/* USER CODE BEGIN PrFD_LedEvents */
static void OnTxTimerLedEvent(void *context)
{
  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET); /* LED_GREEN */
}

static void OnRxTimerLedEvent(void *context)
{
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET); /* LED_BLUE */
}

static void OnJoinTimerLedEvent(void *context)
{
  HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN); /* LED_RED */
}