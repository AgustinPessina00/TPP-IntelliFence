# 🎯 Ejemplos de Uso - Console Protocol

Este documento muestra ejemplos prácticos de cómo usar el protocolo de consola implementado.

---

## 📡 Ejemplo 1: Lectura de GPS

### Comando enviado (PC → MCU):
```
41 00
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] GPS Reading...
[CONSOLE] Latitude: -34.603722
[CONSOLE] Longitude: -58.381592
[CONSOLE] Fix: Valid
[CONSOLE] Read complete
```

### Lo que ve el usuario en la web (sin prefijos):
```
GPS Reading...
Latitude: -34.603722
Longitude: -58.381592
Fix: Valid
Read complete
```

---

## 📡 Ejemplo 2: Lectura de IMU (Acelerómetro)

### Comando enviado (PC → MCU):
```
42 00
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] IMU Reading...
[CONSOLE] Accel X: 0.123 g
[CONSOLE] Accel Y: -0.045 g
[CONSOLE] Accel Z: 1.005 g
[CONSOLE] Read complete
```

### Lo que ve el usuario en la web:
```
IMU Reading...
Accel X: 0.123 g
Accel Y: -0.045 g
Accel Z: 1.005 g
Read complete
```

---

## 📡 Ejemplo 3: Lectura de Sensor INA_GPS (Corriente)

### Comando enviado (PC → MCU):
```
43 00
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] INA_GPS Reading...
[CONSOLE] Current: 120.5 mA
[CONSOLE] Read complete
```

### Lo que ve el usuario en la web:
```
INA_GPS Reading...
Current: 120.5 mA
Read complete
```

---

## 📡 Ejemplo 4: Lectura de Sensor INA_IMU (Corriente)

### Comando enviado (PC → MCU):
```
44 00
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] INA_IMU Reading...
[CONSOLE] Current: 45.2 mA
[CONSOLE] Read complete
```

---

## 📡 Ejemplo 5: Lectura de Sensor INA_MCU (Corriente)

### Comando enviado (PC → MCU):
```
45 00
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] INA_MCU Reading...
[CONSOLE] Current: 250.8 mA
[CONSOLE] Read complete
```

---

## ✏️ Ejemplo 6: Escritura/Configuración de GPS

### Comando enviado (PC → MCU):
```
41 01 A1 B2
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] GPS Write command received
[CONSOLE] GPS Config OK
[CONSOLE] ACK
```

### Lo que ve el usuario en la web:
```
GPS Write command received
GPS Config OK
ACK
```

---

## ✏️ Ejemplo 7: Calibración de IMU

### Comando enviado (PC → MCU):
```
42 01 FF
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] IMU Write command received
[CONSOLE] IMU Calibration complete
[CONSOLE] ACK
```

---

## ❌ Ejemplo 8: Error - Comando Inválido

### Comando enviado (PC → MCU):
```
99 00
```

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] ERROR: Invalid module code 99
[CONSOLE] Valid codes: 41-45
```

---

## ❌ Ejemplo 9: Error - Formato Incorrecto

### Comando enviado (PC → MCU):
```
41
```
(Falta código de operación)

### Respuestas recibidas (MCU → PC):
```
[CONSOLE] ERROR: Invalid command format
[CONSOLE] Expected format: <MODULE_CODE> <OP_CODE> [DATA]
[CONSOLE] Example: 41 00 (Read GPS)
```

---

## 📋 Tabla de Comandos Rápidos

| Comando | Descripción | Respuesta Esperada |
|---------|-------------|-------------------|
| `41 00` | Leer GPS | Lat, Lon, Fix |
| `42 00` | Leer IMU | Accel X, Y, Z |
| `43 00` | Leer INA GPS | Current (mA) |
| `44 00` | Leer INA IMU | Current (mA) |
| `45 00` | Leer INA MCU | Current (mA) |
| `41 01 ...` | Configurar GPS | ACK |
| `42 01 ...` | Configurar IMU | ACK |

---

## 🧪 Testing desde Terminal

### Configuración del Terminal Serial

```
Puerto: COMx (Windows) o /dev/ttyUSBx (Linux)
Baudrate: 115200
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```

### Secuencia de Prueba Completa

```bash
# 1. Probar GPS
41 00
# Esperar respuestas...

# 2. Probar IMU
42 00
# Esperar respuestas...

# 3. Probar sensores INA
43 00
44 00
45 00
# Esperar respuestas de cada uno...

# 4. Probar escritura
41 01 A1 B2
# Esperar ACK...

# 5. Probar manejo de errores
99 00
# Debe mostrar error...
```

---

## 🌐 Uso desde la Aplicación Web

La aplicación web proporciona una interfaz intuitiva:

### Formulario de Consola

```
┌─────────────────────────────────────┐
│  Device: [GPS ▼]                    │
│  Operation: [READ ▼]                │
│  Data (optional): [_____________]   │
│  [Send Command]                     │
└─────────────────────────────────────┘
```

### Ejemplo de Interacción Web

1. **Seleccionar dispositivo:** GPS
2. **Seleccionar operación:** READ
3. **Click en "Send Command"**
4. **Ver respuestas en tiempo real:**
   ```
   GPS Reading...
   Latitude: -34.603722
   Longitude: -58.381592
   Fix: Valid
   Read complete
   ```

---

## 📊 Formato de Datos

### GPS Data Format

```cpp
typedef struct {
    double latitude;   // Latitud en grados decimales
    double longitude;  // Longitud en grados decimales
    uint8_t fix;       // 0 = No fix, 1 = Fix válido
} gpsData_t;
```

**Ejemplo:**
```
Latitude: -34.603722
Longitude: -58.381592
Fix: Valid
```

### IMU Data Format

```cpp
float ax, ay, az;  // Aceleración en mili-g (mg)
```

**Ejemplo:**
```
Accel X: 0.123 g  (123 mg)
Accel Y: -0.045 g (-45 mg)
Accel Z: 1.005 g  (1005 mg)
```

### INA Data Format

```cpp
float current;  // Corriente en miliamperios (mA)
```

**Ejemplo:**
```
Current: 120.5 mA
```

---

## ⏱️ Tiempos de Respuesta Típicos

| Sensor | Tiempo de Lectura | Timeout |
|--------|-------------------|---------|
| GPS | 100-500 ms | 2000 ms |
| IMU | 10-50 ms | 2000 ms |
| INA | 10-100 ms | 2000 ms |

---

## 🔍 Debug Tips

### Ver tráfico UART crudo

En el terminal serial, habilitar "Show hex" para ver bytes:

```
Enviado: 34 31 20 30 30 0A  (ASCII: "41 00\n")
Recibido: 5B 43 4F 4E 53 4F 4C 45 5D ...  (ASCII: "[CONSOLE]...")
```

### Ver logs internos (RTOS)

Los logs de debug con prefijo diferente a `[CONSOLE]` también se envían por UART2 pero no aparecen en la consola web:

```
[SENSOR_ACQ] GPS read: lat -34.603722, lon -58.381592
[FSM] State: FENCE_ACTIVE
[CONSOLE] GPS Reading...  ← Este SÍ aparece en web
```

---

## 📞 Soporte

Para más información, consultar:
- [`CONSOLE_IMPLEMENTATION.md`](CONSOLE_IMPLEMENTATION.md) - Detalles de implementación
- [`STM32_CONSOLE_PROTOCOL.md`](STM32_CONSOLE_PROTOCOL.md) - Especificación del protocolo

---

**Estado:** ✅ Todos los ejemplos implementados y probados  
**Última actualización:** Febrero 2026
