# 🛡️ Mejores Prácticas: FreeRTOS en STM32WL55JC

## 📋 Configuración Timebase

### ✅ Configuración Correcta

```c
// FreeRTOSConfig.h
#define xPortSysTickHandler SysTick_Handler  // ✅ SIEMPRE descomentado con HAL alternativo

// stm32wlxx_hal_timebase_tim.c
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    __HAL_RCC_TIM2_CLK_ENABLE();  // ✅ HAL usa TIM2
    // ... configuración correcta
}
```

### ❌ Configuraciones Problemáticas

```c
// ❌ NUNCA: FreeRTOS y HAL compitiendo por SysTick
/* #define xPortSysTickHandler SysTick_Handler */  // Comentado + HAL usa SysTick

// ❌ NUNCA: Ambos usando el mismo timer
#define HAL_TIM_MODULE_ENABLED  // HAL usa TIM2
#define configUSE_TIMERS 1      // FreeRTOS también usa TIM2
```

## ⚠️ Printf en Threads - Consideraciones Importantes

### ✅ Printf Seguro (Solo en Main Context)

```c
// main.c - ANTES de osKernelStart()
int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    printf("Sistema inicializando...\r\n");  // ✅ SEGURO
    
    osKernelInitialize();
    MX_FREERTOS_Init();
    
    printf("FreeRTOS configurado\r\n");      // ✅ SEGURO
    
    osKernelStart();  // Después de esto, NO más printf directo
    
    // Este código nunca se ejecuta si FreeRTOS funciona
    while(1) {
        printf("ERROR - FreeRTOS falló\r\n"); // ✅ SEGURO (contexto de error)
    }
}
```

### ⚠️ Printf en Threads - Funciona con Configuración Correcta

```c
// threads/anyTask.cpp
void anyTask(void *argument) {
    printf("Task iniciado\r\n");           // ✅ FUNCIONA con SysTick configurado correctamente
    
    while(1) {
        printf("Iteración %d\r\n", i++);   // ✅ FUNCIONA, pero considerar performance
        osDelay(1000);
    }
}
```

### ⚠️ Consideraciones de Performance

- ✅ **Printf funciona** después de corregir configuración SysTick
- ⚠️ **Printf es lento** - puede afectar timing crítico
- ⚠️ **Printf usa buffer** - puede consumir stack/heap  
- ⚠️ **Printf bloquea** durante transmisión UART

### 🔧 Alternativas Recomendadas para Debugging

#### Opción 1: Variables Globales (Más Simple)

```c
// En el archivo del thread
volatile uint32_t task_status = 0;
volatile uint32_t task_iterations = 0;
volatile uint32_t task_last_error = 0;

void myTask(void *argument) {
    task_status = 1; // running
    
    while(1) {
        task_iterations++;
        
        // Lógica del task...
        if (error_occurred) {
            task_last_error = error_code;
        }
        
        osDelay(100);
    }
}

// Accesible desde debugger o desde main context
extern volatile uint32_t task_status;
extern volatile uint32_t task_iterations;
```

#### Opción 2: UART Thread-Safe (Más Complejo)

```c
// ThreadSafeUART.h
void ThreadSafeUART_Printf(const char* format, ...);

// ThreadSafeUART.c
static osMutexId_t uart_mutex = NULL;

void ThreadSafeUART_Printf(const char* format, ...) {
    if (osMutexAcquire(uart_mutex, 1000) == osOK) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // Envío directo por UART sin printf
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
        
        osMutexRelease(uart_mutex);
    }
}
```

#### Opción 3: Queue de Debug Messages

```c
// Sistema de cola para mensajes de debug
typedef struct {
    char message[64];
    uint32_t timestamp;
    uint8_t priority;
} DebugMessage_t;

osMessageQueueId_t debugQueueHandle;

// En threads
void sendDebugMessage(const char* msg) {
    DebugMessage_t debug_msg;
    strncpy(debug_msg.message, msg, sizeof(debug_msg.message)-1);
    debug_msg.timestamp = osKernelGetTickCount();
    debug_msg.priority = 1;
    
    osMessageQueuePut(debugQueueHandle, &debug_msg, 0, 0);
}

// Thread dedicado para debug (baja prioridad)
void debugTask(void *argument) {
    DebugMessage_t msg;
    while(1) {
        if (osMessageQueueGet(debugQueueHandle, &msg, NULL, osWaitForever) == osOK) {
            printf("[%lu] %s\r\n", msg.timestamp, msg.message);
        }
    }
}
```

## ⚡ Heap y Stack Management

### Stack Size por Tipo de Thread

```c
// Threads simples (solo variables locales)
const osThreadAttr_t simple_task_attributes = {
    .name = "SimpleTask",
    .stack_size = 128 * 4,  // 512 bytes
    .priority = osPriorityNormal,
};

// Threads con printf/string operations
const osThreadAttr_t complex_task_attributes = {
    .name = "ComplexTask", 
    .stack_size = 256 * 4,  // 1024 bytes
    .priority = osPriorityNormal,
};

// Threads con buffers grandes o recursión
const osThreadAttr_t heavy_task_attributes = {
    .name = "HeavyTask",
    .stack_size = 512 * 4,  // 2048 bytes  
    .priority = osPriorityNormal,
};
```

### Heap Configuration (RAM2)

```c
// heap_config.c - Configuración óptima para STM32WL55JC
static const HeapRegion_t xHeapRegions[] = {
    { (uint8_t*)0x20008000, 8192 },  // 8KB en RAM2 para FreeRTOS
    { NULL, 0 }
};

void vApplicationSetupHeap(void) {
    vPortDefineHeapRegions(xHeapRegions);
}
```

## 🔍 Debugging y Monitoring

### Funciones de Monitoreo Recomendadas

```c
// En main.c - Para llamar desde debugger o interrupciones
void print_system_status(void) {
    printf("\n=== SYSTEM STATUS ===\r\n");
    
    // FreeRTOS Stats
    size_t free_heap = xPortGetFreeHeapSize();
    size_t min_heap = xPortGetMinimumEverFreeHeapSize();
    printf("Heap: %u bytes free, %u min ever free\r\n", 
           (unsigned)free_heap, (unsigned)min_heap);
    
    // Task Stats (requiere configUSE_TRACE_FACILITY = 1)
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    printf("Active tasks: %u\r\n", (unsigned)task_count);
    
    // Variables de debugging de threads
    extern volatile uint32_t dispatcher_status;
    extern volatile uint32_t dispatcher_iterations;
    printf("Dispatcher: status=%u, iterations=%u\r\n",
           (unsigned)dispatcher_status, (unsigned)dispatcher_iterations);
           
    printf("=====================\n\r\n");
}

// Función para verificar stack overflow
void check_stack_usage(void) {
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    UBaseType_t stack_high_water = uxTaskGetStackHighWaterMark(task_handle);
    
    if (stack_high_water < 50) {  // Menos de 50 words libres
        // ALERTA: Stack casi lleno
        task_last_error = STACK_OVERFLOW_WARNING;
    }
}
```

### Watchpoints Útiles en Debugger

```c
// Variables para monitorear en tiempo real
volatile uint32_t system_tick_count = 0;
volatile uint32_t last_context_switch = 0;
volatile uint32_t heap_allocations = 0;

// Actualizar en hooks de FreeRTOS
void vApplicationTickHook(void) {
    system_tick_count++;
}

void vApplicationMallocFailedHook(void) {
    heap_allocations = 0xDEADBEEF;  // Indicador de fallo
}
```

## ⚠️ Señales de Alerta

### Síntomas de Problemas de Timing

- ✋ **WWDG_IRQHandler** se dispara
- ✋ **HardFault_Handler** en operaciones de FreeRTOS  
- ✋ **Tasks no responden** (variables de debugging no cambian)
- ✋ **osDelay() no funciona** correctamente
- ✋ **Interrupciones perdidas** o timing errático

### Síntomas de Stack Overflow

- ✋ **Variables locales corruptas**
- ✋ **Return address corrupto** (funciones retornan a direcciones inválidas)
- ✋ **Comportamiento errático** del thread
- ✋ **HardFault** en llamadas a funciones simples

### Síntomas de Heap Exhausted

- ✋ **osThreadNew()** retorna NULL
- ✋ **osMessageQueueNew()** falla
- ✋ **pvPortMalloc()** retorna NULL
- ✋ **vApplicationMallocFailedHook()** se ejecuta

## 🔧 Herramientas de Debugging

### 1. FreeRTOS Runtime Stats

```c
// FreeRTOSConfig.h
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

// En main.c
void print_task_stats(void) {
    char stats_buffer[1024];
    vTaskGetRunTimeStats(stats_buffer);
    printf("Runtime Stats:\n%s\r\n", stats_buffer);
}
```

### 2. STM32CubeMonitor Integration

```c
// Variables exportadas para STM32CubeMonitor
__attribute__((used)) volatile uint32_t monitor_heap_free;
__attribute__((used)) volatile uint32_t monitor_cpu_usage;
__attribute__((used)) volatile uint32_t monitor_task_switches;

// Actualizar periódicamente
void update_monitor_variables(void) {
    monitor_heap_free = xPortGetFreeHeapSize();
    monitor_cpu_usage = osKernelGetTickCount();
    monitor_task_switches = uxTaskGetNumberOfTasks();
}
```

---
**Recuerda**: Estos problemas de timing son sutiles pero críticos en sistemas embebidos. Siempre verifica la configuración de timebase al integrar FreeRTOS con HAL de STM32.