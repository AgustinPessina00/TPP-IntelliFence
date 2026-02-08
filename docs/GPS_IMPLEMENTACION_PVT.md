# Implementación GPS - UBX-NAV-PVT Polling

## Resumen de Modificaciones en `sam_m10q.cpp`

Este documento detalla las modificaciones realizadas para implementar la funcionalidad de polling UBX-NAV-PVT en el módulo GPS SAM-M10Q, permitiendo obtener datos de posición, velocidad y tiempo.

---

## 📋 Funciones Implementadas

### 1. `getPVT()`
**Función principal** para obtener datos PVT del GPS.

**Flujo:**
1. Limpia la estructura de datos
2. Solicita mensaje PVT vía `requestPVT()`
3. Recibe y parsea respuesta vía `receivePVT()` o `receivePVTValidateOption()`
4. Valida que hay un fix GPS válido (fixType >= 2)

**Uso:**
```cpp
UBX_NAV_PVT_data_t pvtData;

if (gps.getPVT(&pvtData, 2000)) {
    double latitude = pvtData.lat * 1e-7;   // deg
    double longitude = pvtData.lon * 1e-7;  // deg
    printf("Lat: %.7f, Lon: %.7f\r\n", latitude, longitude);
    printf("Satélites: %d\r\n", pvtData.numSV);
}
```

---

### 2. `requestPVT()`
**Construye y envía** el mensaje de polling UBX-NAV-PVT al GPS.

**Mensaje enviado:**
```
B5 62 01 07 00 00 [CRC-A] [CRC-B]
│  │  │  │  └─┴─ Length: 0 bytes (polling message sin payload)
│  │  │  └─ ID: 0x07 (PVT)
│  │  └─ Class: 0x01 (NAV)
│  └─ Sync char 2: 0x62
└─ Sync char 1: 0xB5
```

**Características:**
- Mensaje de 8 bytes total
- Checksum UBX calculado automáticamente
- Delay de 10ms para procesamiento del GPS

---

### 3. `receivePVT()` - Opción Simple (Bulk Read)
**Lectura optimizada en chunks** para máxima velocidad.

**Características:**
- ✅ Lectura en chunks de 64 bytes (respeta límite I2C)
- ✅ Espera hasta que hay 100 bytes disponibles
- ✅ Limpia bit 15 del registro 0xFD (bug firmware GPS)
- ✅ Delay de 10ms en caso de error I2C
- ✅ Timeout configurable

**Flujo:**
1. Verifica bytes disponibles en registro 0xFD
2. Si hay >= 100 bytes, lee en 2 chunks (64 + 36 bytes)
3. Parsea mensaje completo con `parseUBXMessage()`

**Uso recomendado:**
- GPS dedicado solo para UBX-NAV-PVT
- Sin mensajes NMEA habilitados
- Ambiente estable sin interferencias

---

### 4. `receivePVTValidateOption()` - Opción Robusta (State Machine)
**Máquina de estados byte-a-byte** para máxima robustez.

**Estados implementados:**
```
WAITING_SYNC1      → Busca 0xB5
WAITING_SYNC2      → Busca 0x62
READING_CLASS      → Lee Class byte
READING_ID         → Lee ID byte
READING_LENGTH_LSB → Lee Length LSB
READING_LENGTH_MSB → Lee Length MSB + valida tamaño
READING_PAYLOAD    → Lee 92 bytes de payload
READING_CHECKSUM_A → Lee CK_A
READING_CHECKSUM_B → Lee CK_B + valida mensaje completo
```

**Características:**
- ✅ Polling adaptativo de 100ms (optimizado para GPS @ 1Hz)
- ✅ Auto-recuperación ante basura en buffer
- ✅ Puede procesar múltiples mensajes UBX mezclados
- ✅ Validación incremental (detecta errores temprano)
- ✅ Lectura en chunks de 64 bytes por iteración
- ✅ Protección contra buffer overflow

**Flujo:**
1. Polling cada 100ms (no satura el bus)
2. Lee hasta 64 bytes del FIFO I2C (registro 0xFF)
3. Procesa byte por byte con máquina de estados
4. Encuentra sync chars automáticamente
5. Valida tamaño de payload (debe ser 92 bytes)
6. Retorna cuando encuentra mensaje NAV-PVT válido

**Uso recomendado:**
- GPS con NMEA y UBX habilitados simultáneamente
- Ambiente con interferencias/ruido
- Máxima robustez requerida

---

### 5. `parseUBXMessage()`
**Extrae y convierte** los 92 bytes del payload a estructura C.

**Validaciones realizadas:**
- ✅ Verifica headers 0xB5 0x62
- ✅ Valida Class=0x01 y ID=0x07
- ✅ Confirma payload length = 92 bytes
- ✅ Verifica checksum UBX

**Campos extraídos (little-endian):**
```cpp
iTOW        → GPS time of week [ms]
year/month/day/hour/min/sec → Fecha y hora UTC
fixType     → Tipo de fix (0=no fix, 2=2D, 3=3D)
numSV       → Número de satélites
lat/lon     → Latitud/Longitud [deg * 1e-7]
height/hMSL → Altura elipsoide/nivel del mar [mm]
hAcc/vAcc   → Precisión horizontal/vertical [mm]
velN/velE/velD → Velocidad NED [mm/s]
gSpeed      → Velocidad en tierra [mm/s]
headMot     → Rumbo de movimiento [deg * 1e-5]
pDOP        → Position DOP [* 0.01]
```

---

### 6. `verifyUBXChecksum()`
**Valida la integridad** del mensaje UBX recibido.

**Algoritmo Fletcher:**
```cpp
CK_A = 0, CK_B = 0
Para cada byte desde Class hasta último byte de Payload:
    CK_A = CK_A + byte
    CK_B = CK_B + CK_A
```

---

## 🔧 Correcciones Críticas Aplicadas

### Bug #1: Parámetros faltantes en `memRead()`
**Problema:** Faltaban `I2C_MEMADD_SIZE_8BIT` y timeout.
```cpp
// ❌ Antes
i2cBus->memRead(i2cAddr, 0xFD, bytesAvailable, 2)

// ✅ Después
i2cBus->memRead(i2cAddr, 0xFD, I2C_MEMADD_SIZE_8BIT, bytesAvailable, 2, 100)
```

### Bug #2: Lectura de 100 bytes en transacción única
**Problema:** Excedía límite hardware I2C de 64 bytes.
```cpp
// ❌ Antes
i2cBus->memRead(i2cAddr, 0xFF, buffer, 100, 100)

// ✅ Después (Chunked read)
while (totalRead < 100) {
    uint8_t toRead = min(64, 100 - totalRead);
    i2cBus->memRead(i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, &buffer[totalRead], toRead, 100);
    totalRead += toRead;
}
```

### Bug #3: Bit 15 del registro 0xFD
**Problema:** Firmware GPS tiene bug conocido en bit 15.
```cpp
// ✅ Solución
uint16_t available = (bytesAvailable[1] << 8) | bytesAvailable[0];
available &= 0x7FFF; // Limpiar bit 15
```

### Optimización #1: Polling rate
**Problema:** Polling cada 1ms era innecesario para GPS @ 1Hz.
```cpp
// ❌ Antes
BusyDelayMs(1);

// ✅ Después (receivePVTValidateOption)
const uint32_t pollingWait = 100; // 100ms entre checks
if ((HAL_GetTick() - lastCheck) < pollingWait) {
    BusyDelayMs(10);
    continue;
}
```

### Optimización #2: Delay redundante eliminado
```cpp
// ❌ Antes
BusyDelayMs(10);
BusyDelayMs(1);

// ✅ Después
BusyDelayMs(10);
```

---

## 📊 Comparativa de Implementaciones

| Característica | `receivePVT()` | `receivePVTValidateOption()` |
|----------------|----------------|------------------------------|
| **Velocidad** | ⚡ Muy rápida (2 transacciones I2C) | 🐢 Más lenta (múltiples iteraciones) |
| **Robustez** | ⚠️ Asume buffer limpio | ✅ Máxima (auto-recuperación) |
| **Complejidad** | ✅ Simple | ⚠️ Compleja |
| **Polling rate** | 10ms (en error) | 100ms (adaptativo) |
| **Chunk size** | 64 bytes | 64 bytes |
| **Manejo NMEA** | ❌ No | ✅ Sí |
| **Basura en buffer** | ❌ Puede fallar | ✅ La filtra |
| **Uso CPU** | Bajo | Medio |

---

## 🎯 Recomendaciones de Uso

### Usar `receivePVT()` cuando:
- ✅ GPS configurado solo con UBX-NAV-PVT
- ✅ Mensajes NMEA deshabilitados
- ✅ Máxima velocidad requerida
- ✅ Ambiente controlado/estable

### Usar `receivePVTValidateOption()` cuando:
- ✅ GPS con NMEA y UBX mezclados
- ✅ Ambiente con interferencias
- ✅ Robustez crítica
- ✅ Debug/desarrollo

---

## 📝 Estructura de Datos UBX-NAV-PVT

```cpp
typedef struct {
    uint32_t iTOW;      // [ms] GPS time of week
    uint16_t year;      // [year] UTC
    uint8_t month;      // [1..12]
    uint8_t day;        // [1..31]
    uint8_t hour;       // [0..23]
    uint8_t min;        // [0..59]
    uint8_t sec;        // [0..60]
    uint8_t valid;      // Validity flags
    uint32_t tAcc;      // [ns] Time accuracy
    int32_t nano;       // [ns] Fraction of second
    uint8_t fixType;    // 0=no fix, 2=2D, 3=3D
    uint8_t flags;      // Fix status flags
    uint8_t flags2;     // Additional flags
    uint8_t numSV;      // Satellites used
    int32_t lon;        // [deg * 1e-7]
    int32_t lat;        // [deg * 1e-7]
    int32_t height;     // [mm] Above ellipsoid
    int32_t hMSL;       // [mm] Above sea level
    uint32_t hAcc;      // [mm] Horizontal accuracy
    uint32_t vAcc;      // [mm] Vertical accuracy
    int32_t velN;       // [mm/s] NED north velocity
    int32_t velE;       // [mm/s] NED east velocity
    int32_t velD;       // [mm/s] NED down velocity
    int32_t gSpeed;     // [mm/s] Ground speed
    int32_t headMot;    // [deg * 1e-5] Heading
    uint32_t sAcc;      // [mm/s] Speed accuracy
    uint32_t headAcc;   // [deg * 1e-5] Heading accuracy
    uint16_t pDOP;      // [* 0.01] Position DOP
    uint16_t flags3;    // Additional flags
    uint8_t reserved1[4];
    int32_t headVeh;    // [deg * 1e-5] Vehicle heading
    int16_t magDec;     // [deg * 1e-2] Magnetic declination
    uint16_t magAcc;    // [deg * 1e-2] Declination accuracy
} UBX_NAV_PVT_data_t;
```

**Total:** 92 bytes de payload  
**Mensaje completo:** 100 bytes (header + payload + checksum)

---

## 🔍 Conversiones de Unidades

```cpp
// Latitud/Longitud a grados decimales
double lat_deg = pvtData.lat * 1e-7;
double lon_deg = pvtData.lon * 1e-7;

// Altura a metros
double height_m = pvtData.height / 1000.0;
double hMSL_m = pvtData.hMSL / 1000.0;

// Velocidad a m/s
double speed_ms = pvtData.gSpeed / 1000.0;

// Rumbo a grados
double heading_deg = pvtData.headMot * 1e-5;

// Precisión a metros
double hAcc_m = pvtData.hAcc / 1000.0;
double vAcc_m = pvtData.vAcc / 1000.0;

// PDOP
double pdop = pvtData.pDOP * 0.01;
```

---

## 🛠️ Debugging Tips

### Verificar comunicación I2C:
```cpp
// Leer registro 0xFD (bytes disponibles)
uint8_t bytesAvail[2];
I2CResult result = i2cBus->memRead(0x42, 0xFD, I2C_MEMADD_SIZE_8BIT, bytesAvail, 2, 100);
uint16_t available = (bytesAvail[1] << 8) | bytesAvail[0];
printf("Bytes disponibles: %d\r\n", available & 0x7FFF);
```

### Verificar mensaje UBX recibido:
```cpp
// Dump del buffer en hexadecimal
for (int i = 0; i < 100; i++) {
    printf("%02X ", buffer[i]);
    if ((i+1) % 16 == 0) printf("\r\n");
}
```

### Validar checksum manualmente:
```cpp
uint8_t ck_a = 0, ck_b = 0;
for (int i = 2; i < 98; i++) { // Class hasta último byte de payload
    ck_a += buffer[i];
    ck_b += ck_a;
}
printf("CK_A calc: %02X recv: %02X\r\n", ck_a, buffer[98]);
printf("CK_B calc: %02X recv: %02X\r\n", ck_b, buffer[99]);
```

---

## 📚 Referencias

- **u-blox Protocol Specification:** UBX-NAV-PVT (Class 0x01, ID 0x07)
- **SparkFun u-blox GNSS v3 Library:** Implementación de referencia estudiada
- **SAM-M10Q Datasheet:** Registros I2C 0xFD, 0xFE, 0xFF
- **I2C Buffer Size:** MAX_MESSAGE_PAYLOAD_SIZE = 64 bytes

---

## ✅ Testing Checklist

- [ ] `getPVT()` retorna `true` con fix válido
- [ ] Latitud/longitud en rango esperado
- [ ] `numSV` > 4 satélites
- [ ] `fixType` == 3 (fix 3D)
- [ ] Fecha/hora UTC correctas
- [ ] Checksum validado en todos los mensajes
- [ ] Sin errores I2C en logs
- [ ] Timeout funciona correctamente (sin GPS)
- [ ] Funciona con ambas implementaciones (`receivePVT` y `receivePVTValidateOption`)

---

**Última actualización:** Noviembre 2025  
**Autor:** Implementación para TPP-IntelliFence STM32WL55JC1
