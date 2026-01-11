# Sistema de Test para la Máquina de Estados (FSM)

## 📋 Descripción General

Este sistema de prueba proporciona datos estáticos simulados de GPS e IMU para validar el comportamiento de la máquina de estados sin necesidad de hardware real.

## 🏗️ Arquitectura

### Archivos del Sistema de Test

- **`test_data.h`**: Definiciones de estructuras y funciones de test
- **`test_data.cpp`**: Arrays estáticos con 30 posiciones GPS y 30 muestras IMU
- **`test_sensor_mock.cpp`**: Mock que reemplaza `sensorAcqTask` y responde con datos de prueba

## 📊 Escenarios de Prueba Incluidos

### 🟢 ESCENARIO 1: STARTUP ROUTINE
- **Objetivo**: Verificar la inicialización correcta del sistema
- **Posiciones**: Muestra 0
- **Estado**: Vaca en reposo en el centro del cerco
- **Zona esperada**: GREEN_ZONE
- **Estado esperado**: SLEEP

### 🟢 ESCENARIO 2: OPERACIÓN NORMAL - GREEN ZONE
- **Objetivo**: Validar comportamiento en zona segura
- **Posiciones**: Muestras 1-7
- **Estados cubiertos**:
  - GRAZING (pastando)
  - SLEEP (durmiendo)
  - MOVEMENT (movimiento dentro de zona verde)
- **Zona esperada**: GREEN_ZONE (distancia al límite: 4-22m)

### 🔵 ESCENARIO 3: APROXIMACIÓN AL LÍMITE - LIGHT_BLUE_ZONE
- **Objetivo**: Validar activación progresiva del buzzer
- **Posiciones**: Muestras 8-10
- **Estado**: MOVEMENT (vaca avanzando hacia el límite)
- **Zona esperada**: LIGHT_BLUE_ZONE (distancia: 35-48m)
- **Estímulo**: Buzzer leve (frecuencia baja, duty bajo)

### 🔵 ESCENARIO 4: BLUE_ZONE
- **Objetivo**: Verificar escalado del estímulo sonoro
- **Posiciones**: Muestras 11-13
- **Estado**: MOVEMENT
- **Zona esperada**: BLUE_ZONE (distancia: 55-68m)
- **Estímulo**: Buzzer medio (intensidad aumentada)

### 🔵 ESCENARIO 5: DARK_BLUE_ZONE
- **Objetivo**: Confirmar máximo nivel de advertencia sonora
- **Posiciones**: Muestras 14-16
- **Estado**: MOVEMENT (vaca agitada)
- **Zona esperada**: DARK_BLUE_ZONE (distancia: 75-88m)
- **Estímulo**: Buzzer intenso

### 🟡 ESCENARIO 6: YELLOW_ZONE (STIMULUS ZONE)
- **Objetivo**: Validar activación de buzzer + vibración
- **Posiciones**: Muestras 17-19
- **Estado**: MOVEMENT (movimiento intenso debido al estímulo)
- **Zona esperada**: YELLOW_ZONE (distancia: 95-108m)
- **Estímulo**: Buzzer + Vibración
- **FSM esperada**: Transición a `STIMULUS_ZONE` sub-FSM

### 🔴 ESCENARIO 7: RED_ZONE
- **Objetivo**: Confirmar máximo nivel de estímulo antes del escape
- **Posiciones**: Muestras 20-22
- **Estado**: MOVEMENT (muy agitada, resistiendo estímulo)
- **Zona esperada**: RED_ZONE (distancia: 115-128m)
- **Estímulo**: Vibración intensa (sin shock)

### ⚫ ESCENARIO 8: BLACK_ZONE (ESCAPE)
- **Objetivo**: Detectar escape y activar alerta crítica
- **Posiciones**: Muestras 23-24
- **Estado**: MOVEMENT (corriendo)
- **Zona esperada**: BLACK_ZONE (distancia: 138-145m)
- **Acción esperada**: Alerta al sistema central vía LoRa

### ↩️ ESCENARIO 9: FENCE TRANSITION (REGRESO)
- **Objetivo**: Validar la FSM de transición cuando la vaca regresa
- **Posiciones**: Muestras 25-29
- **Estados**: MOVEMENT → GRAZING → SLEEP
- **Zonas**: RED → YELLOW → DARK_BLUE → GREEN
- **FSM esperada**: `FENCE_TRANSITION` hasta que vuelva a GREEN_ZONE

## 📐 Geometría del Cerco de Prueba

### Configuración
- **Forma**: Cuadrado
- **Dimensiones**: ~222m x 222m (equivalente a radio de ~100m)
- **Centro**: (-34.9205°, -57.9536°) - Coordenadas cerca de La Plata, Argentina
- **Vértices**:
  ```cpp
  NO: {-34.919500, -57.954600}
  NE: {-34.919500, -57.952600}
  SE: {-34.921500, -57.952600}
  SO: {-34.921500, -57.954600}
  ```

### Umbrales de Zona (desde el límite del polígono)
- **GREEN**: 0-30m desde el centro
- **LIGHT_BLUE**: 30-50m
- **BLUE**: 50-70m
- **DARK_BLUE**: 70-90m
- **YELLOW**: 90-110m
- **RED**: 110-130m
- **BLACK**: >130m (fuera del cerco)

## 🎯 Datos de Acelerómetro (IMU)

### Valores Característicos por Estado

#### SLEEP (Durmiendo)
- **Aceleración**: ~0.01-0.02 m/s² en XY, ~9.81 m/s² en Z
- **Descripción**: Movimiento mínimo, solo respiración
- **Muestras**: 0, 5-7

#### GRAZING (Pastando)
- **Aceleración**: ~0.12-0.20 m/s² en XY, ~9.78-9.85 m/s² en Z
- **Descripción**: Movimientos suaves de cabeza al comer
- **Muestras**: 1-4, 28

#### MOVEMENT (Movimiento)
- **Lento**: 0.45-0.55 m/s² (muestras 8-10)
- **Normal**: 0.65-0.75 m/s² (muestras 11-13)
- **Rápido**: 0.85-1.0 m/s² (muestras 14-16)
- **Intenso**: 1.1-1.6 m/s² (muestras 17-22)
- **Corriendo**: 1.8-2.0 m/s² (muestras 23-24, escape)

## 🔧 Uso del Sistema de Test

### 1. Inicialización
```cpp
#include "Test/test_data.h"

// Al inicio de tu sistema
TestData_Init();
```

### 2. Integración con FreeRTOS
Reemplazar la tarea `sensorAcqTask` original con:
```cpp
void sensorAcqTask_Test(void *argument);  // Definida en test_sensor_mock.cpp
```

### 3. Control del Modo Test
```cpp
// Habilitar modo test
TestMode_Enable();

// Deshabilitar y volver a producción
TestMode_Disable();

// Resetear índices para repetir test
TestMode_Reset();

// Verificar si quedan datos
if (TestData_HasMoreData()) {
    // Continuar test
}
```

### 4. Monitoreo
El sistema imprime logs con emojis para facilitar el seguimiento:
```
🧪 [GPS 08] Lat: -34.920520, Lon: -57.953850 | LIGHT_BLUE: Buzzer leve → LIGHT_BLUE
🧪 [IMU 08] ax: 0.45, ay: 0.38, az: 10.20 | MOVEMENT: Caminando lento → MOVEMENT
🧪 [ZONE] Zona: LIGHT_BLUE, Distancia: 35.0m
```

## ✅ Verificación de Resultados

### Transiciones Esperadas en la FSM Principal
1. **STARTUP_ROUTINE** → Muestra 0
2. **NORMAL_OPERATION.INITIALIZE** → Muestras 1-7
3. **NORMAL_OPERATION.GREEN_ZONE** → Muestras 1-7
4. **NORMAL_OPERATION.STIMULUS_ZONE** → Muestras 17-22 (YELLOW/RED)
5. **FENCE_TRANSITION** → Muestras 25-29 (regreso)

### Comportamientos Específicos a Validar

#### En GREEN_ZONE
- ✓ GPS rate bajo (poder saving)
- ✓ Sin estímulo activo
- ✓ Diferentes tiempos de sleep según CowState

#### En STIMULUS_ZONE
- ✓ Envío de zona actual por LoRa
- ✓ Estímulo progresivo según zona
- ✓ GPS rate alto para tracking preciso

#### En FENCE_TRANSITION
- ✓ Desactivación de estímulo
- ✓ Actualización de cerca si es necesario
- ✓ GPS rate rápido hasta confirmar zona
- ✓ Regreso a NORMAL_OPERATION cuando vuelve a GREEN

## 🐛 Troubleshooting

### Problema: FSM no avanza
- Verificar que `sensorAcqQueueHandle` esté creada
- Confirmar que `sensorAcqTask_Test` está corriendo
- Revisar timeouts en las esperas de mensajes

### Problema: Datos incorrectos
- Llamar `TestData_Reset()` para reiniciar índices
- Verificar que `TestData_Init()` se llamó al inicio

### Problema: No hay logs
- Verificar que `RTOS_PRINTF_AUTO` esté definido
- Confirmar que `rtos_printf` está configurado correctamente

## 📝 Notas Adicionales

- Los 30 pares de datos (GPS + IMU) cubren un ciclo completo de prueba
- Cada llamada a `TestData_GetNextGPS()` avanza el índice automáticamente
- El sistema detecta automáticamente cuando se acaban los datos
- Los datos están sincronizados: muestra N del GPS corresponde a muestra N del IMU

## 🎓 Próximos Pasos

Una vez validado el comportamiento básico de la FSM con estos datos:
1. Agregar más escenarios complejos (ej: múltiples escapes)
2. Implementar tests con temporizadores reales
3. Validar recuperación de errores
4. Probar con cerca de diferentes formas (triángulos, pentágonos)
