# Fix: Startup Files para Dual-Core STM32WL55

**Fecha:** 25 de Enero de 2026  
**Estado:** ✅ Resuelto

## Problema

El proyecto no compilaba debido a startup files desactualizados que no soportaban correctamente la arquitectura dual-core del STM32WL55.

## Síntomas

- Error de compilación en build
- Startup files básicos sin soporte para memoria compartida entre cores
- Falta de inicialización correcta de segmentos MB_MEM (Mailbox Memory)

## Análisis

Los archivos `startup_stm32wl55xx_cm0plus.s` y `startup_stm32wl55xx_cm4.s` eran versiones simplificadas que:

1. **No inicializaban segmentos de memoria compartida:**
   - CM0+: Faltaban `MB_MEM2` y `MB_MEM3`
   - CM4: Faltaban `MB_MEM1` y `MAPPING_TABLE`

2. **Usaban código inline en lugar de macros:**
   - Duplicación de código para inicialización de `.data` y `.bss`
   - Menos mantenible y más propenso a errores

3. **Orden incorrecto de inicialización:**
   - Llamaban a `SystemInit` antes de inicializar memoria
   - Potencialmente peligroso para variables estáticas

## Solución

### 1. Startup CM0+ (`startup_stm32wl55xx_cm0plus.s`)

**Cambios aplicados:**

- ✅ Agregados símbolos para `MB_MEM2` y `MB_MEM3`:
  ```asm
  .word _siMB_MEM2
  .word _sMB_MEM2
  .word _eMB_MEM2
  .word _sMB_MEM3
  .word _eMB_MEM3
  ```

- ✅ Creadas macros `INIT_BSS` e `INIT_DATA`:
  ```asm
  .macro INIT_BSS start, end
    ldr r0, =\start
    ldr r1, =\end
    movs r3, #0
    bl LoopFillZerobss
  .endm
  ```

- ✅ Nueva sección `.text.data_initializers` con funciones helper separadas

- ✅ Orden correcto en `Reset_Handler`:
  1. Set stack pointer
  2. **Zero fill BSS** (incluyendo MB_MEM2 y MB_MEM3)
  3. **Copy DATA** (incluyendo MB_MEM2 inicializado)
  4. SystemInit
  5. __libc_init_array
  6. main

### 2. Startup CM4 (`startup_stm32wl55xx_cm4.s`)

**Cambios aplicados:**

- ✅ Agregados símbolos para `MB_MEM1` y `MAPPING_TABLE`:
  ```asm
  .word _sMB_MEM1
  .word _eMB_MEM1
  .word _sMAPPING_TABLE
  .word _eMAPPING_TABLE
  ```

- ✅ Misma estructura de macros e inicialización que CM0+

- ✅ Orden idéntico en `Reset_Handler` para consistencia

### 3. Linker Script CM0+ (`STM32WL55XX_FLASH_CM0PLUS.ld`)

**Ajustes de stack:**

```ld
_Min_Heap_Size = 0x400;   /* 1KB (era 0x200 - 512B) */
_Min_Stack_Size = 0x1000; /* 4KB (era 0x400 - 1KB) */
```

### 4. Cleanup en `main.c` (CM0+)

- ✅ Removida función de debug `wait_debugger_cm0p()` que bloqueaba el arranque

## Archivos Modificados

```
CM0PLUS/Core/Startup/startup_stm32wl55xx_cm0plus.s  (+49 líneas, arquitectura completa)
CM4/Core/Startup/startup_stm32wl55xx_cm4.s          (+46 líneas, arquitectura completa)
CM0PLUS/STM32WL55XX_FLASH_CM0PLUS.ld                (aumentado stack/heap)
CM0PLUS/Core/Src/main.c                             (removido wait_debugger)
```

## Archivos de Referencia (duplicados, eliminables)

Los archivos `startup_stm32wl55jcix.s` en ambos cores son copias idénticas a los modificados y pueden eliminarse.

## Verificación

✅ **Compilación exitosa:**
```bash
cube-cmake --build [...]/build/Debug --
# Exit Code: 0
```

✅ **Flash exitoso:** Ambos cores se programaron correctamente

✅ **Ejecución:** El sistema arranca sin errores

## Beneficios de la Solución

1. **Soporte completo dual-core:** Inicialización correcta de memoria compartida (IPCC)
2. **Código modular:** Macros reutilizables para BSS y DATA
3. **Orden seguro:** Memoria lista antes de cualquier código C
4. **Consistencia:** Ambos cores usan la misma arquitectura
5. **Mantenibilidad:** Código más claro y fácil de depurar

## Lecciones Aprendidas

- Los startup files de STM32CubeMX a veces están simplificados
- En dual-core, la inicialización de memoria compartida es **crítica**
- El orden de operaciones en `Reset_Handler` es importante
- Siempre comparar con ejemplos oficiales de ST para arquitecturas complejas

## Referencias

- STM32WL55 Reference Manual (RM0453)
- AN5406: How to build a LoRa® application with STM32CubeWL
- Ejemplos ST: `STM32Cube_FW_WL_V1.x.x/Projects/NUCLEO-WL55JC/Applications/LoRaWAN`
