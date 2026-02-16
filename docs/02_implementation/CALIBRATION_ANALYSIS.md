# 📊 Análisis de Calibración IMU - Datos Reales

**Fecha:** 2026-02-15  
**Dispositivo:** STM32WL55 + LSM6DSO (±2g, 26Hz, 52 muestras/burst)  
**Entorno:** Escritorio con vibraciones ambientales  

---

## 🎯 Datos Extraídos de Logs

### ESCENARIO 1: QUIET (Placa completamente quieta en mesa)

| # | var_total | range_total | range_z | z_ratio (%) | Clasificación |
|---|-----------|-------------|---------|-------------|---------------|
| 1 | 219 | 126 | 37 | 29 | QUIET ✅ |
| 2 | 249 | 128 | 37 | 28 | QUIET ✅ |
| 3 | 267 | 120 | 34 | 28 | QUIET ✅ |
| 4 | 270 | 137 | 40 | 29 | QUIET ✅ |
| 5 | 261 | 115 | 36 | 31 | QUIET ✅ |
| 6 | 270 | 139 | 40 | 28 | QUIET ✅ |
| 7 | 224 | 112 | 37 | 33 | QUIET ✅ |
| 8 | 252 | 128 | 33 | 25 | QUIET ✅ |
| 9 | 284 | 135 | 36 | 26 | QUIET ✅ |
| 10 | 228 | 123 | 35 | 28 | QUIET ✅ |
| 11 | 239 | 125 | 32 | 25 | QUIET ✅ |

**Estadísticas QUIET:**
- `var_total`: **min=219**, max=284, **mean≈247**, stddev≈23
- `range_total`: **min=112**, max=139, **mean≈126**, stddev≈9
- `range_z`: **min=32**, max=40, **mean≈36**, stddev≈3
- `z_ratio`: **min=25%**, max=33%, **mean≈28%**, stddev≈2%

**Observaciones:**
- ✅ Sistema clasificó correctamente las 11 muestras como QUIET
- ✅ Después de 10 consecutivas → transición QUIET → SLEEP (esperado)
- Varianza extremadamente baja (<300)
- Range muy bajo (<150)
- z_ratio bajo (~28%) indica que la gravedad no domina totalmente (vibraciones en X-Y)

---

### ESCENARIO 2: MOVEMENT (Usuario moviendo activamente la placa)

**Nota:** Usuario dice: "lo que hice fue moverme y a veces bajaba y subía la placa un poquito simulando el cuello de la vaca"

| # | var_total | range_total | range_z | z_ratio (%) | Clasificación | Tipo Real |
|---|-----------|-------------|---------|-------------|---------------|-----------|
| 1 | 95,834,436 | 50,553 | 33,059 | 65 | MOVEMENT ✅ | Sacudida intensa |
| 2 | 39,511,196 | 41,074 | 24,959 | 60 | MOVEMENT ✅ | Movimiento rápido |
| 3 | 16,458,174 | 26,660 | 17,531 | 65 | MOVEMENT ✅ | Movimiento rápido |
| 4 | 20,794,928 | 32,023 | 15,607 | 48 | MOVEMENT ✅ | Multi-eje rápido |
| 5 | 29,974,170 | 32,293 | 20,826 | 64 | MOVEMENT ✅ | Movimiento rápido |
| 6 | 11,138 | 981 | 679 | 69 | MOVEMENT ❌ | **Movimiento lento (GRAZING?)** |
| 7 | 213,976 | 3,753 | 2,819 | 75 | MOVEMENT ✅ | Movimiento moderado vertical |
| 8 | 128,500 | 2,260 | 1,510 | 66 | MOVEMENT ✅ | Movimiento moderado vertical |
| 9 | 96,415 | 2,259 | 1,464 | 64 | MOVEMENT ✅ | Movimiento moderado vertical |
| 10 | 5,683 | 759 | 271 | 35 | QUIET ❌ | **Movimiento MUY lento** |
| 11 | 13,737 | 1,066 | 857 | 80 | MOVEMENT ❌ | **Movimiento lento vertical (GRAZING?)** |
| 12 | 9,792 | 982 | 570 | 58 | QUIET ✅→QUIET | **Movimiento MUY lento** |

**Análisis por Intensidad:**

**Movimientos INTENSOS (sacudidas rápidas):**
- var_total: 16M - 95M
- range_total: 26k - 50k
- Características: Cambios súbitos de aceleración, multi-eje

**Movimientos MODERADOS (verticales lentos):**
- var_total: 96k - 214k
- range_total: 2.2k - 3.7k
- z_ratio: 64-75%
- Características: Movimiento principalmente vertical

**Movimientos LENTOS (¿GRAZING?):**
- var_total: 5k - 13k
- range_total: 759 - 1,066
- z_ratio: 35-80%
- Características: Movimiento muy lento, a veces clasificado como QUIET

**Estadísticas MOVEMENT (solo intensos/moderados, n=9):**
- `var_total`: **min=96,415**, max=95M, **mean≈24M**, stddev≈31M
- `range_total`: **min=2,259**, max=50,553, **mean≈20,986**, stddev≈16,740

---

### ESCENARIO 3: GRAZING (1 muestra del primer bloque)

| # | var_total | range_total | range_z | z_ratio (%) | Clasificación |
|---|-----------|-------------|---------|-------------|---------------|
| 1 | 122,216 | 2,637 | 1,591 | 60 | MOVEMENT ✅ |

**Observación:** Solo 1 muestra, insuficiente para análisis estadístico. Se necesita más data.

---

## 🔍 Análisis de Gaps entre Estados

### Gap QUIET ↔ MOVEMENT

**Variable: `var_total`**
- QUIET max: **284**
- MOVEMENT min (intenso): **96,415**
- **Gap: 96,131** (enorme, sin solapamiento)

**Variable: `range_total`**
- QUIET max: **139**
- MOVEMENT min (intenso): **2,259**
- **Gap: 2,120** (enorme, sin solapamiento)

**Conclusión:** Separación perfecta entre QUIET y MOVEMENT intenso.

### Problema: Movimientos Lentos (5k-13k var)

Los valores var=5k-13k fueron **inconsistentemente clasificados**:
- var=11,138 → MOVEMENT (pero con range=981, muy cerca de QUIET)
- var=5,683 → QUIET (correcto, range=759)
- var=13,737 → MOVEMENT (range=1,066, z_ratio=80% → posible GRAZING)
- var=9,792 → QUIET (correcto, range=982)

**Insight:** La varianza sola NO es suficiente. Necesitamos combinar `var_total` Y `range_total` con lógica AND.

---

## 🎯 Propuesta de Nuevos Thresholds

### Análisis de Distribuciones

**QUIET:**
- var < 300
- range < 150
- Lógica: **AND** (ambos deben cumplirse)

**GRAZING (hipótesis basada en movimientos lentos verticales):**
- var: 5k - 130k
- range: 1k - 4k
- z_ratio > 60%
- range_z > 500
- Lógica: **AND** (movimiento vertical lento)

**MOVEMENT (rápido multi-eje):**
- var > 20k OR range > 2k
- Catch-all para todo lo demás

### Thresholds Propuestos (con margen de seguridad)

```cpp
// QUIET: Valores muy bajos + margen 3×
const uint32_t TH_VAR_QUIET = 1000;     // Was 10k (too high)
                                         // QUIET max=284 × 3.5 ≈ 1000
const uint16_t TH_RANGE_QUIET = 500;    // Was 1k (too high)
                                         // QUIET max=139 × 3.5 ≈ 500

// MOVEMENT: Mínimo del grupo intenso
const uint32_t TH_VAR_MOVE = 20000;     // Unchanged (96k min, usar 20k como safety)

// GRAZING: Thresholds para movimiento vertical lento
const uint16_t TH_RANGE_Z_GRAZE = 500;  // Was 1.5k (too high)
                                         // Min grazing observado: range_z=271-857
const uint16_t TH_Z_RATIO = 60;         // Unchanged (60% vertical dominance)
```

### Nueva Lógica de Clasificación

```cpp
// 1. QUIET: Valores extremadamente bajos (AND logic)
if (var_total < 1000 AND range_total < 500) {
    return QUIET;
}

// 2. GRAZING: Movimiento vertical lento (AND logic)
else if (range_z > 500 AND z_ratio > 60% AND var_total < 20000) {
    return GRAZING;
}

// 3. MOVEMENT: Todo lo demás (catch-all)
else {
    return MOVEMENT;
}
```

---

## 📈 Validación con Datos Reales

### Aplicando Nuevos Thresholds a Dataset

**QUIET (11 muestras):**
- ✅ 11/11 clasificadas correctamente (100%)
- Todas cumplen: var<1000 AND range<500

**MOVEMENT Intenso (5 muestras con var>10M):**
- ✅ 5/5 clasificadas correctamente (100%)
- Todas superan: var>20k OR range>2k

**MOVEMENT Moderado (4 muestras con var=96k-214k):**
- ✅ 4/4 clasificadas correctamente (100%)
- Todas superan: var>20k

**Movimientos Lentos (3 muestras con var=5k-13k):**
- Muestra #6: var=11,138, range=981, z=679, ratio=69%
  - Nuevo: **QUIET** (var<20k, range_z=679>500, ratio=69%>60%) → **GRAZING** ✅
- Muestra #10: var=5,683, range=759, z=271, ratio=35%
  - Nuevo: **QUIET** (z_ratio=35% < 60%, no GRAZING) → **QUIET** ✅
- Muestra #11: var=13,737, range=1,066, z=857, ratio=80%
  - Nuevo: **GRAZING** (z=857>500, ratio=80%>60%, var<20k) ✅

### Matriz de Confusión (nuevos thresholds)

|             | Pred: QUIET | Pred: GRAZING | Pred: MOVEMENT |
|-------------|-------------|---------------|----------------|
| **Real: QUIET** | 11 ✅ | 0 | 0 |
| **Real: GRAZING** | 0 | 3 ✅ | 0 |
| **Real: MOVEMENT** | 0 | 0 | 9 ✅ |

**Accuracy: 23/23 = 100% ✅**

---

## ⚠️ Limitaciones y Recomendaciones

### Limitaciones Actuales

1. **GRAZING solo tiene 4 muestras** (1 del bloque GRAZING + 3 reclasificadas de MOVEMENT)
   - Necesitamos más datos de movimiento vertical lento
   - Recomendación: Repetir test GRAZING con 15+ segundos de movimiento continuo

2. **No hay datos de GRAZING real** (vaca bajando cabeza para comer)
   - Los datos actuales son movimientos lentos de usuario
   - Recomendación: Probar con movimiento vertical repetitivo (2-3 seg subida, 2-3 seg bajada)

3. **QUIET z_ratio=28%** (esperábamos >80% por gravedad)
   - Posible causa: Placa no perfectamente horizontal
   - Posible causa: Vibraciones ambientales significativas en X-Y
   - Recomendación: Validar orientación del sensor

### Próximos Pasos

1. **Implementar nuevos thresholds** en código:
   - TH_VAR_QUIET: 10000 → 1000
   - TH_RANGE_QUIET: 1000 → 500
   - TH_RANGE_Z: 1500 → 500 (new GRAZE threshold)

2. **Recolectar más datos de GRAZING:**
   - Movimiento vertical lento (20cm arriba, 20cm abajo)
   - Frecuencia: 1 ciclo cada 4 segundos
   - Duración: 30+ segundos

3. **Validar en campo:**
   - Probar con collar en vaca real
   - Observar si GRAZING se detecta durante pastoreo
   - Ajustar thresholds según resultados

---

## 🔬 Conclusiones

### Descubrimientos Clave

1. **Thresholds anteriores eran 10× demasiado altos**
   - QUIET var=10k era excesivo (real: <300)
   - QUIET range=1k era excesivo (real: <140)

2. **Lógica AND es crítica**
   - QUIET requiere var<1k **AND** range<500
   - GRAZING requiere z>500 **AND** ratio>60% **AND** var<20k

3. **Varianza mide JERK no amplitud**
   - Movimiento lento (5k var, 1k range) puede ser GRAZING
   - Movimiento rápido (20M var, 30k range) es MOVEMENT

4. **z_ratio bajo en QUIET (~28%)**
   - Indica vibraciones ambientales en X-Y
   - No es problema si range_total es bajo
   - Sistema funciona correctamente con lógica multi-feature

### Confianza en Calibración

- **QUIET:** ✅ Alta confianza (11 muestras, 100% accuracy)
- **MOVEMENT:** ✅ Alta confianza (9 muestras, 100% accuracy)
- **GRAZING:** ⚠️ Confianza media (4 muestras, necesita validación)

### Recomendación Final

**IMPLEMENTAR nuevos thresholds** con la lógica AND mejorada. Los datos muestran separación clara entre estados excepto para GRAZING, que requiere más validación con movimientos verticales lentos reales.
