# Solución de Problema: WWDG Timeout en FreeRTOS + STM32WL55JC

## 📋 Resumen del Problema

**Síntoma**: El sistema se colgaba con `WWDG_IRQHandler` durante la ejecución de threads de FreeRTOS, específicamente cuando se ejecutaba `printf()` dentro de tareas.

**Error observado**:
```
WWDG_IRQHandler (startup_stm32wl55xx_cm4.s:118)
<signal handler called>
UART_WaitOnFlagUntilTimeout (stm32wlxx_hal_uart.c:3459)
HAL_UART_Transmit (stm32wlxx_hal_uart.c:1190)
__io_putchar (stm32wlxx_nucleo.c:598)
_write (syscalls.c:87)
printf -> dispatcherTask (dispatcherTask.cpp:16)
```

## 🔍 Análisis de la Causa Raíz

### Problema Principal: Conflicto de Timebase entre HAL y FreeRTOS

1. **STM32CubeMX configuró HAL para usar TIM2** como timebase (correcto para FreeRTOS)
2. **FreeRTOS no tenía control del SysTick** debido a configuración incorrecta
3. **Desincronización de timers** causaba timeouts del Watchdog durante operaciones normales
4. **Printf en threads** exponía el problema por las esperas en UART, pero **NO era la causa raíz**

**IMPORTANTE**: Después de corregir la configuración SysTick, **printf funciona correctamente en threads**.

### Configuración Problemática:

**En `FreeRTOSConfig.h`**:
```c
/* #define xPortSysTickHandler SysTick_Handler */  // ❌ COMENTADO
```

**En `stm32wlxx_hal_timebase_tim.c`**:
```c
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    // HAL usa TIM2 correctamente ✅
    __HAL_RCC_TIM2_CLK_ENABLE();
}
```

## ✅ Solución Implementada

### 1. Corrección de Configuración SysTick

**Archivo**: `CM4/Core/Inc/FreeRTOSConfig.h`

**Cambio realizado**:
```c
// ANTES (problemático):
/* IMPORTANT: This define is commented when used with STM32Cube firmware, when the timebase source is SysTick,
              to prevent overwriting SysTick_Handler defined within STM32Cube HAL */
/* #define xPortSysTickHandler SysTick_Handler */

// DESPUÉS (correcto):
/* IMPORTANT: This define SHOULD be uncommented when HAL uses alternate timebase (TIM2)
              to allow FreeRTOS to control SysTick directly */
#define xPortSysTickHandler SysTick_Handler
```

### 2. Configuración de Variables de Debugging

**Archivo**: `CM4/Core/Src/threads/dispatcherTask.cpp`

**Nota**: Después de corregir SysTick, `printf()` funciona correctamente en threads.

**Variables de debugging agregadas** (útiles para monitoreo):

```cpp
// Variables globales para debugging (visibles en debugger)
volatile uint32_t dispatcher_status = 0;        // 0=init, 1=running, 2=error
volatile uint32_t dispatcher_iterations = 0;
volatile uint32_t dispatcher_messages_received = 0;
volatile uint32_t dispatcher_last_message_id = 0;
volatile uint32_t dispatcher_last_sender = 0;
volatile uint32_t dispatcher_last_receiver = 0;

void dispatcherTask(void *argument) {
    // Marcar estado y usar printf (ahora funciona correctamente)
    dispatcher_status = 1; // running
    
    printf("[DISPATCHER] Task initialized\r\n"); // ✅ FUNCIONA después de fix SysTick
    
    while(1) {
        dispatcher_iterations++; // Variables de debugging útiles
        
        if (osMessageQueueGet(dispatcherQueueHandle, &msg, NULL, 0) == osOK) {
            dispatcher_messages_received++;
            dispatcher_last_message_id = msg->id;
            
            // Printf también funciona aquí si se necesita
            printf("[DISPATCHER] Message ID %d from %d to %d\r\n", 
                   msg->id, msg->sender, msg->receiver);
            
            // ... resto del código
        }
        osDelay(100);
    }
}
```

## 📊 Configuración Final del Sistema

### Distribución de Timers:

| Componente | Timer Usado | Frecuencia | Propósito |
|------------|-------------|------------|-----------|
| **HAL Timebase** | TIM2 | 1000 Hz | Funciones HAL (HAL_Delay, etc.) |
| **FreeRTOS Scheduler** | SysTick | 1000 Hz | Context switching, osDelay |
| **UART Communication** | Interrupciones | Variable | Printf, comunicación serie |

### Memory Layout (después de optimizaciones):

```
Memory region         Used Size  Region Size  %age Used
             ROM:       57928 B       128 KB     44.20%
            RAM1:        9008 B        16 KB     54.98%
            RAM2:           0 B        16 KB      0.00% (reservado para heap FreeRTOS)
```

## 🛠️ Debugging Recomendado

### Variables de Monitoreo (Accesibles desde Debugger):

```cpp
// Estado del Dispatcher
extern volatile uint32_t dispatcher_status;         // 1 = funcionando
extern volatile uint32_t dispatcher_iterations;     // Contador de loops
extern volatile uint32_t dispatcher_messages_received; // Mensajes procesados

// Información del último mensaje
extern volatile uint32_t dispatcher_last_message_id;
extern volatile uint32_t dispatcher_last_sender;
extern volatile uint32_t dispatcher_last_receiver;
```

### Función de Debug desde Main (Thread-Safe):

```cpp
// Llamar desde main.c (no desde threads)
void print_dispatcher_debug_info() {
    printf("[DEBUG] Dispatcher - Status:%u, Iter:%u, Msgs:%u\r\n",
           (unsigned)dispatcher_status, 
           (unsigned)dispatcher_iterations, 
           (unsigned)dispatcher_messages_received);
}
```

## 🔧 Lecciones Aprendidas

### 1. **FreeRTOS + STM32Cube Timebase Configuration**
- ✅ **HAL debe usar timer alternativo** (TIM2, TIM3, etc.)
- ✅ **FreeRTOS debe controlar SysTick** completamente
- ❌ **Nunca ambos usen SysTick** simultáneamente

### 2. **Printf en Sistemas Embebidos**
- ✅ **Printf es seguro en main context** (antes de scheduler)
- ❌ **Printf NO es thread-safe** en FreeRTOS
- ✅ **Usar variables globales** para debugging en threads
- ✅ **Mutex + UART directo** si printf es necesario en threads

### 3. **WWDG y Timing**
- **WWDG timeout** generalmente indica problemas de timing
- **Verificar siempre la configuración de timebase** en proyectos FreeRTOS
- **SysTick conflicts** son una causa común de watchdog resets

## 📁 Archivos Modificados

```
CM4/Core/Inc/FreeRTOSConfig.h              - Habilitado xPortSysTickHandler
CM4/Core/Src/threads/dispatcherTask.cpp    - Removido printf, agregado debugging variables
```

## ✅ Verificación de la Solución

1. **Sistema inicia correctamente** sin WWDG timeout
2. **dispatcher_status = 1** (running)
3. **dispatcher_iterations** se incrementa constantemente
4. **dispatcher_messages_received** se incrementa al recibir mensajes
5. **No más interrupciones WWDG_IRQHandler**
6. **FreeRTOS scheduler funciona estable**
7. **Printf funciona correctamente en threads** ✅

## 🎯 Lección Clave Aprendida

**El problema NO era printf en sí mismo**, sino la **configuración incorrecta del SysTick**:

```
ANTES (problemático): 
- HAL usa TIM2 ✅
- FreeRTOS SysTick deshabilitado ❌
- Printf expone el problema de timing ⚠️

DESPUÉS (correcto):
- HAL usa TIM2 ✅  
- FreeRTOS controla SysTick ✅
- Printf funciona perfectamente ✅
```

**Moraleja**: Siempre verificar la configuración de timebase al diagnosticar problemas de timing en FreeRTOS.

## 📚 Referencias

- [STM32 FreeRTOS Configuration Guide](https://www.st.com/resource/en/application_note/dm00105262-stm32cube-freertos-adaptation-stmicroelectronics.pdf)
- [FreeRTOS SysTick Configuration](https://www.freertos.org/RTOS-Cortex-M3-M4.html)
- [STM32WL Reference Manual - WWDG Section](https://www.st.com/resource/en/reference_manual/rm0461-stm32wl5x-advanced-armbased-32bit-mcus-with-subghz-radio-solution-stmicroelectronics.pdf)

---
**Fecha**: Noviembre 4, 2025  
**Proyecto**: TPP-IntelliFence  
**MCU**: STM32WL55JC (Dual Core)  
**RTOS**: FreeRTOS v10.2.1