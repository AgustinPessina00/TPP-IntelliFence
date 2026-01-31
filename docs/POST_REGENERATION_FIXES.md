# Post-Regeneration Compilation Fixes

**Fecha de Creación**: 28 de Enero de 2026  
**Autor**: Sistema de desarrollo TPP-IntelliFence  
**Propósito**: Guía de correcciones manuales necesarias después de regenerar código con STM32CubeMX

---

## 📋 Resumen Ejecutivo

Este documento detalla **todos los cambios manuales** que deben aplicarse después de regenerar el proyecto con STM32CubeMX para que el código compile correctamente. STM32CubeMX sobrescribe ciertos archivos críticos, requiriendo la reaplicación de estas correcciones.

---

## ✅ Checklist Post-Regeneración

Después de regenerar con CubeMX, aplicar en este orden:

- [ ] **1. CM4: mx-generated.cmake** - Configuración BSP (5 cambios)
- [ ] **2. CM0PLUS: mx-generated.cmake** - Configuración BSP (5 cambios)  
- [ ] **3. CM0PLUS: mx-generated.cmake** - Path mbed-crypto library
- [ ] **4. CM4: timer_if.c** - extern hrtc
- [ ] **5. CM0PLUS: timer_if.c** - Verificar extern hrtc (usualmente correcto)
- [ ] **6. CM4: platform.h** - Eliminar USE_BSP_DRIVER duplicado
- [ ] **7. CM0PLUS: platform.h** - Eliminar USE_BSP_DRIVER duplicado
- [ ] **8. CM4: FreeRTOSConfig.h** - xPortSysTickHandler (si no está en USER CODE)
- [ ] **9. Compilar y verificar** - No deben haber errores

---

## 🔧 Correcciones Detalladas

### 1. CM4: BSP Configuration en mx-generated.cmake

**Archivo**: `CM4/mx-generated.cmake`

**⚠️ CRÍTICO**: Estas 5 modificaciones deben aplicarse **cada vez** que se regenera el proyecto.

#### 1.1. Agregar USE_BSP_DRIVER a defines

**Ubicación**: En `set(MX_Defines_Syms` (líneas ~4-9)

```cmake
set(MX_Defines_Syms 
	CORE_CM4 
	USE_HAL_DRIVER 
	STM32WL55xx
	USE_BSP_DRIVER
    $<$<CONFIG:Debug>:DEBUG>
)
```

#### 1.2. Agregar BSP include path

**Ubicación**: En `set(MX_Include_Dirs` después de CMSIS Device (línea ~32)

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Device/ST/STM32WLxx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/LoRaWAN/Mac/Region
```

#### 1.3. Crear set de archivos BSP

**Ubicación**: Después de `set(FreeRTOS_Src` (línea ~122)

```cmake
set(FreeRTOS_Src
    # ... archivos FreeRTOS ...
)
set(BSP_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo_radio.c
)
```

#### 1.4. Crear biblioteca BSP

**Ubicación**: Después de `add_library(FreeRTOS OBJECT)` (línea ~145)

```cmake
# Create FreeRTOS static library
add_library(FreeRTOS OBJECT)
target_sources(FreeRTOS PRIVATE ${FreeRTOS_Src})
target_link_libraries(FreeRTOS PUBLIC stm32cubemx)

# Create BSP static library
add_library(BSP OBJECT)
target_sources(BSP PRIVATE ${BSP_Src})
target_link_libraries(BSP PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
```

#### 1.5. Agregar BSP a librerías de linkeo

**Ubicación**: En `set(MX_LINK_LIBS` (línea ~135)

```cmake
set (MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    Utilities
	FreeRTOS
	BSP
)
```

---

### 2. CM0PLUS: BSP Configuration en mx-generated.cmake

**Archivo**: `CM0PLUS/mx-generated.cmake`

**Aplicar las mismas 5 modificaciones que en CM4**, adaptadas para CM0PLUS:

#### 2.1. Agregar USE_BSP_DRIVER a defines

```cmake
set(MX_Defines_Syms 
	CORE_CM0PLUS 
	USE_HAL_DRIVER 
	STM32WL55xx 
	KMS_ENABLED 
	MBEDTLS_CONFIG_FILE=<mbedtls_config.h>
	USE_BSP_DRIVER
    $<$<CONFIG:Debug>:DEBUG>
)
```

#### 2.2. Agregar BSP include path

**Ubicación**: Antes de `${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include`

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/CMSIS/Include
```

#### 2.3. Crear set de archivos BSP

**Ubicación**: Después de `set(SubGHz_Phy_Src`

```cmake
set(SubGHz_Phy_Src
    # ... archivos SubGHz_Phy ...
)
set(BSP_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo_radio.c
)
```

#### 2.4. Crear biblioteca BSP

**Ubicación**: Después de `add_library(SubGHz_Phy OBJECT)`

```cmake
# Create SubGHz_Phy static library
add_library(SubGHz_Phy OBJECT)
target_sources(SubGHz_Phy PRIVATE ${SubGHz_Phy_Src})
target_link_libraries(SubGHz_Phy PUBLIC stm32cubemx)

# Create BSP static library
add_library(BSP OBJECT)
target_sources(BSP PRIVATE ${BSP_Src})
target_link_libraries(BSP PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
```

#### 2.5. Agregar BSP a librerías de linkeo

```cmake
set (MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    KMS
	mbed-crypto
	Utilities
	LoRaWAN
	SubGHz_Phy
	BSP
)
```

---

### 3. CM0PLUS: mbed-crypto Library Include Path

**Archivo**: `CM0PLUS/mx-generated.cmake`

**Problema**: El archivo `ca_ecc_mbed.c` necesita incluir `pk_internal.h` del directorio `library` de mbed-crypto.

**Solución**: Agregar el include path faltante.

**Ubicación**: En `set(MX_Include_Dirs`, después de las otras rutas de mbed-crypto (línea ~43)

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/mbed-crypto/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/mbed-crypto/include/mbedtls
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/mbed-crypto/include/psa
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/mbed-crypto/library
    ${CMAKE_CURRENT_SOURCE_DIR}/../Middlewares/Third_Party/LoRaWAN/Crypto
```

**Error sin este cambio**:
```
pk_internal.h: No such file or directory
```

---

### 4. CM4: Corregir múltiple definición de hrtc

**Archivo**: `CM4/Core/Src/timer_if.c`

**Problema**: La variable `hrtc` está definida en `rtc.c` pero CubeMX puede generarla también en `timer_if.c`, causando error de múltiple definición en el linker.

**Ubicación**: Línea ~36, en la sección "External variables"

**ANTES** (generado por CubeMX):
```c
/* External variables ---------------------------------------------------------*/
/**
  * @brief RTC handle
  */
RTC_HandleTypeDef hrtc;
```

**DESPUÉS** (corregido):
```c
/* External variables ---------------------------------------------------------*/
/**
  * @brief RTC handle
  */
extern RTC_HandleTypeDef hrtc;
```

**⚠️ IMPORTANTE**: Esta corrección **NO** está en sección USER CODE, por lo que debe reaplicarse manualmente después de cada regeneración.

---

### 5. CM0PLUS: Verificar extern hrtc

**Archivo**: `CM0PLUS/Core/Src/timer_if.c`

**Verificar**: Que la declaración sea `extern` (usualmente CubeMX lo genera correctamente para CM0PLUS):

```c
extern RTC_HandleTypeDef hrtc;
```

Si aparece sin `extern`, corregir como en el paso 4.

---

### 6. CM4: Eliminar define duplicado en platform.h

**Archivo**: `CM4/Core/Inc/platform.h`

**Problema**: `USE_BSP_DRIVER` se define tanto en CMake (paso 1.1) como en platform.h, generando warning de redefinición.

**Ubicación**: Línea ~31

**ANTES**:
```c
/* Exported constants --------------------------------------------------------*/

#define USE_BSP_DRIVER
/* USER CODE BEGIN EC */
```

**DESPUÉS**:
```c
/* Exported constants --------------------------------------------------------*/

/* USER CODE BEGIN EC */
```

**Nota**: Simplemente eliminar la línea `#define USE_BSP_DRIVER`.

---

### 7. CM0PLUS: Eliminar define duplicado en platform.h

**Archivo**: `CM0PLUS/Core/Inc/platform.h`

**Aplicar la misma corrección que en el paso 6**:

Eliminar `#define USE_BSP_DRIVER` en línea ~31.

---

### 8. CM4: FreeRTOSConfig.h - xPortSysTickHandler

**Archivo**: `CM4/Core/Inc/FreeRTOSConfig.h`

**Problema**: FreeRTOS necesita controlar SysTick cuando HAL usa timebase alternativo (TIM2).

**Ubicación**: En la sección `USER CODE BEGIN Defines` (línea ~180)

**Agregar**:
```c
/* USER CODE BEGIN Defines */
/* Section where parameter definitions can be added (for instance, to override default ones in FreeRTOS.h) */

/* IMPORTANT: This define MUST be uncommented when HAL uses alternate timebase (TIM2)
              to allow FreeRTOS to control SysTick directly. This prevents timing conflicts
              that cause WWDG_IRQHandler exceptions during UART operations. */
#define xPortSysTickHandler SysTick_Handler

/* Configuraciones para RTOS Views debugging */
#define configRECORD_STACK_HIGH_ADDRESS          1

/* USER CODE END Defines */
```

**✅ PROTEGIDO**: Esta sección está protegida por `USER CODE`, por lo que sobrevive regeneraciones **SI** se agregó correctamente en USER CODE.

**⚠️ ADVERTENCIA**: NO duplicar las definiciones de `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS` y `portGET_RUN_TIME_COUNTER_VALUE` aquí. Esas deben estar **solo** en la sección `USER CODE END 2` (líneas ~169-171).

---

## 🔍 Errores Comunes y Soluciones

### Error: `stm32wlxx_nucleo_radio.h: No such file or directory`

**Causa**: Falta configuración BSP en CMake.

**Solución**: Aplicar pasos 1 y 2 (BSP Configuration).

---

### Error: `multiple definition of 'hrtc'`

**Causa**: `timer_if.c` define `hrtc` en lugar de declararlo como `extern`.

**Solución**: Aplicar paso 4 (cambiar a `extern`).

---

### Error: `pk_internal.h: No such file or directory`

**Causa**: Falta include path de mbed-crypto library.

**Solución**: Aplicar paso 3 (agregar library path).

---

### Warning: `"USE_BSP_DRIVER" redefined`

**Causa**: Define duplicado en platform.h y CMake.

**Solución**: Aplicar pasos 6 y 7 (eliminar de platform.h).

---

### Error: `WWDG_IRQHandler exception` durante operaciones UART

**Causa**: SysTick no configurado para FreeRTOS.

**Solución**: Aplicar paso 8 (agregar xPortSysTickHandler).

**Referencia**: Ver `docs/02_implementation/04_freertos_wwdg.md`

---

## 📊 Tabla Resumen de Cambios

| Archivo | Ubicación | Cambio | ¿Protegido por USER CODE? |
|---------|-----------|--------|---------------------------|
| CM4/mx-generated.cmake | MX_Defines_Syms | Agregar USE_BSP_DRIVER | ❌ NO |
| CM4/mx-generated.cmake | MX_Include_Dirs | Agregar BSP include | ❌ NO |
| CM4/mx-generated.cmake | Después FreeRTOS_Src | Crear BSP_Src set | ❌ NO |
| CM4/mx-generated.cmake | Después add_library(FreeRTOS) | Crear BSP library | ❌ NO |
| CM4/mx-generated.cmake | MX_LINK_LIBS | Agregar BSP | ❌ NO |
| CM0PLUS/mx-generated.cmake | MX_Defines_Syms | Agregar USE_BSP_DRIVER | ❌ NO |
| CM0PLUS/mx-generated.cmake | MX_Include_Dirs | Agregar BSP include | ❌ NO |
| CM0PLUS/mx-generated.cmake | MX_Include_Dirs | Agregar mbed-crypto/library | ❌ NO |
| CM0PLUS/mx-generated.cmake | Después SubGHz_Phy_Src | Crear BSP_Src set | ❌ NO |
| CM0PLUS/mx-generated.cmake | Después add_library(SubGHz_Phy) | Crear BSP library | ❌ NO |
| CM0PLUS/mx-generated.cmake | MX_LINK_LIBS | Agregar BSP | ❌ NO |
| CM4/Core/Src/timer_if.c | ~línea 36 | `hrtc` → `extern hrtc` | ❌ NO |
| CM0PLUS/Core/Src/timer_if.c | ~línea 37 | Verificar `extern hrtc` | ❌ NO |
| CM4/Core/Inc/platform.h | ~línea 31 | Eliminar USE_BSP_DRIVER | ❌ NO |
| CM0PLUS/Core/Inc/platform.h | ~línea 31 | Eliminar USE_BSP_DRIVER | ❌ NO |
| CM4/Core/Inc/FreeRTOSConfig.h | USER CODE Defines | Agregar xPortSysTickHandler | ✅ SÍ |

---

## 🚀 Procedimiento de Regeneración Completo

### Paso 1: Antes de Regenerar

```bash
# 1. Hacer commit de cambios actuales
git add .
git commit -m "Pre-regeneration backup"

# 2. Hacer backup de archivos críticos (opcional pero recomendado)
cp CM4/mx-generated.cmake CM4/mx-generated.cmake.backup
cp CM0PLUS/mx-generated.cmake CM0PLUS/mx-generated.cmake.backup
```

### Paso 2: Regenerar con STM32CubeMX

1. Abrir `TPP-IntelliFence.ioc` en STM32CubeMX
2. Realizar cambios necesarios
3. Generar código (Project → Generate Code)

### Paso 3: Aplicar Correcciones

```bash
# Seguir checklist de este documento (pasos 1-8)
```

### Paso 4: Verificar Compilación

```bash
# Limpiar y reconstruir
cd C:\Users\ezero\Documents\TPP\TPP-IntelliFence
Remove-Item -Recurse -Force .\build\Debug\CM0PLUS,.\build\Debug\CM4 -ErrorAction SilentlyContinue

# Reconfigurar
cube-cmake --preset Debug

# Compilar
cube-cmake --build build/Debug --
```

### Paso 5: Validar

- ✅ No errores de compilación
- ✅ No warnings de redefinición de USE_BSP_DRIVER
- ✅ Binarios generados: `TPP-Intellifence_CM4.elf` y `TPP-Intellifence_CM0PLUS.elf`
- ✅ Tamaños de memoria aceptables:
  - CM4 RAM: ~28KB / 32KB
  - CM4 FLASH: ~64KB / 124KB
  - CM0PLUS RAM: ~8KB / 20KB
  - CM0PLUS FLASH: ~66KB / 120KB

---

## 📚 Referencias Relacionadas

- `docs/02_implementation/07_post_cubemx_regeneration_fixes.md` - Documento original de ST
- `docs/STARTUP_FILES_FIX.md` - Correcciones de startup files
- `docs/02_implementation/06_system_setup_after_codegen.md` - Configuración general post-codegen
- `docs/COMPILATION_FIXES.md` - Historial de problemas de compilación

---

## 📝 Notas Adicionales

### Archivos que NO necesitan modificación post-regeneración

Estos archivos están protegidos por secciones USER CODE o no son sobrescritos:

- `CM4/Core/Src/main.c` - Runtime stats functions (USER CODE 0)
- `CM4/Core/Inc/FreeRTOSConfig.h` - xPortSysTickHandler (USER CODE Defines)
- Archivos custom del proyecto (GPS, IMU, FSM, etc.)

### Versiones de Herramientas

- **STM32CubeMX**: 6.x
- **STM32WL SDK**: v1.x.x
- **LoRaWAN Middleware**: v1.0.4
- **GCC ARM**: 13.3.1+st.9
- **CMake**: 3.22+

### Orden Recomendado de Aplicación

1. **Primero**: Cambios en `mx-generated.cmake` (pasos 1, 2, 3)
2. **Segundo**: Cambios en archivos fuente (pasos 4, 5)
3. **Tercero**: Cambios en headers (pasos 6, 7, 8)
4. **Finalmente**: Compilar y verificar

---

**Última actualización**: 28 de Enero de 2026  
**Versión del documento**: 1.0  
**Estado**: ✅ Verificado y funcional
