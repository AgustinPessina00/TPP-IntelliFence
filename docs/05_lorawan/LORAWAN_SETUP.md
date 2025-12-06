# Configuración LoRaWAN End Node - STM32WL55JC1

Esta guía describe cómo configurar el STM32WL55JC1 como end node LoRaWAN para conectarse al gateway ChirpStack.

## Arquitectura del Sistema

```
STM32WL55JC1 (End Node)
    ├── CM4: Aplicación principal
    │   ├── loraTask: Maneja envío de datos GPS
    │   ├── fsmTask: Máquina de estados
    │   └── lora_app.c: Stack LoRaWAN
    └── CM0+: Radio LoRa (hardware)

          ↓ LoRaWAN AU915

Raspberry Pi 4 Gateway
    ├── RAK5146 Concentrator (SPI)
    ├── rak_common_for_gateway (packet forwarder)
    └── ChirpStack (docker)
```

## Configuración del Device (STM32WL55)

### 1. Configurar las Keys de LoRaWAN

Edita el archivo `CM0PLUS/LoRaWAN/App/se-identity.h`:

```c
/*!
 * End-device IEEE EUI (big endian)
 * Cuando se pone 00,00,00,00,00,00,00,00, se genera automáticamente desde el MCU
 */
#define LORAWAN_DEVICE_EUI    00,00,00,00,00,00,00,00  // Se auto-genera

/*!
 * App/Join server IEEE EUI (big endian)
 * Para ChirpStack, normalmente se usa todo ceros
 */
#define LORAWAN_JOIN_EUI      00,00,00,00,00,00,00,00

/*!
 * Application root key (16 bytes)
 * IMPORTANTE: Esta key debe coincidir con ChirpStack
 */
#define LORAWAN_APP_KEY       2B,7E,15,16,28,AE,D2,A6,AB,F7,15,88,09,CF,4F,3C
```

**IMPORTANTE:** Cuando `LORAWAN_DEVICE_EUI` es todo ceros, el firmware genera un DevEUI único basado en el chip UID del STM32. Debes obtener este valor en tiempo de ejecución para registrarlo en ChirpStack.

### 2. Configurar la Región

En `CM4/LoRaWAN/App/lora_app.h`:

```c
/* LoraWAN application configuration */
#define ACTIVE_REGION    LORAMAC_REGION_AU915  // Australia
```

Regiones disponibles:
- `LORAMAC_REGION_AU915` - Australia
- `LORAMAC_REGION_US915` - USA
- `LORAMAC_REGION_EU868` - Europa
- `LORAMAC_REGION_AS923` - Asia

### 3. Verificar Parámetros de Transmisión

En `CM4/LoRaWAN/App/lora_app.h`:

```c
// Intervalo de envío de datos (en milisegundos)
#define APP_TX_DUTYCYCLE                            10000  // 10 segundos

// Puerto de aplicación
#define LORAWAN_USER_APP_PORT                       2

// Tipo de activación
#define LORAWAN_DEFAULT_ACTIVATION_TYPE             ACTIVATION_TYPE_OTAA

// Data Rate (DR_0 es el más robusto pero más lento)
#define LORAWAN_DEFAULT_DATA_RATE                   DR_0

// Potencia de transmisión (TX_POWER_0 es máxima)
#define LORAWAN_DEFAULT_TX_POWER                    TX_POWER_0
```

### 4. Obtener el DevEUI del Device

Cuando flashees el código, el device generará un DevEUI automáticamente basado en el chip UID. Para obtenerlo:

**Opción A: Por UART/Debug**
El firmware imprimirá el DevEUI al iniciar. Busca en los logs:
```
[LORA] DevEUI: 00:80:E1:10:00:XX:XX:XX
```

**Opción B: Leer desde código**
Agrega esto temporalmente en `lora_app.c` dentro de `LoRaWAN_Init()`:

```c
uint8_t devEui[8];
LmHandlerGetDevEUI(devEui);
APP_LOG(TS_OFF, VLEVEL_M, "DevEUI: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
        devEui[0], devEui[1], devEui[2], devEui[3],
        devEui[4], devEui[5], devEui[6], devEui[7]);
```

## Configuración de ChirpStack

### 1. Acceder a ChirpStack Web UI

Abre el navegador y ve a:
```
http://<IP_RASPBERRY_PI>:8080
```

Login por defecto:
- Usuario: `admin`
- Password: `admin`

### 2. Crear una Aplicación

1. Ve a **Applications** → **Add Application**
2. Completa:
   - **Name:** `IntelliFence`
   - **Description:** `Virtual fence for cattle`
   - **Service Profile:** Selecciona el existente
3. Click **Submit**

### 3. Agregar el Device

1. Dentro de la aplicación, ve a **Devices** → **Create**
2. Completa:
   - **Device name:** `Cow-Collar-001`
   - **Device description:** `STM32WL55 Collar`
   - **Device EUI:** El DevEUI obtenido del device (ej: `0080E110000XXXXX`)
   - **Device Profile:** Selecciona o crea uno para Class A, OTAA, AU915
3. Click **Submit**

### 4. Configurar las Keys del Device

1. Dentro del device recién creado, ve a la pestaña **Keys (OTAA)**
2. Completa:
   - **Application key:** `2B7E151628AED2A6ABF7158809CF4F3C` (el mismo que `LORAWAN_APP_KEY`)
   - **Network key:** Dejar en blanco o todo ceros (LoRaWAN 1.0.x)
3. Click **Submit**

### 5. Configurar el Device Profile (si es necesario)

Si no existe un Device Profile adecuado:

1. Ve a **Device Profiles** → **Create**
2. Completa:
   - **Name:** `Class-A-OTAA-AU915`
   - **Region:** `AU915`
   - **MAC version:** `1.0.3` (compatible con STM32WL)
   - **Regional parameters revision:** `A`
   - **Max EIRP:** `30`
   - **Uplink interval:** `10` (segundos)
   - **Supports OTAA:** ✅ Marcado
   - **Supports Class B:** ❌
   - **Supports Class C:** ❌
3. Click **Submit**

## Configuración del Gateway (RAK5146)

### Verificar el Gateway está conectado

```bash
ssh pi@<IP_RASPBERRY_PI>
sudo systemctl status ttn-gateway
```

### Verificar configuración regional

Edita `/opt/ttn-gateway/packet_forwarder/lora_pkt_fwd/global_conf.json`:

```json
{
  "SX130x_conf": {
    "com_type": "SPI",
    "com_path": "/dev/spidev0.0",
    "lorawan_public": true,
    "clksrc": 0,
    "antenna_gain": 0,
    "full_duplex": false,
    "freq_hz": 923300000,  // AU915 - Verifica tu subband
    ...
  }
}
```

Para AU915, los canales típicos son:
- Subband 2 (recomendado): 916.8 - 918.2 MHz
- Subband 1: 915.0 - 916.6 MHz

Reinicia el servicio:
```bash
sudo systemctl restart ttn-gateway
```

### Verificar logs del gateway

```bash
sudo journalctl -u ttn-gateway -f
```

Deberías ver mensajes de uplink cuando el device envíe datos.

## Formato del Payload

El device envía un payload de **17 bytes**:

| Offset | Tamaño | Campo      | Tipo   | Descripción                    |
|--------|--------|------------|--------|--------------------------------|
| 0      | 8      | Latitude   | double | Latitud GPS (IEEE 754)         |
| 8      | 8      | Longitude  | double | Longitud GPS (IEEE 754)        |
| 16     | 1      | Zone       | uint8  | 0=GREEN, 1=YELLOW, 2=ORANGE, 3=RED, 4=BLACK |

### Decoder para ChirpStack

En ChirpStack, puedes agregar un decoder en JavaScript:

1. Ve a **Device Profile** → **Codec** → **Payload codec**
2. Selecciona **Custom JavaScript codec**
3. Agrega este código:

```javascript
function Decode(port, bytes) {
    if (port !== 2 || bytes.length !== 17) {
        return {
            errors: ["Invalid payload"]
        };
    }
    
    // Decodificar latitude (bytes 0-7)
    var latBytes = bytes.slice(0, 8);
    var latView = new DataView(new ArrayBuffer(8));
    for (var i = 0; i < 8; i++) {
        latView.setUint8(i, latBytes[i]);
    }
    var latitude = latView.getFloat64(0, true); // little endian
    
    // Decodificar longitude (bytes 8-15)
    var lonBytes = bytes.slice(8, 16);
    var lonView = new DataView(new ArrayBuffer(8));
    for (var i = 0; i < 8; i++) {
        lonView.setUint8(i, lonBytes[i]);
    }
    var longitude = lonView.getFloat64(0, true); // little endian
    
    // Decodificar zone (byte 16)
    var zone = bytes[16];
    var zoneNames = ["GREEN", "YELLOW", "ORANGE", "RED", "BLACK"];
    
    return {
        latitude: latitude,
        longitude: longitude,
        zone: zone,
        zone_name: zoneNames[zone] || "UNKNOWN"
    };
}
```

## Uso desde el Código

### Enviar datos GPS desde fsmTask u otro módulo

```c
#include "threads/loraTask.h"

// Preparar datos
LoraGpsData_t gpsData;
gpsData.latitude = -34.603722;  // Buenos Aires ejemplo
gpsData.longitude = -58.381592;
gpsData.zone = GREEN_ZONE;
gpsData.distance = 50.5;

// Enviar a LoRa
if (sendGpsDataToLora(&gpsData) == HAL_OK) {
    RTOS_LOG_INFO("Datos GPS enviados a LoRa\n");
}
```

### Integración con fsmTask existente

Puedes integrar esto en `fsmTask.cpp` después de obtener la posición GPS:

```c
// En STARTUP_ROUTINE_WAIT_POSITION o en INITIALIZE_WAIT_POSITION
if (updatePosition(cow, *msgReceived) == HAL_OK) {
    // Preparar y enviar datos por LoRa
    LoraGpsData_t loraData;
    Position pos = cow.getPosition();
    loraData.latitude = pos.latitude;
    loraData.longitude = pos.longitude;
    loraData.zone = cow.getCurrentZone();
    loraData.distance = cow.getDistanceToLimit();
    
    sendGpsDataToLora(&loraData);
    
    *state = SIGUIENTE_ESTADO;
}
```

## Verificación y Debugging

### 1. Monitor Serial (UART)

Conecta un USB-UART al STM32WL55 y monitorea los logs:

```
[LORA_TASK] Inicializando LoRaWAN...
[LORA_TASK] LoRaWAN iniciado, esperando JOIN...
JOIN SUCCESS!
[LORA_TASK] JOIN exitoso! Red LoRaWAN conectada
[LORA_TASK] Datos GPS recibidos: lat=-34.603722, lon=-58.381592, zone=0, dist=50.50
Sending GPS data: 17 bytes
```

### 2. ChirpStack Dashboard

En ChirpStack, ve a **Device** → **LoRaWAN frames** para ver:
- **JoinRequest:** El device intenta conectarse
- **JoinAccept:** El servidor acepta la conexión
- **Uplink:** Datos enviados por el device
- **Downlink:** Comandos enviados al device (si es necesario)

### 3. Errores Comunes

| Error | Causa | Solución |
|-------|-------|----------|
| No hay JOIN | Gateway no recibe | Verificar distancia, antena, logs del gateway |
| JOIN pero sin Uplink | Keys incorrectas | Verificar `LORAWAN_APP_KEY` coincide |
| Payload vacío | No hay datos GPS | Verificar que fsmTask llame a `sendGpsDataToLora()` |
| RX timeout | Duty cycle | Esperar más tiempo entre mensajes |

## Prueba Básica

### Código de prueba en fsmTask

Agrega esto temporalmente para probar el envío:

```c
// En fsmTask(), después de la inicialización
static uint32_t testCounter = 0;

if (testCounter % 100 == 0) {  // Cada 10 segundos aprox
    LoraGpsData_t testData;
    testData.latitude = -34.603722;
    testData.longitude = -58.381592;
    testData.zone = testCounter % 5;  // Cicla entre zonas
    testData.distance = 50.0f;
    
    sendGpsDataToLora(&testData);
    RTOS_LOG_INFO("[TEST] Enviando datos de prueba #%lu\n", testCounter / 100);
}
testCounter++;
```

## Siguiente Paso: Producción

Una vez que el sistema funcione:

1. **Ajustar el duty cycle** en `APP_TX_DUTYCYCLE` según tus necesidades
2. **Implementar downlink** para recibir nuevos fences
3. **Optimizar consumo** usando modos de bajo consumo
4. **Agregar confirmación** cambiando a `LORAMAC_HANDLER_CONFIRMED_MSG`

## Referencias

- [STM32WL LoRaWAN Documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32wl-series.html)
- [ChirpStack Documentation](https://www.chirpstack.io/docs/)
- [LoRaWAN Regional Parameters](https://lora-alliance.org/resource_hub/rp2-101-lorawan-regional-parameters-2/)
