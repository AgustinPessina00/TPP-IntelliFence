# 🐛 Guía de Debug en Modo TEST

## 📋 Nueva Configuración de Debug

Se ha agregado una configuración específica para debuggear el firmware en **modo TEST** con datos mock.

## 🚀 Cómo Usar

### Opción 1: Desde el Panel de Debug (Recomendado)

1. Presiona `Ctrl+Shift+D` o ve al panel de Debug
2. En el dropdown de configuraciones, selecciona: **`CM4_Debug_TEST_MODE`**
3. Presiona `F5` o click en el botón ▶️ verde

**Esto automáticamente:**
- ✅ Configura CMake con `-DENABLE_TEST_MODE=ON`
- ✅ Compila el firmware CM4 con datos mock
- ✅ Flashea el `.elf` al microcontrolador
- ✅ Inicia sesión de debug
- ✅ Muestra instrucciones en el terminal

### Opción 2: Usando Tareas Manualmente

```bash
# 1. Configurar modo TEST
Ctrl+Shift+P > Tasks: Run Task > Configure CMake TEST MODE

# 2. Compilar
Ctrl+Shift+P > Tasks: Run Task > Build CM4 TEST MODE

# 3. Flashear
Ctrl+Shift+P > Tasks: Run Task > Flash CM4 Only

# 4. Debuggear
F5 con configuración CM4_Debug_TEST_MODE
```

## 🎯 Configuraciones Disponibles

| Configuración | Descripción | Modo | preLaunchTask |
|--------------|-------------|------|---------------|
| **CM4_Debug** | Debug normal (hardware real) | Producción | Build All (CM0PLUS + CM4) |
| **CM4_Debug_TEST_MODE** | Debug con mock sensors | TEST | Build All TEST MODE |

## 🔧 Tareas Disponibles

### Tareas de Compilación

- **Build All (CM0PLUS + CM4)** - Compilación normal (producción)
- **Build All TEST MODE** - Compilación con `-DENABLE_TEST_MODE=ON`
  - Subtarea: Configure CMake TEST MODE
  - Subtarea: Build CM4 TEST MODE

### Tareas de Flash

- **Flash CM0PLUS and CM4** - Flash ambos cores
- **Flash CM4 Only** - Flash solo CM4 (útil para test)
- **Build and Flash Both Cores** - Compilar y flashear todo

### Tareas de Ayuda

- **Show Test Instructions** - Muestra info sobre el modo test

## 🐛 Puntos de Interrupción Útiles

Para debuggear el sistema de test, coloca breakpoints en:

### Inicialización del Test
```cpp
// app_freertos.c - Línea ~245
TestData_Init();
TestMode_Enable();
```

### Mock de Sensores
```cpp
// test_sensor_mock.cpp - Línea ~51
case MSG_ID_REQUEST_GPS: {
    const TestGPSData_t* gpsData = TestData_GetNextGPS();
    
// test_sensor_mock.cpp - Línea ~87
case MSG_ID_REQUEST_IMU: {
    const TestIMUData_t* imuData = TestData_GetNextIMU();
```

### FSM States
```cpp
// fsmTask.cpp - Línea ~34
void fsmTask(void *argument) {

// fsmTask.cpp - Línea ~85
void runStartupRoutineFSM(...)

// fsmTask.cpp - Línea ~215
void runNormalOperationFSM(...)
```

### Datos de Test
```cpp
// test_data.cpp - Línea ~17
const TestGPSData_t TEST_GPS_DATA[TEST_GPS_DATA_COUNT]

// test_data.cpp - Línea ~84
const TestIMUData_t TEST_IMU_DATA[TEST_IMU_DATA_COUNT]
```

## 📊 Verificación en Debug

### Variables a Observar (Watch)

```cpp
// En sensorAcqTask_Test
gpsData->position.latitude
gpsData->position.longitude
gpsData->expectedZone
imuData->acceleration.ax
imuData->expectedState

// En fsmTask
s_mainFSM
s_normalOpFSM
s_startupRoutineState
cow.getCurrentZone()
fence.getCenter()

// Índices de test
TestData_GetGPSIndex()
TestData_GetIMUIndex()
```

### Call Stack Esperado

Durante el procesamiento de un mensaje GPS:
```
sensorAcqTask_Test()
  └─ TestData_GetNextGPS()
      └─ osMessageQueuePut(fsmQueueHandle, ...)
          └─ fsmTask() [en otra thread]
              └─ runStartupRoutineFSM() o runNormalOperationFSM()
```

## 🎨 Características del Debug TEST MODE

### SWO/ITM Output
La configuración incluye SWO (Serial Wire Output) habilitado para capturar `rtos_printf()`:
- CPU Frequency: 48 MHz
- SWO Frequency: 2 MHz
- Puerto ITM 0 para console output

### Semihosting
Semihosting habilitado para `printf()` redirection a debug console.

## ⚡ Tips de Debug

### 1. Ver Logs en Tiempo Real
- Los `rtos_printf()` aparecen en la consola de debug
- Buscar logs con prefijo `[TEST_GPS]`, `[TEST_IMU]`, `[FSM]`

### 2. Step Through Test Data
```cpp
// Coloca breakpoint aquí y usa F10 (Step Over)
const TestGPSData_t* gpsData = TestData_GetNextGPS();
// Inspecciona gpsData en Watch para ver próxima muestra
```

### 3. Saltar Escenarios
```cpp
// En Immediate Window (Debug Console):
gpsIndex = 20;  // Saltar al escenario RED_ZONE
```

### 4. Verificar Sincronización GPS/IMU
```cpp
// En Watch:
TestData_GetGPSIndex()  // Debe estar cerca de
TestData_GetIMUIndex()  // este valor (diferencia < 2)
```

## 🔄 Cambiar Entre Modos

### De TEST a PRODUCCIÓN

1. Selecciona configuración: **CM4_Debug** (sin TEST_MODE)
2. Presiona `F5`
3. El `preLaunchTask` compilará sin `-DENABLE_TEST_MODE`

### De PRODUCCIÓN a TEST

1. Selecciona configuración: **CM4_Debug_TEST_MODE**
2. Presiona `F5`
3. El `preLaunchTask` compilará con `-DENABLE_TEST_MODE=ON`

## 🚨 Troubleshooting Debug

### Problema: "Cannot find ELF file"
**Solución**: Ejecutar manualmente `Build All TEST MODE` primero

### Problema: "No symbols loaded"
**Solución**: Verificar que `.elf` en `CM4/build/` sea reciente

### Problema: "SWO output not working"
**Solución**: 
1. Verificar conexión ST-Link
2. Check CPU frequency (48 MHz para STM32WL55)
3. Reiniciar debug session

### Problema: "Breakpoint not hit in test files"
**Solución**: Verificar que compilaste con `-DENABLE_TEST_MODE=ON`
```bash
# Verificar en terminal:
cat CM4/build/CMakeCache.txt | grep ENABLE_TEST_MODE
# Debe mostrar: ENABLE_TEST_MODE:BOOL=ON
```

## 📈 Workflow Típico de Debug en TEST

1. **Setup**
   - Seleccionar `CM4_Debug_TEST_MODE`
   - F5 para compilar y lanzar

2. **Observar Inicialización**
   - Breakpoint en `TestData_Init()`
   - Verificar que 30 muestras están cargadas

3. **Seguir Primer Mensaje GPS**
   - Breakpoint en `MSG_ID_REQUEST_GPS` case
   - F10 para ver `gpsData` extraída
   - F10 hasta `osMessageQueuePut()`

4. **Ver FSM Procesando**
   - Breakpoint en `fsmTask()` main loop
   - Ver `msgReceived` en Watch
   - Seguir hasta estado específico

5. **Verificar Transición de Zona**
   - Breakpoint en `runNormalOperationFSM()`
   - Cuando índice GPS > 8, debe entrar a STIMULUS_ZONE
   - Verificar que `cow.getCurrentZone() != GREEN_ZONE`

6. **Validar Escape (BLACK_ZONE)**
   - Continuar hasta GPS índice 23
   - Verificar zona = BLACK_ZONE
   - Verificar que FSM detecta escape

## 🎓 Recursos Adicionales

- [README_COMPILE.md](../../CM4/Core/Src/Test/README_COMPILE.md) - Guía de compilación manual
- [IMPLEMENTATION_SUMMARY.md](../../CM4/Core/Src/Test/IMPLEMENTATION_SUMMARY.md) - Resumen de implementación
- [README_TEST.md](../../CM4/Core/Src/Test/README_TEST.md) - Documentación de escenarios de test

---

**¡Happy Testing & Debugging! 🐛🔬**
