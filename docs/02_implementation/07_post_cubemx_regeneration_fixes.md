# Post CubeMX Regeneration - Manual Fixes

**Última actualización**: Enero 2026  
**Autor**: Sistema de compilación TPP-IntelliFence

## Descripción General

Este documento detalla las correcciones manuales necesarias después de regenerar código con STM32CubeMX. La regeneración sobrescribe ciertos archivos, requiriendo que algunas modificaciones se reapliquen manualmente.

## Contexto

Cuando se regenera el proyecto desde el archivo `.ioc` con STM32CubeMX, se sobrescriben:
- Archivos de configuración HAL/LL
- Archivos mx-generated.cmake
- Archivos platform.h (excepto secciones USER CODE)
- Archivos main.c (excepto secciones USER CODE)

## Correcciones Requeridas

### 1. CM0PLUS: Eliminar código LED inexistente

**Archivo**: `CM0PLUS/Core/Src/main.c`

**Problema**: CubeMX puede generar código para `LED_RED_GPIO_Port`/`LED_RED_Pin` que no están configurados en el proyecto.

**Solución**: En el `while(1)` del main loop, eliminar cualquier toggle de LED:

```c
// ELIMINAR estas líneas si existen:
// HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
// HAL_Delay(500);
```

**Ubicación**: Dentro del loop principal entre `USER CODE BEGIN WHILE` y `USER CODE END WHILE`.

---

### 2. CM4: Configurar CMSIS Device Header para FreeRTOS

**Archivo**: `CM4/Core/Inc/FreeRTOSConfig.h`

**Problema**: El wrapper CMSIS-RTOS v2 requiere definir `CMSIS_device_header` pero CubeMX no lo genera automáticamente.

**Solución**: Agregar el define en la sección USER CODE:

```c
/* USER CODE BEGIN Includes */
#define CMSIS_device_header "stm32wlxx.h"
/* USER CODE END Includes */
```

**Línea**: Aproximadamente línea 47, dentro del bloque `USER CODE BEGIN Includes`.

**Nota**: Esta sección está protegida por USER CODE, por lo que sobrevive regeneraciones.

---

### 3. CM4 y CM0PLUS: Corregir múltiple definición de hrtc

**Archivo CM4**: `CM4/Core/Src/timer_if.c`  
**Archivo CM0PLUS**: `CM0PLUS/Core/Src/timer_if.c`

**Problema**: La variable `hrtc` está definida en `rtc.c` pero CubeMX puede generarla también en `timer_if.c`, causando error de múltiple definición en el linker.

**Solución**: Cambiar de definición a declaración extern:

```c
// ANTES (puede generarse así):
// RTC_HandleTypeDef hrtc;

// DESPUÉS:
extern RTC_HandleTypeDef hrtc;
```

**Ubicación**: Línea ~36, justo después de los includes.

**⚠️ IMPORTANTE**: Esta corrección NO está en sección USER CODE, por lo que debe reaplicarse manualmente después de cada regeneración.

---

### 4. CM0PLUS y CM4: Configurar soporte BSP en CMake

**Archivos afectados**:
- `CM0PLUS/mx-generated.cmake`
- `CM4/mx-generated.cmake`

**Problema**: STM32CubeMX no genera automáticamente la configuración BSP en los archivos CMake, aunque se haya seleccionado BSP en el .ioc.

**Solución**: Agregar 5 secciones en cada archivo `mx-generated.cmake`:

#### 4.1. Agregar define USE_BSP_DRIVER

En la lista `MX_Defines_Syms`, agregar:

```cmake
set(MX_Defines_Syms
    # ... otros defines ...
    USE_BSP_DRIVER
)
```

#### 4.2. Agregar include path BSP

En la lista `MX_Include_Dirs`, después de los includes de CMSIS, agregar:

```cmake
set(MX_Include_Dirs
    # ... otros includes ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo
)
```

#### 4.3. Crear set de archivos BSP

Después de las otras definiciones de sets (HAL_Src, LoRaWAN_Src, etc.), agregar:

```cmake
set(BSP_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo_radio.c
)
```

#### 4.4. Crear biblioteca BSP

Después de las otras definiciones de librerías (add_library), agregar:

```cmake
add_library(BSP OBJECT)
target_sources(BSP PRIVATE ${BSP_Src})
target_link_libraries(BSP PUBLIC stm32cubemx)
```

#### 4.5. Agregar BSP a librerías de linkeo

En la lista `MX_LINK_LIBS`, agregar:

```cmake
set(MX_LINK_LIBS
    # ... otras librerías ...
    BSP
)
```

**⚠️ CRÍTICO**: Estos archivos se sobrescriben completamente en cada regeneración. Guardar estas secciones para copiar/pegar rápidamente.

---

### 5. CM0PLUS y CM4: Eliminar definición duplicada en platform.h

**Archivos**: 
- `CM0PLUS/Core/Inc/platform.h`
- `CM4/Core/Inc/platform.h`

**Problema**: Si `USE_BSP_DRIVER` se define tanto en CMake como en platform.h, genera warnings de redefinición.

**Solución**: Eliminar el `#define USE_BSP_DRIVER` de platform.h (línea ~31):

```c
/* Exported constants --------------------------------------------------------*/

// ELIMINAR ESTA LÍNEA:
// #define USE_BSP_DRIVER

/* USER CODE BEGIN EC */
```

**Nota**: Esta sección NO está protegida por USER CODE, pero la eliminación solo es necesaria si se agregó previamente el define en CMake.

---

## Checklist de Verificación Post-Regeneración

Después de regenerar con CubeMX, verificar y aplicar en este orden:

- [ ] **CM0PLUS main.c**: Eliminar código LED si existe
- [ ] **CM4 FreeRTOSConfig.h**: Verificar `CMSIS_device_header` en USER CODE
- [ ] **CM4 timer_if.c**: Cambiar `hrtc` a `extern`
- [ ] **CM0PLUS timer_if.c**: Cambiar `hrtc` a `extern`
- [ ] **CM0PLUS mx-generated.cmake**: Agregar 5 secciones BSP (define, include, set, library, link)
- [ ] **CM4 mx-generated.cmake**: Agregar 5 secciones BSP (define, include, set, library, link)
- [ ] **CM0PLUS platform.h**: Eliminar `#define USE_BSP_DRIVER` duplicado
- [ ] **CM4 platform.h**: Eliminar `#define USE_BSP_DRIVER` duplicado
- [ ] **Compilación**: Ejecutar `cube-cmake --build build/Debug --`
- [ ] **Verificar warnings**: No deben aparecer warnings de redefinición

## Orden de Aplicación Recomendado

1. **Primero**: Correcciones protegidas por USER CODE (FreeRTOSConfig.h)
2. **Segundo**: Correcciones en archivos fuente (timer_if.c, main.c)
3. **Tercero**: Correcciones en archivos CMake (mx-generated.cmake)
4. **Cuarto**: Limpieza de defines duplicados (platform.h)
5. **Finalmente**: Compilar y verificar

## Errores Comunes al Olvidar Estas Correcciones

| Corrección Olvidada | Error de Compilación |
|---------------------|---------------------|
| LED code en CM0PLUS | `'LED_RED_GPIO_Port' undeclared` |
| CMSIS_device_header | `CMSIS_device_header must be defined` |
| extern hrtc | `multiple definition of 'hrtc'` |
| BSP en CMake | `stm32wlxx_nucleo_radio.h: No such file or directory` |
| Define duplicado | `warning: "USE_BSP_DRIVER" redefined` |

## Notas Adicionales

- **USER CODE sections**: Solo FreeRTOSConfig.h está protegido. Todo lo demás se sobrescribe.
- **Versión LoRaWAN**: Asegurarse de tener v1.0.4 seleccionado en el .ioc.
- **BSP Selection**: Verificar que "Nucleo WL55JC1" esté seleccionado como board en CubeMX.
- **Backup**: Considerar hacer commit git antes de regenerar para poder comparar cambios fácilmente.

## Referencias

- `COMPILATION_FIXES.md` - Historial de problemas de compilación resueltos
- `06_system_setup_after_codegen.md` - Setup general del sistema

---

**Versión del documento**: 1.0  
**Compatible con**: STM32CubeMX 6.x, STM32WL55xx, LoRaWAN 1.0.4
