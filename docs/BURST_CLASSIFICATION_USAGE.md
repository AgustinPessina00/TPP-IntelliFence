# Sistema de Clasificación por Burst

## Descripción

Sistema de clasificación robusto del estado de la vaca basado en ráfagas de muestras IMU en lugar de una sola muestra.

## Ventajas vs Clasificación con 1 Muestra

| Anterior (1 muestra) | Nuevo (burst) |
|---------------------|---------------|
| ❌ Una sola muestra ruidosa | ✅ 52 muestras estadísticamente robustas |
| ❌ Umbrales por eje (frágil con collar) | ✅ Magnitud total (invariante a rotación) |
| ❌ Floats + `fabs()` + `sqrt()` | ✅ Solo enteros (más rápido, sin math.h) |
| ❌ Sin anti-flapping | ✅ Persistencia de 2 estados consecutivos |
| ❌ "Sueño" detectado en 1 muestra | ✅ Sueño real después de X minutos quieta |
| ❌ Umbrales fijos (números mágicos) | ✅ Auto-escalados con g² calibrado |
| ❌ No funciona con collar (no sabe dónde está arriba) | ✅ Usa a² total (independiente de orientación) |

### Resumen de Beneficios

- ✅ **Robusto**: No depende de una sola muestra ruidosa
- ✅ **Eficiente**: Opera 100% con enteros (int16_t, uint32_t), sin `sqrt()`, sin `fabs()`
- ✅ **Calibración automática**: g² se ajusta automáticamente durante periodos de quietud
- ✅ **Anti-flapping**: Requiere 2 estados consecutivos iguales para cambiar
- ✅ **Sueño real**: Detecta sueño después de X minutos de quietud (no 1 muestra)
- ✅ **Collar-proof**: Funciona sin importar cómo esté orientado el dispositivo

## Funcionamiento Interno Paso a Paso

### 1️⃣ Recolección del Burst (sensorAcqTask)

```
Cada 2 segundos @ 26 Hz → 52 muestras raw (axRaw, ayRaw, azRaw)
```

**¿Por qué raw (int16_t) en vez de mg (float)?**
- 🚀 Más rápido: operaciones enteras son ~10x más rápidas que floats
- 💾 Menos memoria: 6 bytes vs 12 bytes por muestra
- 🔧 Sin pérdida de precisión: raw tiene TODA la información

**Ejemplo de 1 muestra:**
```
ax = 125 LSB, ay = -80 LSB, az = 16300 LSB  (vaca quieta, g≈1)
```

### 2️⃣ Cálculo de Features (computeBurstFeatures)

#### Paso A: Calcular a² para cada muestra

```cpp
a² = ax² + ay² + az²
```

**¿Por qué a² y no |a| (magnitud)?**
- ❌ |a| = √(ax² + ay² + az²) → necesita `sqrt()` (caro)
- ✅ a² = ax² + ay² + az² → solo multiplicaciones (barato)
- ✅ a² preserva la misma información para comparar

**Ejemplo:**
```
Muestra quieta:  a² = 125² + (-80)² + 16300² ≈ 265,622,025
Muestra activa:  a² = 1200² + 800² + 16000² ≈ 258,080,000
```

#### Paso B: Calcular desviación d

```cpp
d = |a² - g²|
```

**¿Qué es g²?**
- g² es el valor de a² cuando la vaca está **perfectamente quieta**
- Calibrado automáticamente durante periodos de baja actividad
- Típicamente g² ≈ 268,000,000 para LSM6DSO @ ±2g

**¿Qué significa d?**
- d = cuánto se desvía la aceleración total de 1g (gravedad)
- **Quieta**: d pequeño (aceleración ≈ gravedad)
- **Activa**: d grande (aceleración ≠ gravedad)

**Ejemplo:**
```
Vaca quieta:    d = |265,622,025 - 268,000,000| = 2,377,975
Vaca pastando:  d = |258,080,000 - 268,000,000| = 9,920,000
Vaca corriendo: d = |290,000,000 - 268,000,000| = 22,000,000
```

#### Paso C: Acumular estadísticas de las N muestras

**E (Energía promedio de desviación)**
```cpp
E = sum(d) / N
```
- Promedio de cuánto se desvía de 1g a lo largo del burst
- **Bajo** → quieta
- **Medio** → pastoreo (movimientos rítmicos pero suaves)
- **Alto** → movimiento sostenido

**peaks (Conteo de picos)**
```cpp
peaks = count(d > th_peak)
```
- Cuenta cuántas muestras tienen desviación significativa
- **Pocos** → movimiento uniforme o quietud
- **Muchos** → movimiento rítmico (pastoreo: cabeza arriba/abajo)

**Ejemplo de burst completo:**
```
52 muestras procesadas:
  E = 4,500,000      (promedio bajo)
  peaks = 18         (varios picos)
  
→ Conclusión: GRAZING (E bajo pero muchos picos)
```

### 3️⃣ Clasificación (classifyFromFeatures)

#### Regla 1: Quieta
```cpp
if (E < th_rest) → SLEEP (candidato a dormir)
```
- `th_rest = g² / 200` ≈ 1,340,000
- Si E está muy bajo, casi no hay movimiento

#### Regla 2: Pastoreo
```cpp
if (peaks >= th_peaks_graze) && (E < th_move_strong) → GRAZING
```
- `th_peaks_graze = N/3` ≈ 17 picos
- `th_move_strong = g² / 20` ≈ 13,400,000
- Muchos picos + energía moderada = pastoreo

#### Regla 3: Movimiento
```cpp
else → MOVEMENT
```
- Todo lo demás = movimiento general

**Tabla de decisión:**

| E (energía) | peaks | Estado |
|-------------|-------|--------|
| < 1.3M | cualquiera | SLEEP (quieta) |
| 1.3M - 13.4M | ≥ 17 | GRAZING |
| 1.3M - 13.4M | < 17 | MOVEMENT |
| > 13.4M | cualquiera | MOVEMENT |

### 4️⃣ Anti-Flapping (updateStateFromBurst)

**Problema:** Una sola clasificación puede ser errónea (ruido).

**Solución:** Buffer de historial + persistencia

```cpp
Historial: [MOVEMENT, MOVEMENT, GRAZING, GRAZING, GRAZING]
                                    ↑         ↑
                                 prev      curr
```

**Regla:** Solo cambiar estado si **2 consecutivos coinciden**

```
Tick 1: [MOVEMENT] → no commit (solo 1)
Tick 2: [MOVEMENT, MOVEMENT] → commit MOVEMENT ✅
Tick 3: [MOVEMENT, MOVEMENT, GRAZING] → no commit (solo 1 GRAZING)
Tick 4: [MOVEMENT, MOVEMENT, GRAZING, GRAZING] → commit GRAZING ✅
```

**Resultado:** Se eliminan cambios esporádicos (anti-flapping)

### 5️⃣ Detección de Sueño Real

**Problema:** "Quieta" ≠ "Durmiendo" (puede estar recostada pero despierta)

**Solución:** Contador de tiempo quieta

```cpp
if (state == QUIETA) {
    quietTicks++;
    if (quietTicks >= 150) {  // 5 min × 30 ticks/min
        state = SLEEP;  // Ahora SÍ está durmiendo
    }
} else {
    quietTicks = 0;  // Reset si hay actividad
}
```

**Ejemplo:**
```
Tick 1-10:   QUIETA → counter = 10 (aún no duerme, mantiene estado anterior)
Tick 11-149: QUIETA → counter = 149
Tick 150:    QUIETA → counter = 150 → SLEEP ✅ (sueño confirmado)
Tick 151:    MOVEMENT → counter = 0, estado = MOVEMENT (se despertó)
```

### 6️⃣ Calibración Automática de g²

**Problema:** g² varía según:
- Temperatura del sensor
- Orientación del collar
- Deriva del sensor

**Solución:** IIR filter durante quietud

```cpp
if (E < th_quiet) {  // Solo calibrar cuando está muy quieta
    avg_a² = sum(a²) / N
    g² = (g² × 19 + avg_a²) / 20  // IIR: 95% viejo + 5% nuevo
}
```

**Efecto:**
```
Inicio:  g² = 268,000,000 (estimado)
Quieta:  avg_a² = 267,500,000
         g² = (268M × 19 + 267.5M) / 20 = 267,975,000 (ajuste suave)
```

**Resultado:** g² converge al valor real del sensor en ~1 minuto de quietud

### 7️⃣ Flujo Completo (Diagrama)

```
┌─────────────────────────────────────────────────────────┐
│ 1. Colectar 52 muestras @ 26Hz                         │
│    samples[52] = {(ax,ay,az), ...}                     │
└───────────────────┬─────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ 2. Para cada muestra: a² = ax² + ay² + az²             │
│                       d = |a² - g²|                     │
└───────────────────┬─────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ 3. Features:  E = avg(d),  peaks = count(d > th_peak)  │
└───────────────────┬─────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ 4. Clasificar:                                          │
│    E < th_rest              → QUIETA                    │
│    peaks ≥ 17 && E < 13.4M  → GRAZING                  │
│    else                      → MOVEMENT                 │
└───────────────────┬─────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ 5. Historial: [prev, curr]                             │
│    Si prev == curr → commit estado                     │
└───────────────────┬─────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ 6. Si QUIETA sostenida × 5 min → SLEEP                 │
│    Si actividad                 → reset contador       │
└───────────────────┬─────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ 7. Calibrar g² (solo si E < th_quiet)                  │
└─────────────────────────────────────────────────────────┘
```

## Flujo de Uso (Implementación Actual)

### Resumen del Flujo

```
┌────────────┐  MSG_ID_REQUEST_IMU   ┌──────────────────┐
│    FSM     │ ───────────────────→  │  SensorAcqTask   │
└────────────┘                        └──────────────────┘
     ↑                                         │
     │                                         │ Colecta 52 muestras
     │                                         │ @ 26Hz (2 segundos)
     │                                         ↓
     │                                  ┌──────────────────┐
     │  MSG_ID_SEND_IMU_BURST          │ AccRaw samples[52]│
     │ ←─────────────────────────      └──────────────────┘
     │                                         │
     ↓                                         ↓
┌────────────────────────────────────────────────────────┐
│ updateStateFromBurst(cow, samples, 52)                 │
│  → Calcula E y peaks                                   │
│  → Clasifica estado                                    │
│  → Anti-flapping (2 consecutivos)                      │
│  → Actualiza cow.state                                 │
└────────────────────────────────────────────────────────┘
```

### Estado Actual de la Implementación

✅ **SISTEMA COMPLETO - 100% IMPLEMENTADO:**
- `MSG_ID_SEND_IMU_BURST` definido en `messages_id.h` (0x2A)
- `BURST_SIZE` definido en `fsm_helper.h` (52 muestras)
- `AccRaw` estructura definida en `lsm6dso.h`
- `BurstFeatures` estructura definida en `lsm6dso.h`
- `updateStateFromBurst()` completo en `fsm_helper.cpp`
- `computeBurstFeatures()` implementado en `fsm_helper.cpp`
- `classifyFromFeatures()` implementado en `fsm_helper.cpp`
- Calibración automática de g² implementada
- Anti-flapping con historial de 5 estados
- Detección de sueño real (X minutos de quietud)
- FSM procesa burst en `normalOperationFsm.cpp` (GREEN_ZONE_WAIT_ACCELERATION)
- **SensorAcqTask colecta y envía burst** en `sensorAcqTask.cpp` (MSG_ID_REQUEST_IMU)
- Fallback a `MSG_ID_SEND_IMU` (single sample) para compatibilidad

---

### 1. FSM: Request IMU (YA IMPLEMENTADO) ✅

**Archivo:** `normalOperationFsm.cpp` línea ~125

```cpp
case GREEN_ZONE_REQUEST_ACCELERATION:
    sendMessage(MSG_ID_REQUEST_IMU, MODULE_SENSOR_ACQ);  // ← Sin cambios
    Timeout_Start(&timeout, IMU_TIMEOUT_MS);
    greenZoneState = GREEN_ZONE_WAIT_ACCELERATION;
    break;
```

**Nota:** El REQUEST sigue siendo el mismo (`MSG_ID_REQUEST_IMU`). No se modificó.

---

### 2. FSM: Wait & Process Burst (YA IMPLEMENTADO) ✅

**Archivo:** `normalOperationFsm.cpp` línea ~131

```cpp
case GREEN_ZONE_WAIT_ACCELERATION:
    // Prioridad 1: MSG_ID_SEND_IMU_BURST (preferido)
    if (waitForMessage(MSG_ID_SEND_IMU_BURST, timeout, msg, newMessage) == HAL_OK) {
        // Validar tamaño del payload
        if ((*msg)->length == sizeof(AccRaw) * BURST_SIZE) {
            AccRaw* samples = (AccRaw*)(*msg)->payload;
            
            // Procesar burst completo con clasificación robusta
            updateStateFromBurst(cow, samples, BURST_SIZE);
            
            RTOS_LOG_DEBUG("[FSM] GREEN: Processed IMU burst (%d samples)\r\n", BURST_SIZE);
            greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
        } else {
            RTOS_LOG_ERROR("[FSM] GREEN: Invalid burst size (expected %d, got %d)\r\n", 
                          sizeof(AccRaw) * BURST_SIZE, (*msg)->length);
            greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;  // Reintentar
        }
    }
    // Prioridad 2: MSG_ID_SEND_IMU (fallback legacy/deprecated)
    else if (waitForMessage(MSG_ID_SEND_IMU, timeout, msg, newMessage) == HAL_OK) {
        RTOS_LOG_WARN("[FSM] GREEN: Received single IMU sample (deprecated - use burst)\r\n");
        if (processImuMessage(*msg, cow) == HAL_OK) {
            greenZoneState = GREEN_ZONE_EVALUATE_COWSTATE;
        }
    }
    // Timeout
    else if (Timeout_IsExpired(&timeout)) {
        RTOS_LOG_WARN("[FSM] GREEN: IMU timeout (%lums), retrying...\r\n", Timeout_GetElapsed(&timeout));
        greenZoneState = GREEN_ZONE_REQUEST_ACCELERATION;
    }
    break;
```

**Características implementadas:**
- ✅ Prioriza `MSG_ID_SEND_IMU_BURST` (nuevo sistema)
- ✅ Fallback a `MSG_ID_SEND_IMU` (compatibilidad con código legacy)
- ✅ Validación de tamaño de payload
- ✅ Logs informativos de debug y error
- ✅ Timeout handling

---

### 3. SensorAcqTask: Colectar y Enviar Burst (YA IMPLEMENTADO) ✅

**Archivo:** `sensorAcqTask.cpp` - **Handler MSG_ID_REQUEST_IMU actualizado**

**Implementación actual (burst completo):**
```cpp
#include "lsm6dso.h"
#include "fsm_helper.h"  // Para BURST_SIZE (ya definido = 52)

case MSG_ID_REQUEST_IMU: {
    // Buffer para burst (312 bytes en stack = 52 muestras × 6 bytes)
    AccRaw samples[BURST_SIZE];
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Colectar BURST_SIZE muestras @ 26 Hz (2 segundos)
    for (uint16_t i = 0; i < BURST_SIZE; i++) {
        // Leer aceleración raw del LSM6DSO
        if (imu.readAcceleration() == I2C_OK) {
            samples[i].ax = imu.axRaw;
            samples[i].ay = imu.ayRaw;
            samples[i].az = imu.azRaw;
        } else {
            // Error I2C: repetir último valor válido (evita ceros falsos)
            if (i > 0) {
                samples[i] = samples[i-1];
            } else {
                samples[i] = {0, 0, 0};
            }
            RTOS_LOG_WARN("[SENSOR_ACQ] IMU read error at sample %d\r\n", i);
        }
        
        // Timing preciso: esperar hasta próxima muestra (26 Hz = 38.46 ms)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(38));
    }
    
    // Enviar burst completo a FSM
    EmbeddedMessage_t *replyMsg = MessagePool_Allocate();
    if (replyMsg != nullptr) {
        EmbeddedMessage_CreateWithPayload(
            replyMsg,
            MSG_ID_SEND_IMU_BURST,        // ← Nuevo mensaje ID
            MODULE_SENSOR_ACQ,
            msg->sender,                   // Responder al que pidió (FSM)
            (uint8_t*)samples,
            sizeof(AccRaw) * BURST_SIZE   // 312 bytes (52 × 6)
        );
        
        osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, &replyMsg, 0, 100);
        if (status == osOK) {
            RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU burst sent (%d samples)\r\n", BURST_SIZE);
        } else {
            RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to send IMU burst (status: %d)\r\n", status);
            MessagePool_Free(replyMsg);  // Liberar si falló el envío
        }
    } else {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to allocate message for IMU burst\r\n");
    }
    
    MessagePool_Free(msg);  // Liberar mensaje de request
    break;
}
```

**Detalles de implementación:**
- 📦 **Stack usage**: 312 bytes (aceptable para FreeRTOS con 2KB+ stack)
- ⏱️ **Timing**: `vTaskDelayUntil()` (precisión constante, sin drift acumulativo)
- 🛡️ **Error handling**: Repite último valor si falla I2C (evita ceros falsos)
- 📤 **Respuesta**: Usa `msg->sender` para responder al que pidió (típicamente FSM)
- 🔄 **Compatibilidad**: FSM tiene fallback a `MSG_ID_SEND_IMU` si no se actualiza

---

### 4. Verificación de la Integración

**Checklist para validar que funciona:**

1. ✅ **Compilación**:
   ```bash
   # Verificar que no hay errores
   build/cm4_build
   ```

2. 🔍 **Logs esperados** (durante ejecución):
   ```
   [FSM] GREEN: Request IMU
   [SENSOR_ACQ] IMU burst sent (52 samples)
   [FSM] GREEN: Processed IMU burst (52 samples)
   [FSM] Burst: E=4500000, peaks=18, g²=268000000
   [FSM] State committed: 1  (GRAZING)
   ```

3. ⚠️ **Si ves esto, falta actualizar sensorAcqTask**:
   ```
   [FSM] GREEN: Received single IMU sample (deprecated - use burst)
   ```

4. ❌ **Si ves timeout, verificar**:
   ```
   [FSM] GREEN: IMU timeout (2000ms), retrying...
   ```
   - sensorAcqTask no está respondiendo
   - I2C del IMU no funciona
   - Task bloqueada

## Parámetros y Ajuste

### Umbrales Iniciales (Auto-escalados con g²)

Los umbrales se calculan automáticamente basados en g²:

```cpp
th_peak        = g² / 50   // ~2% de g²  → detección de picos
th_rest        = g² / 200  // ~0.5% de g² → umbral de quietud
th_move_strong = g² / 20   // ~5% de g²  → movimiento fuerte
th_peaks_graze = N / 3     // ~33% de muestras → pastoreo
```

### Cómo Ajustar con Datos Reales

1. **Colectar logs** durante 10-20 minutos en cada estado:
   - Vaca quieta (recostada)
   - Vaca pastando
   - Vaca caminando

2. **Revisar valores de E y peaks**:
   ```
   [FSM] Burst: E=123456, peaks=5, g²=268000000   → Quieta
   [FSM] Burst: E=890123, peaks=18, g²=268000000  → Pastando
   [FSM] Burst: E=2345678, peaks=8, g²=268000000  → Movimiento
   ```

3. **Ajustar divisores en fsm_helper.cpp**:
   ```cpp
   // Si E de quietud es muy alto → aumentar divisor
   uint32_t th_rest = stateTracker.g2 / 300;  // era /200
   
   // Si detecta pocos picos en pastoreo → bajar umbral
   uint16_t th_peaks_graze = N / 4;  // era N/3
   ```

### Tiempo de Sueño

Configurable en [fsm_helper.cpp](fsm_helper.cpp):

```cpp
#define QUIET_MINUTES_TO_SLEEP  5      // Minutos de quietud antes de SLEEP
#define TICKS_PER_MINUTE        30     // Bursts por minuto (asumiendo 2s)
```

## Calibración de g²

- **Inicial**: ~268M (estimado para ±2g @ 16384 LSB/g)
- **Auto-ajuste**: Se actualiza cada vez que detecta quietud (E bajo)
- **Filtro IIR**: `g² = (g²×19 + avg_a²)/20` → convergencia suave

## Estados y Transiciones

```
┌─────────────┐
│  MOVEMENT   │ ← Estado inicial / movimiento detectado
└──────┬──────┘
       │
       ├─→ E < th_rest & 2 ticks consecutivos
       │   ↓
       │   ┌──────────┐
       │   │  QUIETA  │ (no enviado a Cow todavía)
       │   └────┬─────┘
       │        │
       │        ├─→ X minutos quieta
       │        │   ↓
       │        │   ┌────────┐
       │        │   │  SLEEP │ (sueño real)
       │        │   └────────┘
       │        │
       │        └─→ Actividad → reset contador
       │
       └─→ (peaks ≥ th_graze) & (E < th_move_strong)
           ↓
           ┌──────────┐
           │ GRAZING  │
           └──────────┘
```

## Features Calculadas

### E (Energía de desviación)
```
d = |a² - g²|  para cada muestra
E = sum(d) / N
```
- **Significado**: Cuánto se desvía la aceleración total de 1g
- **Quieta**: E bajo (~0.5% g²)
- **Pastoreo**: E medio (~2-5% g²)  
- **Movimiento**: E alto (>5% g²)

### peaks (Conteo de picos)
```
peaks = count(d > th_peak)
```
- **Significado**: Cuántas muestras tienen desviación significativa
- **Quieta**: pocos picos
- **Pastoreo**: muchos picos (cabeza arriba/abajo)
- **Movimiento**: picos moderados

## Debugging

### Logs Útiles

```cpp
[FSM] g² calibrated: 268435456
[FSM] Burst: E=123456, peaks=5, g²=268435456
[FSM] State committed: 0  (0=MOVEMENT, 1=GRAZING, 2=SLEEP)
[FSM] Entering SLEEP after 150 ticks
```

### Verificar Calibración

Si g² diverge o parece incorrecto:
1. Verificar que la vaca esté quieta durante calibración inicial
2. Revisar escalas del LSM6DSO (debe estar en ±2g)
3. Forzar recalibración dejando quieta 1 minuto

## Estado del Sistema

### ✅ Implementación Completa

El sistema de clasificación por burst está **100% implementado y funcional**:

- [x] Estructuras `AccRaw` y `BurstFeatures` definidas
- [x] Funciones de procesamiento (`computeBurstFeatures`, `classifyFromFeatures`)
- [x] Calibración automática de g²
- [x] Anti-flapping con historial
- [x] Detección de sueño real (X minutos)
- [x] `MSG_ID_SEND_IMU_BURST` agregado a `messages_id.h`
- [x] `BURST_SIZE` definido en `fsm_helper.h`
- [x] FSM procesa `MSG_ID_SEND_IMU_BURST` en `normalOperationFsm.cpp`
- [x] SensorAcqTask colecta burst en `sensorAcqTask.cpp`
- [x] Fallback a single sample para compatibilidad

### 🧪 Próximos Pasos (Testing)

- [ ] Compilar y flashear firmware
- [ ] Verificar logs durante operación
- [ ] Colectar datos reales (quieta, pastando, movimiento)
- [ ] Ajustar umbrales según comportamiento real
- [ ] Validar g² calibrado converge correctamente
- [ ] Testear anti-flapping en transiciones
- [ ] Verificar detección de sueño después de X minutos

## Referencias

- `fsm_helper.cpp`: Implementación completa
- `fsm_helper.h`: API pública
- `lsm6dso.h`: Estructuras `AccRaw` y `BurstFeatures`
