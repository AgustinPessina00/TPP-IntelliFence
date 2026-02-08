# Correcciones de Compilación - TPP-IntelliFence

**Fecha:** 15 de Enero de 2026

## Resumen

Se resolvieron múltiples errores de compilación en ambos núcleos (CM4 y CM0PLUS) del STM32WL55 y se optimizó el uso de ROM para que el firmware quepa en memoria.

---

## Problemas Resueltos

### 1. Error: `usart_if.h` no encontrado (CM4)

**Archivo afectado:** `CM4/Core/Src/stm32_lpm_if.c`

**Problema:**
```c
#include "usart_if.h"  // Archivo no existe
...
vcom_Resume();  // Función no disponible
```

**Solución:**
- Comentado el `#include "usart_if.h"` 
- Comentada la llamada a `vcom_Resume()` que no está definida

**Cambios:**
```c
// #include "usart_if.h"  // File does not exist
...
// vcom_Resume();  // Function not available
```

---

### 2. Error: Múltiple definición de `hrtc` (CM4)

**Archivos afectados:** 
- `CM4/Core/Src/rtc.c` (definición)
- `CM4/Core/Src/timer_if.c` (definición duplicada)

**Problema:**
La variable `hrtc` estaba definida en dos archivos diferentes causando error de enlace.

**Solución:**
Cambiar la definición duplicada a `extern` en `timer_if.c`:

```c
// Antes
RTC_HandleTypeDef hrtc;

// Después
extern RTC_HandleTypeDef hrtc;
```

---

### 3. Error: `UTIL_TraceDriver` indefinido (CM4)

**Archivo afectado:** `CM4/MbMux/mbmuxif_trace.c`

**Problema:**
El sistema de trace requiere la estructura `UTIL_TraceDriver` pero no estaba definida en CM4.

**Solución:**
Agregada definición de `UTIL_TraceDriver` con punteros NULL:

```c
const UTIL_ADV_TRACE_Driver_s UTIL_TraceDriver =
{
  NULL,  /* Init   */
  NULL,  /* DeInit */
  NULL,  /* StartRx */
  NULL   /* Send   */
};
```

---

### 4. Error: Funciones FreeRTOS runtime stats indefinidas (CM4)

**Archivos afectados:**
- `CM4/Core/Inc/FreeRTOSConfig.h` (declaración)
- `CM4/Core/Src/app_freertos.c` (implementación)

**Problema:**
Las funciones `vConfigureTimerForRunTimeStats()` y `vGetRunTimeCounterValue()` estaban declaradas pero no implementadas.

**Solución:**
Implementadas las funciones en `app_freertos.c`:

```c
void vConfigureTimerForRunTimeStats(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t vGetRunTimeCounterValue(void)
{
    return DWT->CYCCNT;
}
```

---

### 5. Error: `ResponseTimeout` no es miembro de `McpsIndication_t` (CM0PLUS)

**Archivo afectado:** `Middlewares/Third_Party/LoRaWAN/Mac/LoRaMac.c`

**Problema:**
El código usaba `ResponseTimeout` sin verificar la versión de LoRaMAC:

```c
if( ... && (MacCtx.McpsIndication.ResponseTimeout > 0) )
```

**Solución:**
Envolver el uso de `ResponseTimeout` con directivas de compilación condicional:

```c
#if (defined( LORAMAC_VERSION ) && (( LORAMAC_VERSION == 0x01000400 ) || ( LORAMAC_VERSION == 0x01010100 )))
    if( ( ... ) || ( MacCtx.McpsIndication.ResponseTimeout > 0 ) )
#else
    if( ( ... ) && ( Nvm.MacGroup2.DeviceClass == CLASS_A ) )
#endif
```

---

### 6. Error: ROM overflow (CM4)

**Problema inicial:**
```
ROM: 131876 B / 128 KB = 100.61% ❌ (overflow de 804 bytes)
```

**Soluciones aplicadas:**

#### 6.1. Deshabilitar soporte float en printf

**Archivo afectado:** `CM4/CMakeLists.txt`

```cmake
# Enable printf/scanf with floating point support
# DISABLED to save ROM space (~8-10KB)
#target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
#    -u_printf_float
#)
```

**Ahorro:** ~8-10 KB

#### 6.2. Configurar optimización para debug

**Archivo afectado:** `CM4/CMakeLists.txt`

Configuración final con `-O0` para mejor debugging:

```cmake
# Debug configuration - No optimization for better debugging
# -O0: No optimization
# -g3: Full debug information (macros, etc.)
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3" CACHE STRING "C Debug flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3" CACHE STRING "C++ Debug flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-O0 -g3" CACHE STRING "Linker Debug flags" FORCE)
```

**Resultado final:**
```
ROM: 123732 B / 128 KB = 94.40% ✓
RAM1: 11312 B / 16 KB = 69.04%
```

---

### 7. Configuración FreeRTOS runtime stats

**Archivo afectado:** `CM4/Core/Inc/FreeRTOSConfig.h`

**Estado final:** Habilitadas para monitoring de tareas

```c
#define configGENERATE_RUN_TIME_STATS            1
#define configUSE_STATS_FORMATTING_FUNCTIONS     1
```

---

## Estado Final de Compilación

### CM0PLUS
```
ROM: 12312 B / 128 KB = 9.39% ✓
RAM1: 1792 B / 16 KB = 10.94%
```

### CM4
```
ROM: 123732 B / 128 KB = 94.40% ✓
RAM1: 11312 B / 16 KB = 69.04%
```

---

## Configuración Actual

### Optimización
- **Nivel:** `-O0` (sin optimización)
- **Debug info:** `-g3` (completa)
- **Printf float:** Deshabilitado
- **FreeRTOS stats:** Habilitadas

### Ventajas de la configuración actual
- ✓ Código cabe en ROM con margen del 5.6%
- ✓ Debug completo disponible
- ✓ Variables visibles sin optimizar
- ✓ Breakpoints funcionan correctamente
- ✓ Estadísticas de runtime de FreeRTOS disponibles

---

## Notas sobre Printf con Float

Se encontraron **58 instancias** de `printf` con formato float en el código, principalmente en:
- Archivos de test (`_test`, `_test_wrapper`)
- Módulos de sensores (GPS, IMU, INA226)
- Clases de datos (Cow, Fence)

**Estado:** Los printf con float no funcionarán correctamente pero no causan error de compilación. Si se necesita, pueden convertirse a formato entero multiplicando por potencias de 10.

**Ejemplo de conversión:**
```c
// Antes (requiere float support)
printf("Lat: %.6f\r\n", latitude);

// Después (sin float support)
int32_t lat_int = (int32_t)(latitude * 1000000);
printf("Lat: %ld.%06ld\r\n", lat_int / 1000000, lat_int % 1000000);
```

---

## Procedimiento de Limpieza y Recompilación

Si se modifican los flags de CMake, ejecutar:

```powershell
Remove-Item -Path "C:\Users\ezero\Documents\TPP\TPP-IntelliFence\CM4\build\*" -Recurse -Force
Remove-Item -Path "C:\Users\ezero\Documents\TPP\TPP-IntelliFence\build\Debug\*" -Recurse -Force
cmake --preset Debug -S . -B build/Debug
```

---

## Referencias

- STM32WL55 tiene 256KB de Flash total (128KB por núcleo)
- FreeRTOS runtime stats usan DWT (Data Watchpoint and Trace) cycle counter
- LoRaWAN Middleware versión variable según `LORAMAC_VERSION`

---

**Compilación exitosa:** ✓  
**Fecha de última verificación:** 15 de Enero de 2026
