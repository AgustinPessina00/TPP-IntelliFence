# Pasos para solucionar "Failed to get RTOS information" en STM32CubeIDE VS Code

## Problema Detectado
```
RTOS Views: Session Name: "CM4_V1", FreeRTOS detected.
Unable to collect full RTOS information. Following info from last query may be stale.
RTOS Views: Failed to get RTOS information. Please report an issue if RTOS is actually running
```

## Solución Implementada

### 1. Configuración VS Code Settings
Se agregaron configuraciones para mejorar el debugging:
```json
{
    "debug.allowBreakpointsEverywhere": true,
    "debug.showInlineBreakpointCandidates": true
}
```

### 2. Configuración FreeRTOS Mínima
Se agregaron las definiciones necesarias en `FreeRTOSConfig.h`:
```c
#define configGENERATE_RUN_TIME_STATS            0
#define configUSE_STATS_FORMATTING_FUNCTIONS     0
```

### 3. Procedimiento de Debugging Correcto

#### Paso 1: Iniciar Debug Session
1. Compilar el proyecto completamente
2. Iniciar sesión de debug con "CM4_V1"
3. **NO acceder inmediatamente a RTOS Views**

#### Paso 2: Esperar que FreeRTOS Inicie Completamente
1. Colocar breakpoint DESPUÉS de `osKernelStart()` en main.c (línea ~470)
2. Ejecutar hasta el breakpoint
3. Verificar que las tareas estén ejecutándose

#### Paso 3: Acceder a RTOS Views
1. Una vez que FreeRTOS esté completamente iniciado
2. Ir a RTOS Views panel
3. Si muestra "Busy updating", usar comando: **RTOS Views: Refresh**

## Comandos Útiles de VS Code

### Para Acceso Manual a RTOS Views:
- `Ctrl+Shift+P` → "RTOS Views: Refresh"
- `Ctrl+Shift+P` → "RTOS Views: Toggle RTOS Panel"

### Verificación Manual de Tasks:
En el Debug Console, usar comandos GDB:
```
info threads
monitor rtos
```

## Tareas Definidas en el Proyecto
Las siguientes tareas deberían aparecer en RTOS Views:
- `dispatcher_Task` (dispatcherTask)
- `fsm_Task` (fsmTask) 
- `sensorAcq_Task` (sensorAcqTask)

## Troubleshooting Adicional

### Si Persiste el Error:
1. **Verificar que FreeRTOS está ejecutándose**: Las variables globales en `dispatcherTask.cpp` deberían mostrar actividad:
   - `dispatcher_status` = 1 (running)
   - `dispatcher_iterations` > 0

2. **Recompilar con símbolos completos**: Verificar que se compile con `-g` y sin optimizaciones altas

3. **Reiniciar completamente**: 
   - Cerrar VS Code
   - Limpiar build: `cd CM4/build && ninja clean`
   - Recompilar: `cd CM4/build && ninja`
   - Reiniciar debugging

### Configuraciones Críticas Ya Presentes:
✅ `configUSE_TRACE_FACILITY = 1`
✅ Multiple tasks definidas (dispatcher, fsm, sensorAcq)
✅ CMSIS-RTOS V2 wrapper habilitado
✅ `configQUEUE_REGISTRY_SIZE = 8`

## Timing Correcto
El error ocurre típicamente porque el debugger intenta acceder a RTOS Views antes de que:
1. FreeRTOS scheduler haya iniciado completamente
2. Las tareas estén creadas y ejecutándose
3. Los símbolos estén completamente cargados

**Solución**: Siempre esperar que `osKernelStart()` termine antes de acceder a RTOS Views.