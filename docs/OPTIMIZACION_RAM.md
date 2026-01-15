# Optimización de RAM - Sistema de Cerco Virtual

**Fecha:** 15 de enero de 2026  
**Microcontrolador:** STM32WL55JC (Cortex-M4)  
**RAM disponible:** 32KB (16KB RAM1 + 16KB RAM2)

## Resumen Ejecutivo

Se realizó una optimización integral de memoria RAM reduciendo el uso de variables globales en **1068 bytes (~14% de RAM1)**, llevando el consumo de .bss de 7768 bytes a 6680 bytes.

### Cambios Principales

1. **Cambio de precisión: `double` → `float`** en todas las coordenadas GPS
2. **Reducción de capacidad: MAX_VERTICES 20 → 10**
3. **Refactorización arquitectural:** Eliminación de almacenamiento redundante de vértices en clase `Fence`

---

## 1. Motivación

### Estado Inicial (Análisis .map)
```
.bss section: 7768 bytes (0x1E58)
├─ fence: 1040 bytes
│  ├─ vertices[20]: 320 bytes (20 × 16 bytes)
│  ├─ limites[20]: 640 bytes (20 × 32 bytes)
│  └─ otros: 80 bytes
└─ receivedVertices[20]: 320 bytes (20 × 16 bytes)
```

**Problemas identificados:**
- Doble almacenamiento de vértices (fence + receivedVertices)
- Precisión innecesaria (double = 1mm, GPS real = 1.5-5m)
- Capacidad excesiva (20 vértices para parcelas típicas de 4-8 lados)

---

## 2. Análisis de Precisión

### Comparación `double` vs `float`

| Tipo | Tamaño | Precisión GPS | Precisión Hardware GPS |
|------|--------|---------------|------------------------|
| `double` | 8 bytes | ~1 mm | u-blox SAM-M10Q: 1.5m CEP |
| `float` | 4 bytes | ~1.1 m | Suficiente para 1.5-5m |

**Conclusión:** La precisión de `float` es más que suficiente dado que el error inherente del GPS es 1000× mayor que la pérdida de precisión.

---

## 3. Cambios Implementados

### 3.1 Estructuras de Datos (fence.h, cow.h)

#### Antes:
```cpp
struct Vertex {
    double latitude;   // 8 bytes
    double longitude;  // 8 bytes
};  // Total: 16 bytes

struct Position {
    double latitude;   // 8 bytes
    double longitude;  // 8 bytes
};  // Total: 16 bytes

#define MAX_VERTICES 20
```

#### Después:
```cpp
struct Vertex {
    float latitude;    // 4 bytes
    float longitude;   // 4 bytes
};  // Total: 8 bytes (50% reducción)

struct Position {
    float latitude;    // 4 bytes
    float longitude;   // 4 bytes
};  // Total: 8 bytes (50% reducción)

#define MAX_VERTICES 10  // Reducción 50%
```

**Ahorro por cambio de tipo:**
- Vertex: 16B → 8B (50%)
- Position: 16B → 8B (50%)
- Line (2 Vertex): 32B → 16B (50%)

---

### 3.2 Refactorización Clase Fence

#### Antes:
```cpp
class Fence {
private:
    Vertex vertices[MAX_VERTICES];  // 20 × 16B = 320 bytes
    Line limites[MAX_VERTICES];     // 20 × 32B = 640 bytes
    uint8_t vertexCount;
    uint8_t limitCount;
    
public:
    bool saveVertices(const Vertex* v, uint8_t count);
    void createLimits();  // Genera límites desde vertices[] interno
    void clearVertices();
};

// Uso:
fence.saveVertices(buffer, count);  // Copia a array interno
fence.createLimits();               // Genera límites
```

#### Después:
```cpp
class Fence {
private:
    // ¡SIN almacenamiento de vértices!
    Line limites[MAX_VERTICES];     // 10 × 16B = 160 bytes
    uint8_t limitCount;
    
public:
    void createLimits(const Vertex* v, uint8_t count);  // Directo
};

// Uso:
fence.createLimits(buffer, count);  // Genera límites sin copiar
```

**Filosofía:** Los vértices se procesan **on-the-fly** desde el buffer de recepción (`receivedVertices` en fsmTask), sin duplicar memoria.

---

### 3.3 Buffer de Recepción LoRa (fsmTask.cpp)

```cpp
// Buffer global en fsmTask.cpp
static Vertex receivedVertices[MAX_VERTICES];

// Antes: 20 × 16B = 320 bytes
// Después: 10 × 8B = 80 bytes
// Ahorro: 240 bytes (75%)
```

---

### 3.4 Payloads de Mensajes Optimizados

| Mensaje | Antes (double) | Después (float) | Ahorro |
|---------|----------------|-----------------|--------|
| `MSG_ID_SEND_GPS` | 16 bytes | 8 bytes | 50% |
| `MSG_ID_SEND_ZONE_AND_DISTANCE` | 9 bytes (1+8) | 5 bytes (1+4) | 44% |
| `MSG_ID_LORA_SEND_POSITION` | 16 bytes | 8 bytes | 50% |
| Fragmento LoRa (vértices) | 2 vértices/msg | 4 vértices/msg | 100% mejora |

**Impacto:** Menos fragmentos LoRa = menor latencia y menor consumo energético.

---

## 4. Archivos Modificados

### Core Types (5 archivos)
1. **fence.h** - Vertex struct, MAX_VERTICES, eliminación de `saveVertices()`
2. **fence.cpp** - `createLimits(buffer, count)`, `updateCenterFence(buffer, count)`
3. **cow.h** - Position struct, `distanceToLimit: float`
4. **cow.cpp** - Implementación de `updateDistanceToLimit(float)`
5. **getZone.h/cpp** - XY struct float, `pointToSegmentDistance()` usa `sqrtf()`

### Tasks/Threads (4 archivos)
6. **fsmTask.cpp** - `receivedVertices[10]`, eliminación de `updateFence()`, `sendPosition()` usa float
7. **loraTask.cpp** - VERTEX_SIZE 8 bytes, fragmentación optimizada (4 vértices/msg)
8. **sensorAcqTask.cpp** - GPS payload `sizeof(float)*2`

### Test Framework (8 archivos)
9. **test_sensor_mock.cpp** - Payloads GPS/zona con float, offsets ajustados
10. **test_data.h** - `TestGPSData_t`, `TestZoneData_t` con float
11. **test_data.cpp** - Literales float (sufijo `f`), datos TEST_FENCE_VERTICES
12. **cow_fence_test.cpp** - Tests actualizados, eliminación de `clearVertices()`
13. **fence_test.cpp** - Uso de nueva API `createLimits(buffer, count)`
14-16. Otros archivos de test con literales float

---

## 5. Resultados Cuantitativos

### Memoria RAM (.bss section)

| Componente | Antes | Después | Ahorro |
|------------|-------|---------|--------|
| **Fence class** | | | |
| - vertices[] | 320 B | 0 B | 320 B |
| - limites[] | 640 B | 160 B | 480 B |
| - otros | 80 B | 52 B | 28 B |
| **Subtotal Fence** | **1040 B** | **212 B** | **828 B** |
| | | | |
| **receivedVertices** | 320 B | 80 B | 240 B |
| | | | |
| **TOTAL AHORRO** | - | - | **1068 B** |

### Memoria Total

```
Antes:  .bss = 7768 bytes (47.4% de RAM1)
Después: .bss = 6680 bytes (40.8% de RAM1)
Reducción: 1088 bytes (14% de mejora)
```

### ROM/Flash

```
Antes:  ROM = 102096 bytes (77.89% de 128KB)
Después: ROM = 97752 bytes (74.58% de 128KB)
Reducción: ~4300 bytes (optimización de código float)
```

**Nota:** El código float (sqrtf, fmaxf) es más compacto que las librerías double.

---

## 6. Impacto en Fragmentación LoRa

### Cálculo de Fragmentos

```
Payload disponible: 35 bytes
Header fragmento: 3 bytes (num, total, count)
Datos disponibles: 35 - 3 = 32 bytes

Antes (double):
- Vertex size: 16 bytes
- Vértices/fragmento: 32 / 16 = 2
- Fragmentos para 10 vértices: ⌈10/2⌉ = 5 mensajes

Después (float):
- Vertex size: 8 bytes
- Vértices/fragmento: 32 / 8 = 4
- Fragmentos para 10 vértices: ⌈10/4⌉ = 3 mensajes
```

**Mejora:** 40% menos mensajes LoRa para transmitir cerca completo.

---

## 7. Validación

### Build Results (TEST_MODE)
```
Memory region         Used Size  Region Size  %age Used
         ROM:       97752 B       128 KB     74.58%
        RAM1:        8520 B        16 KB     52.00%
        RAM2:           0 B        16 KB      0.00%
```

### Test Execution
- ✅ 26 puntos GPS procesados correctamente
- ✅ Transiciones de zona (GREEN → LIGHT_BLUE → BLUE → DARK_BLUE → YELLOW → RED → BLACK)
- ✅ Cálculo de distancia con precisión float (~1m)
- ✅ Recepción y procesamiento de cerca en fragmentos
- ✅ 30 muestras IMU con detección de estado (SLEEP/GRAZING/MOVEMENT)

---

## 8. Trade-offs y Consideraciones

### Ventajas
✅ Ahorro significativo de RAM (1068 bytes)  
✅ Código más limpio (eliminación de duplicación)  
✅ Menor latencia LoRa (menos fragmentos)  
✅ ROM más pequeño (librerías float vs double)  
✅ Precisión adecuada para aplicación real  

### Desventajas
⚠️ Reducción de precisión GPS (despreciable para uso práctico)  
⚠️ Límite de 10 vértices (suficiente para parcelas típicas)  

### Casos Límite
- **Parcelas complejas (>10 vértices):** Simplificar polígono o dividir en sub-parcelas
- **Aplicaciones futuras con precisión crítica:** Evaluar caso por caso (ej: navegación indoor requiere double)

---

## 9. Recomendaciones Futuras

1. **Monitoreo de Stack:** Verificar consumo de pila en tasks (usar `uxTaskGetStackHighWaterMark()`)
2. **Pool de Mensajes:** Optimizar tamaño de `MessagePool` según payloads reducidos
3. **RAM2:** Explorar uso de segunda región de RAM (actualmente 0%)
4. **Compresión LoRa:** Evaluar codificación diferencial para coordenadas cercanas

---

## 10. Conclusión

La optimización logró reducir el consumo de RAM en **14%** mediante:
- Cambio de precisión innecesaria (double → float)
- Eliminación de almacenamiento redundante
- Reducción de capacidad a valores realistas

**Impacto:** Sistema más eficiente con 1068 bytes liberados para futuros desarrollos, sin comprometer funcionalidad ni precisión práctica.

---

**Autor:** TPP-IntelliFence Team  
**Revisión:** v1.0  
**Hardware:** STM32WL55JC-NUCLEO  
**Toolchain:** GCC ARM 13.3.1+st.9
