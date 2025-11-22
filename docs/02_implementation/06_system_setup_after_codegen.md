# TPP-IntelliFence - System Setup After Code Generation

## 🎯 Objetivo

Este documento describe las **modificaciones manuales críticas** que deben realizarse después de regenerar código con STM32CubeMX/CubeIDE. Estas configuraciones NO se preservan automáticamente y deben aplicarse cada vez.

---

## ⚙️ FreeRTOSConfig.h - Configuraciones Críticas

**Ubicación**: `CM4/Core/Inc/FreeRTOSConfig.h`

### 1. 🔧 Configuraciones Básicas del Kernel

Estas ya vienen configuradas por STM32CubeMX, pero verifica que estén presentes:

```c
#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     ( 56 )
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)10000)
#define configMAX_TASK_NAME_LEN                  ( 16 )
```

**Ajustar si es necesario**:
- `configTOTAL_HEAP_SIZE`: 10KB es suficiente para nuestro sistema
- `configMAX_PRIORITIES`: 56 (CubeMX default)

---

### 2. 🐛 CRITICAL: SysTick Handler Override

**⚠️ OBLIGATORIO DESPUÉS DE CADA REGENERACIÓN**

Agregar al final del archivo, en la sección `USER CODE BEGIN Defines`:

```c
/* USER CODE BEGIN Defines */
/* Section where parameter definitions can be added */

/* IMPORTANT: This define MUST be uncommented when HAL uses alternate timebase (TIM2)
              to allow FreeRTOS to control SysTick directly. This prevents timing conflicts
              that cause WWDG_IRQHandler exceptions during UART operations. */

#define xPortSysTickHandler SysTick_Handler

/* USER CODE END Defines */
```

**Por qué es crítico**:
- Permite a FreeRTOS controlar SysTick directamente
- Previene conflictos de timing con HAL
- **Sin esto**: WWDG_IRQHandler exceptions durante operaciones UART
- Ver: [WWDG Solution](../02_implementation/04_freertos_wwdg.md)

---

### 3. 📊 Runtime Statistics (Debugging)

**⚠️ OBLIGATORIO PARA RTOS VIEWS Y PROFILING**

Agregar en `USER CODE BEGIN Defines`:

```c
/* Configuraciones para RTOS Views debugging */
#define configRECORD_STACK_HIGH_ADDRESS          1

/* Runtime statistics configuration */
#define configGENERATE_RUN_TIME_STATS            1
#define configUSE_STATS_FORMATTING_FUNCTIONS     1

/* High-speed counter for runtime stats - using DWT cycle counter */
extern void vConfigureTimerForRunTimeStats(void);
extern uint32_t vGetRunTimeCounterValue(void);

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vConfigureTimerForRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE()         vGetRunTimeCounterValue()
```

**Funciones a implementar en `main.c`** (ver sección siguiente):

```c
/* USER CODE BEGIN 0 */
static uint32_t uwRuntimeStatsCounter = 0;

void vConfigureTimerForRunTimeStats(void) {
    // Enable DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Reset cycle counter
    DWT->CYCCNT = 0;
    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t vGetRunTimeCounterValue(void) {
    return DWT->CYCCNT / (SystemCoreClock / 10000);
}
/* USER CODE END 0 */
```

**Beneficios**:
- Permite usar `vTaskGetRunTimeStats()` para profiling
- Visualización en STM32CubeIDE RTOS Views
- Detección de stack overflows con high water mark
- Ver: [RTOS Debug Views](../02_implementation/03_rtos_debug_views.md)

---

### 4. 🔍 Debugging Features

**Recomendado para Development**:

```c
#define configUSE_TRACE_FACILITY                 1
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
```

**API Functions a incluir**:

```c
#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskDelayUntil              1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTimerPendFunctionCall       1
#define INCLUDE_xQueueGetMutexHolder         1
#define INCLUDE_uxTaskGetStackHighWaterMark  1
#define INCLUDE_eTaskGetState                1
```

---

### 5. 🧠 Heap Configuration

**Por defecto usa heap_5** (recomendado):

```c
/*
 * The CMSIS-RTOS V2 FreeRTOS wrapper is dependent on the heap implementation used
 * by the application thus the correct define need to be enabled below
 */
#define USE_FreeRTOS_HEAP_5
```

**Alternativas**:
- `USE_FreeRTOS_HEAP_4`: Pool allocation (usado en nuestro MessagePool)
- `USE_FreeRTOS_HEAP_1`: Static allocation only
- `USE_FreeRTOS_HEAP_3`: Wrapper de malloc/free

---

### 6. ⚡ Interrupt Priorities

**Verificar configuración correcta**:

```c
/* Cortex-M specific definitions */
#define configPRIO_BITS         4  // STM32WL55 tiene 4 bits

/* Lowest interrupt priority (15) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15

/* Max syscall interrupt priority (5)
   Solo interrupciones con prioridad >= 5 pueden usar FreeRTOS API */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
```

**Regla de oro**:
- Interrupciones de **alta prioridad (0-4)**: NO pueden llamar FreeRTOS API
- Interrupciones de **prioridad normal (5-15)**: Pueden usar `FromISR()` functions

**Ejemplo**:
```c
// UART interrupt - debe ser >= 5 para usar xQueueSendFromISR()
HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);

// Critical hardware interrupt - puede ser 0-4
HAL_NVIC_SetPriority(TIM1_UP_IRQn, 2, 0);
```

---

### 7. 🛡️ Assert Configuration

**Recomendado para debugging**:

```c
/* USER CODE BEGIN 1 */
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}
/* USER CODE END 1 */
```

**Para producción**:
```c
// Comentar para evitar overhead
// #define configASSERT( x )
```

---

## 📝 main.c - Implementaciones Requeridas

### 1. Runtime Stats Counter (en USER CODE 0)

```c
/* USER CODE BEGIN 0 */

// Runtime statistics counter para RTOS profiling
static uint32_t uwRuntimeStatsCounter = 0;

/**
 * @brief  Configura DWT cycle counter para runtime statistics
 * @note   Llamado automáticamente por FreeRTOS si configGENERATE_RUN_TIME_STATS = 1
 */
void vConfigureTimerForRunTimeStats(void) {
    // Enable DWT (Data Watchpoint and Trace)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    // Reset cycle counter
    DWT->CYCCNT = 0;
    
    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  Obtiene valor del counter para runtime stats
 * @return Contador en unidades de 0.1ms (10kHz)
 * @note   DWT cuenta ciclos de CPU, se divide por (SystemCoreClock / 10000)
 *         para convertir a 0.1ms ticks
 */
uint32_t vGetRunTimeCounterValue(void) {
    // Convertir ciclos de CPU a unidades de 0.1ms
    return DWT->CYCCNT / (SystemCoreClock / 10000);
}

/* USER CODE END 0 */
```

---

### 2. Printf con Floating Point (opcional pero recomendado)

**En `syscalls.c`** (si usas retargeting de printf):

Verificar que el linker tenga habilitado floating point printf:

**CMakeLists.txt**:
```cmake
target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    -u _printf_float  # Habilita printf con %f
)
```

---

## 🔄 Checklist Post-Regeneración

Después de regenerar código con STM32CubeMX:

- [ ] **FreeRTOSConfig.h**:
  - [ ] Agregar `#define xPortSysTickHandler SysTick_Handler`
  - [ ] Agregar runtime stats defines
  - [ ] Verificar heap size (20000)
  - [ ] Verificar interrupt priorities

- [ ] **main.c**:
  - [ ] Implementar `vConfigureTimerForRunTimeStats()`
  - [ ] Implementar `vGetRunTimeCounterValue()`
  - [ ] Verificar que tasks estén creadas correctamente

- [ ] **CMakeLists.txt**:
  - [ ] Verificar `-u _printf_float` en linker options
  - [ ] Verificar fuentes custom agregadas

- [ ] **Compilar y verificar**:
  - [ ] No hay warnings de stack size
  - [ ] RTOS Views funcionan correctamente
  - [ ] Printf con floats funciona (%f)
  - [ ] No hay WWDG exceptions

---

## 🚨 Problemas Comunes

### WWDG_IRQHandler Exception

**Síntoma**: Exception durante operaciones UART/I2C
**Causa**: SysTick no configurado para FreeRTOS
**Solución**: Agregar `#define xPortSysTickHandler SysTick_Handler`
**Referencia**: [WWDG Solution](../02_implementation/04_freertos_wwdg.md)

---

### RTOS Views No Funcionan

**Síntoma**: Ventanas RTOS vacías en debugger
**Causa**: Runtime stats no habilitados
**Solución**: 
1. Agregar defines de runtime stats en FreeRTOSConfig.h
2. Implementar funciones DWT en main.c
**Referencia**: [RTOS Debug Views](../02_implementation/03_rtos_debug_views.md)

---

### Printf No Muestra Floats

**Síntoma**: `printf("%.2f", 3.14)` muestra basura o `?`
**Causa**: Linker no tiene `-u _printf_float`
**Solución**: Agregar a CMakeLists.txt:
```cmake
target_link_options(${PROJECT_NAME} PRIVATE -u _printf_float)
```
**Referencia**: [RTOS Printf](../02_implementation/02_rtos_printf.md)

---

### Stack Overflow

**Síntoma**: HardFault, task stops working
**Causa**: Stack size insuficiente
**Solución**: 
```c
// Aumentar stack size en osThreadDef
osThreadDef(taskName, StartTask, osPriorityNormal, 0, 256); // Era 128
```

**Verificar con**:
```c
UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
printf("Stack remaining: %lu words\n", watermark);
```

---

## 📊 Configuraciones de Ejemplo para TPP-IntelliFence

### FreeRTOSConfig.h Completo

```c
#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     ( 56 )
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)20000)
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_TRACE_FACILITY                 1
#define configUSE_16_BIT_TICKS                   0
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1

/* Software timer definitions */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( 2 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

/* Heap allocation */
#define USE_FreeRTOS_HEAP_5

/* USER CODE BEGIN Defines */

/* CRITICAL: SysTick override for HAL timebase compatibility */
#define xPortSysTickHandler SysTick_Handler

/* RTOS Views debugging */
#define configRECORD_STACK_HIGH_ADDRESS          1

/* Runtime statistics */
#define configGENERATE_RUN_TIME_STATS            1
#define configUSE_STATS_FORMATTING_FUNCTIONS     1

extern void vConfigureTimerForRunTimeStats(void);
extern uint32_t vGetRunTimeCounterValue(void);

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vConfigureTimerForRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE()         vGetRunTimeCounterValue()

/* USER CODE END Defines */
```

---

### Task Stack Sizes (main.c)

```c
/* Definitions for FSM Task */
osThreadDef(fsmTask, StartFSMTask, osPriorityNormal, 0, 1024);

/* Definitions for Sensor Acquisition Task */
osThreadDef(sensorAcqTask, StartSensorAcqTask, osPriorityNormal, 0, 512);

/* Definitions for Dispatcher Task */
osThreadDef(dispatcherTask, StartDispatcherTask, osPriorityHigh, 0, 256);

/* Definitions for LoRa Task (CM0+) */
osThreadDef(loraTask, StartLoRaTask, osPriorityNormal, 0, 512);
```

---

## 📖 Referencias

- [FreeRTOS Best Practices](../02_implementation/01_freertos_best_practices.md)
- [RTOS Printf Guide](../02_implementation/02_rtos_printf.md)
- [RTOS Debug Views](../02_implementation/03_rtos_debug_views.md)
- [WWDG Solution](../02_implementation/04_freertos_wwdg.md)

---

**Última actualización**: 22 de Noviembre de 2025  
**Versión**: v1.0
