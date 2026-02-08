# Sistema de Printf Thread-Safe para FreeRTOS

## Descripción General

Este módulo proporciona una implementación thread-safe de `printf` para aplicaciones FreeRTOS, utilizando un mutex para evitar que múltiples tareas interfieran entre sí al escribir en el puerto serial.

## Características

✅ **Thread-Safe**: Usa un mutex recursivo de FreeRTOS para proteger las operaciones de printf  
✅ **Sin Asignación Dinámica**: Buffer estático de tamaño configurable  
✅ **Formato Estándar**: Compatible con todos los especificadores de formato de printf  
✅ **Macros de Logging**: Niveles INFO, WARN, ERROR, DEBUG  
✅ **Fácil Migración**: Macro opcional para redirigir printf estándar  

## Archivos

- `CM4/Core/Inc/rtos_printf.h` - Interfaz pública
- `CM4/Core/Src/rtos_printf.c` - Implementación

## Uso Básico

### 1. Inicialización (en main.c)

```c
#include "rtos_printf.h"

int main(void) {
    // ... inicialización HAL ...
    
    // Inicializar el kernel de FreeRTOS
    osKernelInitialize();
    MX_FREERTOS_Init();
    
    // Inicializar el sistema de printf thread-safe
    if (rtos_printf_init() != 0) {
        printf("ERROR - Failed to initialize RTOS printf\r\n");
        Error_Handler();
    }
    
    // ... resto del código ...
    osKernelStart();
}
```

### 2. Uso en Tareas de FreeRTOS

```c
#include "rtos_printf.h"

void myTask(void *argument) {
    for (;;) {
        // Usar rtos_printf en lugar de printf
        rtos_printf("Task %s: counter = %d\r\n", pcTaskGetName(NULL), counter);
        
        osDelay(1000);
    }
}
```

### 3. Macros de Logging

```c
#include "rtos_printf.h"

void sensorTask(void *argument) {
    RTOS_LOG_INFO("Sensor task started\r\n");
    
    float temperature = readTemperature();
    RTOS_LOG_DEBUG("Temperature reading: %.2f°C\r\n", temperature);
    
    if (temperature > 50.0f) {
        RTOS_LOG_WARN("Temperature high: %.2f°C\r\n", temperature);
    }
    
    if (temperature > 80.0f) {
        RTOS_LOG_ERROR("Temperature critical: %.2f°C\r\n", temperature);
    }
}
```

## Redirección de printf (Opcional)

Si quieres que todas las llamadas a `printf` usen automáticamente la versión thread-safe:

### Opción 1: Define global en main.c

```c
#define RTOS_PRINTF_ENABLED
#include "rtos_printf.h"

// Ahora printf() automáticamente usa rtos_printf()
printf("This is thread-safe!\r\n");
```

### Opción 2: Define en CMakeLists.txt

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    RTOS_PRINTF_ENABLED
)
```

## Configuración

Editar `rtos_printf.h` para ajustar:

```c
// Tamaño máximo de un mensaje (default: 512 bytes)
#define RTOS_PRINTF_BUFFER_SIZE     512

// Timeout del mutex en milisegundos (default: 1000 ms)
#define RTOS_PRINTF_MUTEX_TIMEOUT   1000
```

## Salida Personalizada

Por defecto, `rtos_printf` usa la implementación estándar de `stdout` (típicamente UART retarget). Para redirigir a otro periférico:

```c
// En rtos_printf.c, modifica la función _rtos_printf_write():

int _rtos_printf_write(const char *str, size_t len)
{
    // Ejemplo: salida directa a UART2
    HAL_UART_Transmit(&huart2, (uint8_t*)str, len, HAL_MAX_DELAY);
    return len;
}
```

O define `RTOS_PRINTF_USE_UART` en el CMakeLists.txt y configura el handle UART deseado.

## Ventajas vs Printf Estándar

| Característica | printf estándar | rtos_printf |
|----------------|-----------------|-------------|
| Thread-Safe | ❌ No | ✅ Sí |
| Protección Mutex | ❌ No | ✅ Sí |
| Mensajes Intercalados | ⚠️ Posible | ✅ Evitado |
| Overhead | Bajo | Bajo (solo mutex) |
| Reentrante | ❌ No | ✅ Sí |

## Ejemplo de Problema sin rtos_printf

```c
// Tarea 1
printf("Sensor A: temp="); // ← interrumpida aquí
printf("25.3°C\r\n");

// Tarea 2 (interrumpe)
printf("Sensor B: humidity=45%\r\n");

// Salida mezclada:
// "Sensor A: temp=Sensor B: humidity=45%
// 25.3°C"
```

Con `rtos_printf`, cada mensaje se imprime completo sin interrupciones.

## Debugging

Si `rtos_printf` no produce salida:

1. Verificar que `rtos_printf_init()` fue llamado **después** de `osKernelInitialize()`
2. Verificar que el retarget de UART/ITM está configurado
3. Aumentar `RTOS_PRINTF_MUTEX_TIMEOUT` si hay timeouts
4. Verificar stack size de las tareas (mínimo 512 bytes recomendado)

## Performance

- **Overhead del mutex**: ~10-20 μs (típico en STM32)
- **Tamaño de código**: ~1 KB adicional
- **RAM estática**: 512 bytes (buffer) + 32 bytes (mutex)
- **No usa heap**: Todo estático, sin malloc/free

## Integración con Threads Existentes

Los siguientes archivos ya usan printf y pueden beneficiarse de `rtos_printf`:

- `CM4/Core/Src/threads/dispatcherTask.cpp`
- `CM4/Core/Src/threads/fsmTask_new.cpp`
- `CM4/Core/Src/threads/sensorAcqTask_new.cpp`

Simplemente incluye `rtos_printf.h` y reemplaza `printf` con `rtos_printf` o usa la macro de redirección.

## Notas Importantes

⚠️ **IMPORTANTE**: No usar `printf` estándar después de inicializar FreeRTOS sin el mutex, ya que puede causar corrupción de datos.

⚠️ **Llamadas desde ISR**: No usar `rtos_printf` desde ISRs. El mutex no es ISR-safe. Para logging desde ISRs, usar un buffer de ring dedicado.

✅ **Buena Práctica**: Inicializar `rtos_printf_init()` inmediatamente después de `osKernelInitialize()` para que esté disponible lo antes posible.

## Ejemplo Completo

Ver `CM4/Core/Src/main.c` para un ejemplo completo de integración con tests de módulos y arranque de FreeRTOS.
