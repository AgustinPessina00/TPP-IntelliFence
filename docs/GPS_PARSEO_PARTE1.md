# Explicación Detallada del Parseo de Datos en `getPVT()` - PARTE 1

## Introducción

Este documento explica **paso por paso** cómo funciona el proceso completo desde que llamas `getPVT()` hasta que los datos del GPS están parseados en la estructura `UBX_NAV_PVT_t`.

El módulo GPS utilizado es el **SAM-M10Q** de u-blox, que se comunica mediante el protocolo **UBX** sobre **I2C**.

---

## Arquitectura General

```
┌─────────────────────────────────────────────────────────────────┐
│                    FLUJO COMPLETO DE getPVT()                   │
└─────────────────────────────────────────────────────────────────┘

  1. getPVT()
        ↓
  2. sendCommand()
        ↓
  3. sendI2cCommand() ──→ GPS SAM-M10Q
        ↓                      │
  4. waitForNoACKResponse()    │
        ↓                      │
  5. checkUbloxI2C() ←─────────┘ (GPS envía 100 bytes)
        ↓
  6. process() (detecta inicio UBX)
        ↓
  7. processUBX() (construye payload)
        ↓
  8. processUBXpacket() (PARSEA datos)
        ↓
  9. extractLong(), extractByte(), etc.
        ↓
  10. packetUBXNAVPVT->data (DATOS LISTOS)
```

---

## 1. INICIO: Función `getPVT()` (línea 11485)

### Código

```cpp
bool DevUBLOXGNSS::getPVT(uint16_t maxWait)
{
  // Verifica que existe memoria para los datos PVT
  if (packetUBXNAVPVT == nullptr)
    initPacketUBXNAVPVT();        // Asigna memoria si no existe
  if (packetUBXNAVPVT == nullptr) // Bail if the RAM allocation failed
    return (false);

  // Verifica si el GPS está en modo automático
  if (packetUBXNAVPVT->automaticFlags.flags.bits.automatic && 
      packetUBXNAVPVT->automaticFlags.flags.bits.implicitUpdate)
  {
    // El GPS reporta automáticamente, solo verificamos si hay datos nuevos
    checkUbloxInternal(&packetCfg, 0, 0);
    return packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.all;
  }
  else
  {
    // El GPS NO reporta automáticamente, debemos hacer polling explícito
    
    // Prepara el paquete de solicitud
    packetCfg.cls = UBX_CLASS_NAV;     // Clase: 0x01 (Navigation)
    packetCfg.id = UBX_NAV_PVT;        // ID: 0x07 (Position/Velocity/Time)
    packetCfg.len = 0;                 // Sin payload (es una solicitud)
    packetCfg.startingSpot = 0;
    
    // Envía el comando y espera respuesta
    sfe_ublox_status_e retVal = sendCommand(&packetCfg, maxWait);
    
    if (retVal == SFE_UBLOX_STATUS_DATA_RECEIVED)
      return (true);
    
    if (retVal == SFE_UBLOX_STATUS_DATA_OVERWRITTEN)
      return (true);
    
    return (false);
  }
}
```

### ¿Qué hace?

1. **Verifica memoria**: Asegura que `packetUBXNAVPVT` esté inicializado
2. **Prepara solicitud UBX**: 
   - Class = 0x01 (NAV)
   - ID = 0x07 (PVT)
   - Length = 0 (sin payload, solo solicitud)
3. **Envía comando**: Llama a `sendCommand()` para enviar la solicitud al GPS
4. **Retorna**: `true` si recibió datos válidos, `false` en caso contrario

---

## 2. ENVÍO DEL COMANDO: `sendCommand()` (línea 5246)

### Código

```cpp
sfe_ublox_status_e DevUBLOXGNSS::sendCommand(ubxPacket *outgoingUBX, 
                                             uint16_t maxWait, 
                                             bool expectACKonly)
{
  if (!lock())
    return SFE_UBLOX_STATUS_FAIL;

  sfe_ublox_status_e retVal = SFE_UBLOX_STATUS_SUCCESS;

  // Calcula el checksum del paquete
  calcChecksum(outgoingUBX); // Sets checksumA and B bytes of the packet

  #ifndef SFE_UBLOX_REDUCED_PROG_MEM
  if (_printDebug == true)
  {
    _debugSerial.print(F("\nSending: "));
    printPacket(outgoingUBX, true);
  }
  #endif

  // Envía según el tipo de comunicación
  if (_commType == COMM_TYPE_I2C)
  {
    retVal = sendI2cCommand(outgoingUBX);
    if (retVal != SFE_UBLOX_STATUS_SUCCESS)
    {
      unlock();
      return retVal;
    }
  }
  else if (_commType == COMM_TYPE_SERIAL)
  {
    sendSerialCommand(outgoingUBX);
  }
  else if (_commType == COMM_TYPE_SPI)
  {
    sendSpiCommand(outgoingUBX);
  }

  unlock();

  if (maxWait > 0)
  {
    // Dependiendo de lo que enviamos, buscamos ACK o datos
    if ((outgoingUBX->cls == UBX_CLASS_CFG) || (expectACKonly == true))
    {
      // Para comandos de configuración, esperamos ACK
      retVal = waitForACKResponse(outgoingUBX, outgoingUBX->cls, 
                                   outgoingUBX->id, maxWait);
    }
    else
    {
      // Para comandos de lectura (como NAV-PVT), esperamos DATOS
      retVal = waitForNoACKResponse(outgoingUBX, outgoingUBX->cls, 
                                     outgoingUBX->id, maxWait);
    }
  }

  return retVal;
}
```

### ¿Qué hace?

1. **Calcula checksum**: Genera checksumA y checksumB según el algoritmo UBX
2. **Envía por I2C**: Llama a `sendI2cCommand()` para transmitir al GPS
3. **Espera respuesta**: Como es `UBX_CLASS_NAV` (no CFG), llama a `waitForNoACKResponse()`

### Diferencia entre ACK y NoACK

- **ACK**: Para comandos de configuración (UBX-CFG-*), el GPS responde con UBX-ACK-ACK o UBX-ACK-NACK
- **NoACK**: Para comandos de lectura (UBX-NAV-*, UBX-MON-*, etc.), el GPS responde con los datos solicitados

---

## 3. ENVÍO POR I2C: `sendI2cCommand()` (línea 5327)

### Código

```cpp
sfe_ublox_status_e DevUBLOXGNSS::sendI2cCommand(ubxPacket *outgoingUBX)
{
  // Del manual de integración u-blox:
  // "El receptor no proporciona acceso de escritura excepto para escribir mensajes 
  //  UBX y NMEA al receptor, como configuración o datos de asistencia. Por lo tanto, 
  //  el conjunto de registros mencionado en la sección de Acceso de Lectura no es 
  //  escribible. [...] El número de bytes de datos debe ser al menos 2 para 
  //  distinguir adecuadamente del acceso de escritura para establecer el contador 
  //  de direcciones en accesos de lectura aleatorios."
  
  // Estructura del mensaje UBX:
  // [0xB5] [0x62] [cls] [id] [len LSB] [len MSB] [payload] [checksumA] [checksumB]
  //   μ      b
  
  uint16_t bytesLeftToSend = outgoingUBX->len;
  uint16_t startSpot = 0;

  // ¿Podemos enviar todo en una sola transacción I2C?
  if (bytesLeftToSend + 8 <= i2cTransactionSize)
  {
    // SÍ: envío en una sola transacción
    uint8_t buf[i2cTransactionSize];
    buf[0] = UBX_SYNCH_1;             // 0xB5 (μ - micro)
    buf[1] = UBX_SYNCH_2;             // 0x62 (b)
    buf[2] = outgoingUBX->cls;        // 0x01 (UBX_CLASS_NAV)
    buf[3] = outgoingUBX->id;         // 0x07 (UBX_NAV_PVT)
    buf[4] = outgoingUBX->len & 0xFF; // 0x00 (Length LSB)
    buf[5] = outgoingUBX->len >> 8;   // 0x00 (Length MSB)
    
    // Copia el payload (en este caso, vacío)
    uint16_t i = 0;
    for (; i < outgoingUBX->len; i++)
      buf[i + 6] = outgoingUBX->payload[startSpot + i];
    
    buf[i + 6] = outgoingUBX->checksumA;
    buf[i + 7] = outgoingUBX->checksumB;

    // Envía al GPS (dirección 0x42 por defecto)
    if (writeBytes(buf, bytesLeftToSend + 8) != bytesLeftToSend + 8)
      return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE);
  }
  else
  {
    // NO: envío en múltiples transacciones
    // [código para transacciones múltiples...]
  }

  return (SFE_UBLOX_STATUS_SUCCESS);
}
```

### Mensaje enviado al GPS

Para solicitar PVT, el mensaje es:

```
Byte:  0    1    2    3    4    5    6    7
Data: B5   62   01   07   00   00   08   19
      └─┬─┘ └─┬─┘ └─┬─┘ └─┬─┘ └─┬─┘
      Sync  Sync Cls  ID   Len  CRC
```

**Interpretación:**
- **B5 62**: Caracteres de sincronización UBX
- **01**: Class = NAV (Navigation)
- **07**: ID = PVT (Position/Velocity/Time)
- **00 00**: Length = 0 (sin payload)
- **08 19**: Checksum calculado

### Comunicación I2C

El mensaje se envía al **registro 0xFF** del GPS en la dirección I2C **0x42**.

---

## 4. RESPUESTA DEL GPS SAM-M10Q

### ¿Qué hace el GPS?

Al recibir la solicitud UBX-NAV-PVT, el GPS:

1. **Procesa la solicitud**: Verifica que la clase y el ID sean válidos
2. **Recopila datos**: Obtiene posición, velocidad, tiempo, DOP, etc.
3. **Construye respuesta**: Arma un mensaje UBX-NAV-PVT de 100 bytes totales:
   - 6 bytes de header (B5 62 01 07 5C 00)
   - 92 bytes de payload (datos PVT)
   - 2 bytes de checksum

### Estructura de la respuesta

```
Offset  Bytes  Description
------  -----  -----------
  0       2    Sync chars (B5 62)
  2       1    Class (01 = NAV)
  3       1    ID (07 = PVT)
  4       2    Length (5C 00 = 92 bytes en little-endian)
  6      92    Payload (datos PVT)
 98       2    Checksum (A y B)
------  -----
Total: 100 bytes
```

### Contenido del payload (92 bytes)

Según el **SAM-M10Q Integration Manual** y el **Interface Description**:

```
Offset  Type    Name        Units           Description
------  ------  ----------  --------------  ---------------------------
  0     U4      iTOW        ms              GPS time of week
  4     U2      year        y               Year (UTC)
  6     U1      month       month           Month (UTC) 1..12
  7     U1      day         day             Day of month (UTC) 1..31
  8     U1      hour        h               Hour of day (UTC) 0..23
  9     U1      min         min             Minute of hour (UTC) 0..59
 10     U1      sec         s               Second of minute (UTC) 0..60
 11     X1      valid       -               Validity flags
 12     U4      tAcc        ns              Time accuracy estimate
 16     I4      nano        ns              Fraction of second (-1e9..1e9)
 20     U1      fixType     -               GNSS fix type (0=no fix, 3=3D fix)
 21     X1      flags       -               Fix status flags
 22     X1      flags2      -               Additional flags
 23     U1      numSV       -               Number of satellites used
 24     I4      lon         deg*1e-7        Longitude
 28     I4      lat         deg*1e-7        Latitude
 32     I4      height      mm              Height above ellipsoid
 36     I4      hMSL        mm              Height above mean sea level
 40     U4      hAcc        mm              Horizontal accuracy estimate
 44     U4      vAcc        mm              Vertical accuracy estimate
 48     I4      velN        mm/s            NED north velocity
 52     I4      velE        mm/s            NED east velocity
 56     I4      velD        mm/s            NED down velocity
 60     I4      gSpeed      mm/s            Ground speed (2-D)
 64     I4      headMot     deg*1e-5        Heading of motion (2-D)
 68     U4      sAcc        mm/s            Speed accuracy estimate
 72     U4      headAcc     deg*1e-5        Heading accuracy estimate
 76     U2      pDOP        *0.01           Position DOP
 78     X2      flags3      -               Additional flags
 80     U4      reserved1   -               Reserved
 84     I4      headVeh     deg*1e-5        Heading of vehicle (2-D)
 88     I2      magDec      deg*1e-2        Magnetic declination
 90     U2      magAcc      deg*1e-2        Magnetic declination accuracy
```

**Tipos de datos:**
- **U1**: Unsigned 1 byte (uint8_t)
- **U2**: Unsigned 2 bytes (uint16_t) little-endian
- **U4**: Unsigned 4 bytes (uint32_t) little-endian
- **I1**: Signed 1 byte (int8_t)
- **I2**: Signed 2 bytes (int16_t) little-endian
- **I4**: Signed 4 bytes (int32_t) little-endian
- **X1/X2**: Bitfield de 1/2 bytes

### Disponibilidad en registros I2C

El GPS almacena la respuesta en un buffer FIFO interno. Para leerla:

1. **Leer registros 0xFD y 0xFE**: Devuelven el número de bytes disponibles (little-endian)
   - Ejemplo: `FD=64, FE=00` → 100 bytes disponibles
2. **Leer desde registro 0xFF**: Devuelve los bytes del buffer FIFO secuencialmente

---

## 5. ESPERA DE RESPUESTA: `waitForNoACKResponse()` (línea 5858)

### Código

```cpp
sfe_ublox_status_e DevUBLOXGNSS::waitForNoACKResponse(ubxPacket *outgoingUBX, 
                                                       uint8_t requestedClass, 
                                                       uint8_t requestedID, 
                                                       uint16_t maxTime)
{
  // Inicializa los flags de validez
  outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAuto.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  
  outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAuto.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

  unsigned long startTime = millis();
  while (millis() - startTime < maxTime)
  {
    // Verifica si hay datos nuevos del GPS
    if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true)
    {
      // ¿Los datos recibidos coinciden con lo que pedimos?
      if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && 
          (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && 
          (outgoingUBX->cls == requestedClass) && 
          (outgoingUBX->id == requestedID))
      {
        return (SFE_UBLOX_STATUS_DATA_RECEIVED); // ¡Datos válidos recibidos!
      }

      // Si la clase y el ID coincidían pero ya no, significa que los datos
      // fueron o están siendo sobrescritos por otro paquete (ej: PVT automático)
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && 
               ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
      {
        return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN);
      }

      // Si recibimos datos válidos pero no son los que buscamos
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && 
               (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID))
      {
        // Esto puede ser un mensaje automático (ej: otro PVT que llegó antes)
        // Continuamos esperando
      }

      // Si la clase y el ID coinciden pero el CRC falló
      else if (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID)
      {
        return (SFE_UBLOX_STATUS_CRC_FAIL);
      }
    }

    delay(1); // Permite que un RTOS tome control
  }

  return (SFE_UBLOX_STATUS_TIMEOUT);
}
```

### ¿Qué hace?

1. **Inicializa flags**: Marca todos los paquetes como "no definidos"
2. **Loop de espera**: Hasta `maxTime` milisegundos (por defecto 1100 ms)
3. **Verifica datos**: Llama repetidamente a `checkUbloxInternal()` para ver si hay datos nuevos
4. **Valida coincidencia**: Verifica que:
   - El paquete sea válido (checksum correcto)
   - La clase y el ID coincidan con lo solicitado
5. **Retorna estado**: 
   - `DATA_RECEIVED`: Éxito
   - `DATA_OVERWRITTEN`: Los datos fueron reemplazados
   - `CRC_FAIL`: Error de checksum
   - `TIMEOUT`: No se recibió respuesta

---

**Continúa en GPS_PARSEO_PARTE2.md**
