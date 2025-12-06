# LoRaWAN End Node - Branch: LoRaWAN-EndNode-Test

Esta branch contiene la implementación básica de un end node LoRaWAN para el STM32WL55JC1, conectándose al gateway ChirpStack configurado en Raspberry Pi 4 + RAK5146.

## 📋 Resumen de Cambios

### Archivos Nuevos

1. **`CM4/Core/Inc/threads/loraTask.h`** - Header del módulo LoRa
2. **`CM4/Core/Src/threads/loraTask.cpp`** - Implementación de loraTask
3. **`docs/LORAWAN_SETUP.md`** - Guía completa de configuración
4. **`docs/LORAWAN_QUICK_START.md`** - Guía rápida de integración

### Archivos Modificados

1. **`CM4/Core/Src/app_freertos.c`**
   - Agregado `#include "threads/loraTask.h"`
   - Creada cola `loraQueueHandle` para comunicación con loraTask
   - loraTask ya está inicializado en el sistema FreeRTOS

2. **`CM4/LoRaWAN/App/lora_app.c`**
   - Implementado `SendTxData()` para enviar payload de GPS
   - Implementado `OnJoinRequest()` para notificar a loraTask
   - Implementado `OnRxData()` con logging

3. **`CM4/CMakeLists.txt`**
   - Agregado `loraTask.cpp` a las fuentes compilables

4. **`CM0PLUS/LoRaWAN/App/se-identity.h`** (necesitas configurar)
   - Definir `LORAWAN_APP_KEY` según ChirpStack

## 🚀 Quick Start

### 1. Configuración Mínima

Edita `CM0PLUS/LoRaWAN/App/se-identity.h`:

```c
#define LORAWAN_DEVICE_EUI    00,00,00,00,00,00,00,00  // Auto-generado
#define LORAWAN_JOIN_EUI      00,00,00,00,00,00,00,00  
#define LORAWAN_APP_KEY       2B,7E,15,16,28,AE,D2,A6,AB,F7,15,88,09,CF,4F,3C
```

### 2. Compilar

```bash
cd CM4/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### 3. Flashear

Usa STM32CubeProgrammer o tu método preferido.

### 4. Obtener DevEUI

Monitorea el UART (115200 baud) para ver el DevEUI auto-generado:
```
[LORA] DevEUI: 00:80:E1:10:00:XX:XX:XX
```

### 5. Configurar ChirpStack

1. Accede a http://<IP_RASPBERRY>:8080
2. Crea una aplicación: **IntelliFence**
3. Agrega device con el DevEUI obtenido
4. Configura `LORAWAN_APP_KEY` igual que en el código

## 📡 Arquitectura

```
┌─────────────────────────────────────┐
│   fsmTask / sensorAcqTask           │
│   (Obtiene datos GPS)               │
└──────────────┬──────────────────────┘
               │
               │ sendGpsDataToLora()
               ▼
┌─────────────────────────────────────┐
│   loraTask                          │
│   - Cola: loraQueueHandle           │
│   - Procesa LoraGpsData_t           │
└──────────────┬──────────────────────┘
               │
               │ loraTaskGetPayload()
               ▼
┌─────────────────────────────────────┐
│   lora_app.c                        │
│   - SendTxData()                    │
│   - Stack LoRaWAN                   │
└──────────────┬──────────────────────┘
               │
               │ LoRaWAN AU915
               ▼
┌─────────────────────────────────────┐
│   ChirpStack Gateway                │
│   - RAK5146 Concentrator            │
│   - Raspberry Pi 4                  │
└─────────────────────────────────────┘
```

## 📦 Payload Format

El device envía **17 bytes** por puerto 2:

| Offset | Size | Field     | Type   | Description                      |
|--------|------|-----------|--------|----------------------------------|
| 0      | 8    | Latitude  | double | GPS latitude (IEEE 754)          |
| 8      | 8    | Longitude | double | GPS longitude (IEEE 754)         |
| 16     | 1    | Zone      | uint8  | 0=GREEN, 1=YELLOW, 2=ORANGE, 3=RED, 4=BLACK |

## 🧪 Testing

### Código de Prueba Simple

Agrega temporalmente en `fsmTask()`:

```cpp
// Código de prueba - cada 10 segundos
static uint32_t testCounter = 0;
if (testCounter % 100 == 0) {
    LoraGpsData_t testData;
    testData.latitude = -34.603722;
    testData.longitude = -58.381592;
    testData.zone = testCounter % 5;
    testData.distance = 50.0f;
    sendGpsDataToLora(&testData);
}
testCounter++;
```

### Logs Esperados

```
[LORA_TASK] Inicializando LoRaWAN...
JOIN SUCCESS!
[LORA_TASK] Datos GPS recibidos: lat=-34.603722, lon=-58.381592
Sending GPS data: 17 bytes
```

## 🔧 Configuración Avanzada

### Ajustar Duty Cycle

En `CM4/LoRaWAN/App/lora_app.h`:

```c
#define APP_TX_DUTYCYCLE  10000  // 10 segundos (en ms)
```

### Cambiar Región

```c
#define ACTIVE_REGION  LORAMAC_REGION_AU915  // o US915, EU868, AS923
```

### Data Rate

```c
#define LORAWAN_DEFAULT_DATA_RATE  DR_0  // DR_0 = más robusto, más lento
```

## 📚 Documentación Completa

- **[LORAWAN_SETUP.md](docs/LORAWAN_SETUP.md)** - Configuración detallada del sistema
- **[LORAWAN_QUICK_START.md](docs/LORAWAN_QUICK_START.md)** - Integración con fsmTask

## 🐛 Troubleshooting

### No compila
```bash
# Limpiar build
rm -rf CM4/build/*
cd CM4/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### JOIN falla
- Verifica que el gateway esté encendido y configurado en AU915
- Verifica que `LORAWAN_APP_KEY` coincida con ChirpStack
- Verifica la distancia al gateway (< 1km en línea de vista)

### No envía datos
- Verifica que `sendGpsDataToLora()` se llame correctamente
- Verifica logs: `[LORA_TASK] Datos GPS recibidos`
- Verifica que JOIN haya sido exitoso

## 📝 TODOs

- [ ] Probar el JOIN con el gateway real
- [ ] Verificar el DevEUI generado
- [ ] Configurar ChirpStack con el device
- [ ] Probar envío de datos de prueba
- [ ] Integrar con GPS real
- [ ] Implementar downlink para recibir fences
- [ ] Optimizar consumo de energía

## 🎯 Próximos Pasos

1. **Probar conexión básica** con código de prueba
2. **Integrar con GPS real** desde sensorAcqTask
3. **Implementar downlink** para recibir nuevos fences
4. **Agregar telemetría** (batería, temperatura, etc.)
5. **Optimizar consumo** según estado de la vaca

## ⚠️ Notas Importantes

- Esta es una implementación **básica** para testing
- El `APP_TX_DUTYCYCLE` de 10 segundos es solo para pruebas
- En producción, ajustar según duty cycle regulations de AU915
- El payload es simple, puede extenderse con más datos

## 📞 Contacto

Para preguntas o problemas, revisa los logs del device y del gateway en conjunto.

---

**Branch:** LoRaWAN-EndNode-Test  
**Última actualización:** Diciembre 2025  
**Estado:** ✅ Listo para testing inicial
