# Explicación Detallada del Parseo de Datos en `getPVT()` - PARTE 2

**Continúa desde GPS_PARSEO_PARTE1.md**

---

## 6. VERIFICACIÓN DE DATOS I2C: `checkUbloxInternal()` → `checkUbloxI2C()` (línea 1163 y 1182)

### Código `checkUbloxInternal()`

```cpp
bool DevUBLOXGNSS::checkUbloxInternal(ubxPacket *incomingUBX, 
                                      uint8_t requestedClass, 
                                      uint8_t requestedID)
{
  if (!lock())
    return false;

  bool ok = false;
  if (_commType == COMM_TYPE_I2C)
    ok = (checkUbloxI2C(incomingUBX, requestedClass, requestedID));
  else if (_commType == COMM_TYPE_SERIAL)
    ok = (checkUbloxSerial(incomingUBX, requestedClass, requestedID));
  else if (_commType == COMM_TYPE_SPI)
    ok = (checkUbloxSpi(incomingUBX, requestedClass, requestedID));

  unlock();

  return ok;
}
```

### Código `checkUbloxI2C()`

```cpp
bool DevUBLOXGNSS::checkUbloxI2C(ubxPacket *incomingUBX, 
                                 uint8_t requestedClass, 
                                 uint8_t requestedID)
{
  // Evita saturar el bus I2C
  if (millis() - lastCheck >= i2cPollingWait)
  {
    // Del manual de integración u-blox:
    // "Hay dos formas de transferencia de lectura DDC. La forma de 'acceso aleatorio'
    //  incluye una dirección de registro periférico y por lo tanto permite leer 
    //  cualquier registro. La segunda forma de 'dirección actual' omite la dirección 
    //  de registro. Si se usa esta segunda forma, entonces se usa un puntero de 
    //  dirección en el receptor para determinar qué registro leer. Este puntero de 
    //  dirección se incrementará después de cada lectura a menos que ya esté 
    //  apuntando al registro 0xFF, el registro direccionable más alto, en cuyo 
    //  caso permanece sin alteraciones."
    
    // Pregunta al GPS cuántos bytes tiene disponibles
    uint16_t bytesAvailable = available(); // Lee registros 0xFD y 0xFE
    
    if (bytesAvailable == 0)
    {
      lastCheck = millis(); // Actualiza timestamp
      return (false);
    }

    // Verifica bit de error no documentado (bit 15)
    // Error raro pero si el bit 15 está en 1, lo limpiamos
    if (bytesAvailable & ((uint16_t)1 << 15))
    {
      bytesAvailable &= ~((uint16_t)1 << 15);
    }

    #ifndef SFE_UBLOX_REDUCED_PROG_MEM
    if (_printDebug == true)
    {
      _debugSerial.print(F("checkUbloxI2C: "));
      _debugSerial.print(bytesAvailable);
      _debugSerial.println(F(" bytes available"));
    }
    #endif

    while (bytesAvailable)
    {
      // Limita a 32 bytes (o el límite del buffer de la plataforma)
      uint16_t bytesToRead = bytesAvailable;
      if (bytesToRead > i2cTransactionSize) // i2cTransactionSize = 32 por defecto
        bytesToRead = i2cTransactionSize;

      // Lee desde el registro 0xFF
      uint8_t buf[i2cTransactionSize];
      uint8_t bytesReturned = readBytes(buf, (uint8_t)bytesToRead);
      
      if ((uint16_t)bytesReturned == bytesToRead)
      {
        // Procesa cada byte recibido
        for (uint16_t x = 0; x < bytesToRead; x++)
        {
          process(buf[x], incomingUBX, requestedClass, requestedID);
        }
      }
      else
      {
        // Error en el bus I2C
        if (_resetCurrentSentenceOnBusError)
          currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;
        return (false);
      }

      bytesAvailable -= bytesToRead;
    }
  }

  return (true);
}
```

### ¿Qué hace?

1. **Verifica tiempo transcurrido**: Solo hace polling cada `i2cPollingWait` ms (100 ms por defecto)
2. **Lee bytes disponibles**: Lee registros 0xFD y 0xFE para saber cuántos bytes tiene el GPS
3. **Lee datos**: Lee desde registro 0xFF en bloques de hasta 32 bytes
4. **Procesa bytes**: Llama a `process()` para cada byte recibido

### Ejemplo de lectura I2C

```
┌─────────────────────────────────────────────────────────────┐
│                  LECTURA I2C DEL GPS                        │
└─────────────────────────────────────────────────────────────┘

1. Leer disponibilidad:
   I2C Write: [0x42] [0xFD]       (solicita registro 0xFD)
   I2C Read:  [0x64] [0x00]       (100 bytes disponibles)

2. Leer datos (primera transacción, 32 bytes):
   I2C Write: [0x42] [0xFF]       (solicita registro 0xFF)
   I2C Read:  [B5 62 01 07 5C 00 ... 32 bytes ...]

3. Leer datos (segunda transacción, 32 bytes):
   I2C Write: [0x42] [0xFF]
   I2C Read:  [... 32 bytes ...]

4. Leer datos (tercera transacción, 32 bytes):
   I2C Write: [0x42] [0xFF]
   I2C Read:  [... 32 bytes ...]

5. Leer datos (cuarta transacción, 4 bytes):
   I2C Write: [0x42] [0xFF]
   I2C Read:  [... 4 bytes ...]

Total: 100 bytes leídos
```

---

## 7. PROCESAMIENTO BYTE POR BYTE: `process()` (línea 1626)

Esta es la función **MUY IMPORTANTE** que identifica el tipo de mensaje y lo rutea al procesador correcto.

### Código (simplificado)

```cpp
void DevUBLOXGNSS::process(uint8_t incoming, 
                           ubxPacket *incomingUBX, 
                           uint8_t requestedClass, 
                           uint8_t requestedID)
{
  // Actualiza storedClass y storedID si requestedClass o requestedID son no-cero
  volatile static uint8_t storedClass = 0;
  volatile static uint8_t storedID = 0;
  static size_t payloadAutoBytes;
  
  if (requestedClass || requestedID)
  {
    storedClass = requestedClass;   // 0x01 (NAV)
    storedID = requestedID;         // 0x07 (PVT)
  }

  _outputPort.write(incoming); // Echo opcional

  // ¿Estamos buscando el inicio de un mensaje?
  if ((currentSentence == SFE_UBLOX_SENTENCE_TYPE_NONE) || 
      (currentSentence == SFE_UBLOX_SENTENCE_TYPE_NMEA))
  {
    if (incoming == 0xB5) // UBX_SYNCH_1 (inicio de mensaje UBX)
    {
      // Inicio de un mensaje binario UBX
      ubxFrameCounter = 0;
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_UBX;
      packetBuf.counter = 0;
      ignoreThisPayload = false;
      activePacketBuffer = SFE_UBLOX_PACKET_PACKETBUF; // Usa buffer temporal
    }
    else if (incoming == '$')
    {
      // Inicio de un mensaje NMEA
      nmeaByteCounter = 0;
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NMEA;
    }
    else if (incoming == 0xD3)
    {
      // Inicio de un mensaje RTCM
      rtcmFrameCounter = 0;
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_RTCM;
    }
  }

  uint16_t maxPayload = 0;

  // Si estamos procesando un mensaje UBX...
  if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_UBX)
  {
    // Validaciones
    if ((ubxFrameCounter == 0) && (incoming != 0xB5))
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;
    else if ((ubxFrameCounter == 1) && (incoming != 0x62))
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;
    
    else if (ubxFrameCounter == 2) // CLASS
    {
      // Registra la clase en packetBuf
      packetBuf.cls = incoming;
      rollingChecksumA = 0;
      rollingChecksumB = 0;
      packetBuf.counter = 0;
      packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
      packetBuf.startingSpot = incomingUBX->startingSpot;
    }
    
    else if (ubxFrameCounter == 3) // ID
    {
      // Registra el ID en packetBuf
      packetBuf.id = incoming;
      
      // Ahora podemos identificar el tipo de respuesta
      if (packetBuf.cls != UBX_CLASS_ACK)
      {
        bool logBecauseAuto = autoLookup(packetBuf.cls, packetBuf.id, &maxPayload);
        bool logBecauseEnabled = logThisUBX(packetBuf.cls, packetBuf.id) || 
                                 processThisUBX(packetBuf.cls, packetBuf.id);

        // ¿Este paquete coincide con lo que pedimos?
        if ((packetBuf.cls == storedClass) && (packetBuf.id == storedID))
        {
          // SÍ: usa incomingUBX (normalmente packetCfg)
          activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
          incomingUBX->cls = packetBuf.cls;
          incomingUBX->id = packetBuf.id;
          incomingUBX->counter = packetBuf.counter;
        }
        // ¿O es un mensaje "automático" con storage propio?
        else if (logBecauseAuto || logBecauseEnabled)
        {
          // Asigna memoria para packetAuto
          if (logBecauseAuto && (maxPayload == 0))
          {
            // ERROR: maxPayload es cero
          }

          if ((!logBecauseAuto) && (logBecauseEnabled))
            maxPayload = SFE_UBX_MAX_LENGTH;

          // Reasigna payloadAuto si es necesario
          if (payloadAuto && (payloadAutoBytes < maxPayload))
          {
            delete[] payloadAuto;
            payloadAuto = nullptr;
            payloadAutoBytes = 0;
          }

          if (payloadAuto == nullptr)
          {
            payloadAuto = new uint8_t[maxPayload];
            if (payloadAuto)
              payloadAutoBytes = maxPayload;
          }

          packetAuto.payload = payloadAuto;
          
          if (payloadAuto == nullptr)
          {
            // Falló la asignación de RAM, usa packetCfg como fallback
            activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
            incomingUBX->cls = packetBuf.cls;
            incomingUBX->id = packetBuf.id;
            incomingUBX->counter = packetBuf.counter;
          }
          else
          {
            // Éxito: usa packetAuto
            activePacketBuffer = SFE_UBLOX_PACKET_PACKETAUTO;
            packetAuto.cls = packetBuf.cls;
            packetAuto.id = packetBuf.id;
            packetAuto.counter = packetBuf.counter;
            packetAuto.startingSpot = packetBuf.startingSpot;
          }
        }
        else
        {
          // No coincide y no tiene storage: ignora el payload
          ignoreThisPayload = true;
        }
      }
    }
    
    else if (ubxFrameCounter == 4) // LENGTH LSB
    {
      packetBuf.len = incoming;
    }
    
    else if (ubxFrameCounter == 5) // LENGTH MSB
    {
      packetBuf.len |= incoming << 8;
    }
    
    // [Frames 6 y 7: manejo especial para ACK/NACK...]

    // Rutea cada byte al procesador correspondiente
    if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETACK)
      processUBX(incoming, &packetAck, storedClass, storedID);
    else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
      processUBX(incoming, incomingUBX, storedClass, storedID);
    else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF)
      processUBX(incoming, &packetBuf, storedClass, storedID);
    else // SFE_UBLOX_PACKET_PACKETAUTO
      processUBX(incoming, &packetAuto, storedClass, storedID);

    // Echo opcional
    if (_outputPort != _ubxOutputPort)
      _ubxOutputPort.write(incoming);

    ubxFrameCounter++;
  }
  // [Procesamiento de NMEA y RTCM omitido...]
}
```

### ¿Qué hace?

1. **Detecta inicio**: Identifica 0xB5 (inicio UBX), '$' (NMEA) o 0xD3 (RTCM)
2. **Valida sincronización**: Verifica 0xB5 0x62
3. **Lee Class y ID**: Identifica el tipo de mensaje
4. **Asigna buffer**: Decide si usar `packetCfg`, `packetAuto` o `packetBuf`
5. **Rutea bytes**: Envía cada byte a `processUBX()` con el buffer correcto

---

## 8. CONSTRUCCIÓN DEL PAQUETE: `processUBX()` (línea 3246)

Esta función construye el paquete UBX byte por byte y valida el checksum.

### Código (simplificado)

```cpp
void DevUBLOXGNSS::processUBX(uint8_t incoming, 
                              ubxPacket *incomingUBX, 
                              uint8_t requestedClass, 
                              uint8_t requestedID)
{
  // Determina el tamaño máximo del payload
  uint16_t maximum_payload_size;
  if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
    maximum_payload_size = packetCfgPayloadSize;
  else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
  {
    bool logBecauseAuto = autoLookup(incomingUBX->cls, incomingUBX->id, 
                                     &maximum_payload_size);
    bool logBecauseEnabled = logThisUBX(incomingUBX->cls, incomingUBX->id) || 
                             processThisUBX(incomingUBX->cls, incomingUBX->id);
    if ((!logBecauseAuto) && (logBecauseEnabled))
      maximum_payload_size = SFE_UBX_MAX_LENGTH;
  }
  else
    maximum_payload_size = 2;

  bool overrun = false;

  // Añade a checksum (excepto los bytes de checksum mismos)
  if (incomingUBX->counter < (incomingUBX->len + 4))
    addToChecksum(incoming);

  if (incomingUBX->counter == 0)
  {
    incomingUBX->cls = incoming;  // Byte 0: Class
  }
  else if (incomingUBX->counter == 1)
  {
    incomingUBX->id = incoming;   // Byte 1: ID
  }
  else if (incomingUBX->counter == 2)
  {
    incomingUBX->len = incoming;  // Byte 2: Length LSB
  }
  else if (incomingUBX->counter == 3)
  {
    incomingUBX->len |= incoming << 8;  // Byte 3: Length MSB
  }
  else if (incomingUBX->counter == incomingUBX->len + 4)
  {
    incomingUBX->checksumA = incoming;  // Checksum A
  }
  else if (incomingUBX->counter == incomingUBX->len + 5)  // ÚLTIMO BYTE
  {
    incomingUBX->checksumB = incoming;  // Checksum B
    
    currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;  // Fin del mensaje
    
    // VALIDA EL CHECKSUM
    if ((incomingUBX->checksumA == rollingChecksumA) && 
        (incomingUBX->checksumB == rollingChecksumB))
    {
      incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_VALID;  // ✓ Válido
      _signsOfLife = true;
      
      // ¿Coincide con lo que pedimos (data packet)?
      if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID;
      }

      // ¿Es un ACK del mensaje que pedimos?
      else if ((incomingUBX->cls == UBX_CLASS_ACK) && 
               (incomingUBX->id == UBX_ACK_ACK) && 
               (incomingUBX->payload[0] == requestedClass) && 
               (incomingUBX->payload[1] == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID;
      }

      // ¿Es un NACK del mensaje que pedimos?
      else if ((incomingUBX->cls == UBX_CLASS_ACK) && 
               (incomingUBX->id == UBX_ACK_NACK) && 
               (incomingUBX->payload[0] == requestedClass) && 
               (incomingUBX->payload[1] == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_NOTACKNOWLEDGED;
      }

      // ¿Es un mensaje "automático"?
      else if ((autoLookup(incomingUBX->cls, incomingUBX->id)) || 
               (logThisUBX(incomingUBX->cls, incomingUBX->id)) || 
               (processThisUBX(incomingUBX->cls, incomingUBX->id)))
      {
        // Mensaje automático válido
      }

      // ¡AQUÍ ES DONDE SE PARSEAN LOS DATOS!
      if (ignoreThisPayload == false)
      {
        processUBXpacket(incomingUBX);  // ← ¡FUNCIÓN CLAVE!
      }
    }
    else // Checksum inválido
    {
      incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID;

      if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID;
      }
      else if ((incomingUBX->cls == UBX_CLASS_ACK) && 
               (incomingUBX->payload[0] == requestedClass) && 
               (incomingUBX->payload[1] == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID;
      }
    }

    // Libera memoria de packetAuto
    if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
      packetAuto.payload = nullptr;
  }
  else // Bytes del payload (counter: 4 hasta len+3)
  {
    // Ajusta el startingSpot para mensajes automáticos
    uint16_t startingSpot = incomingUBX->startingSpot;
    if (autoLookup(incomingUBX->cls, incomingUBX->id))
      startingSpot = 0;
    
    // ¿Debemos guardar este byte?
    if (ignoreThisPayload == false)
    {
      // Comienza a grabar después de startingSpot
      if ((incomingUBX->counter - 4) >= startingSpot)
      {
        // Verifica espacio disponible
        if (((incomingUBX->counter - 4) - startingSpot) < maximum_payload_size)
        {
          // Almacena este byte en el array payload
          incomingUBX->payload[(incomingUBX->counter - 4) - startingSpot] = incoming;
        }
        else
        {
          overrun = true; // Buffer overflow
        }
      }
    }
  }

  // Detecta buffer overrun
  if (overrun || ((incomingUBX->counter == maximum_payload_size + 6) && 
                  (ignoreThisPayload == false)))
  {
    currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;
    // ERROR: buffer overflow
  }

  // Incrementa el contador
  incomingUBX->counter++;
}
```

### ¿Qué hace?

1. **Construye el paquete**: Lee Class, ID, Length, Payload, Checksum
2. **Valida checksum**: Compara con `rollingChecksumA` y `rollingChecksumB`
3. **Marca validez**: Establece flags `valid` y `classAndIDmatch`
4. **Llama a parseo**: Invoca `processUBXpacket()` si el checksum es válido

### Al finalizar esta función

- `incomingUBX->payload[]` contiene los 92 bytes de datos PVT
- `incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_VALID`
- `incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID`
- Se ha llamado a `processUBXpacket()` para parsear

---

## 9. PARSEO FINAL: `processUBXpacket()` (línea 3500)

¡**AQUÍ ESTÁ LA MAGIA DEL PARSEO**! Esta función interpreta los bytes del payload.

### Código

```cpp
void DevUBLOXGNSS::processUBXpacket(ubxPacket *msg)
{
  bool addedToFileBuffer = false;
  
  switch (msg->cls)
  {
  case UBX_CLASS_NAV:
    if (msg->id == UBX_NAV_PVT && msg->len == UBX_NAV_PVT_LEN)  // 92 bytes
    {
      // Verifica que existe memoria asignada para PVT
      if (packetUBXNAVPVT != nullptr)
      {
        // ¡AQUÍ SE EXTRAEN LOS DATOS DEL PAYLOAD!
        // Usa las funciones extractXXX() para interpretar los bytes
        
        // Tiempo GPS (iTOW = GPS Time of Week en ms)
        packetUBXNAVPVT->data.iTOW = extractLong(msg, 0);  // Bytes 0-3
        
        // Fecha y hora UTC
        packetUBXNAVPVT->data.year = extractInt(msg, 4);   // Bytes 4-5
        packetUBXNAVPVT->data.month = extractByte(msg, 6); // Byte 6
        packetUBXNAVPVT->data.day = extractByte(msg, 7);   // Byte 7
        packetUBXNAVPVT->data.hour = extractByte(msg, 8);  // Byte 8
        packetUBXNAVPVT->data.min = extractByte(msg, 9);   // Byte 9
        packetUBXNAVPVT->data.sec = extractByte(msg, 10);  // Byte 10
        
        // Flags de validez
        packetUBXNAVPVT->data.valid.all = extractByte(msg, 11); // Byte 11
        
        // Precisión temporal
        packetUBXNAVPVT->data.tAcc = extractLong(msg, 12);  // Bytes 12-15
        packetUBXNAVPVT->data.nano = extractSignedLong(msg, 16); // Bytes 16-19
        
        // Tipo de fix y flags
        packetUBXNAVPVT->data.fixType = extractByte(msg, 20);    // Byte 20
        packetUBXNAVPVT->data.flags.all = extractByte(msg, 21);  // Byte 21
        packetUBXNAVPVT->data.flags2.all = extractByte(msg, 22); // Byte 22
        
        // Número de satélites
        packetUBXNAVPVT->data.numSV = extractByte(msg, 23);      // Byte 23
        
        // Posición (WGS84)
        packetUBXNAVPVT->data.lon = extractSignedLong(msg, 24);    // Bytes 24-27
        packetUBXNAVPVT->data.lat = extractSignedLong(msg, 28);    // Bytes 28-31
        packetUBXNAVPVT->data.height = extractSignedLong(msg, 32); // Bytes 32-35
        packetUBXNAVPVT->data.hMSL = extractSignedLong(msg, 36);   // Bytes 36-39
        
        // Precisión horizontal y vertical
        packetUBXNAVPVT->data.hAcc = extractLong(msg, 40);  // Bytes 40-43
        packetUBXNAVPVT->data.vAcc = extractLong(msg, 44);  // Bytes 44-47
        
        // Velocidad (NED frame)
        packetUBXNAVPVT->data.velN = extractSignedLong(msg, 48);   // Bytes 48-51
        packetUBXNAVPVT->data.velE = extractSignedLong(msg, 52);   // Bytes 52-55
        packetUBXNAVPVT->data.velD = extractSignedLong(msg, 56);   // Bytes 56-59
        packetUBXNAVPVT->data.gSpeed = extractSignedLong(msg, 60); // Bytes 60-63
        
        // Rumbo
        packetUBXNAVPVT->data.headMot = extractSignedLong(msg, 64); // Bytes 64-67
        
        // Precisión de velocidad y rumbo
        packetUBXNAVPVT->data.sAcc = extractLong(msg, 68);    // Bytes 68-71
        packetUBXNAVPVT->data.headAcc = extractLong(msg, 72); // Bytes 72-75
        
        // DOP y más flags
        packetUBXNAVPVT->data.pDOP = extractInt(msg, 76);        // Bytes 76-77
        packetUBXNAVPVT->data.flags3.all = extractInt(msg, 78);  // Bytes 78-79
        
        // Rumbo del vehículo y declinación magnética
        packetUBXNAVPVT->data.headVeh = extractSignedLong(msg, 84); // Bytes 84-87
        packetUBXNAVPVT->data.magDec = extractSignedInt(msg, 88);   // Bytes 88-89
        packetUBXNAVPVT->data.magAcc = extractInt(msg, 90);         // Bytes 90-91
        
        // Marca todos los datos como "frescos" (recién actualizados)
        // Esto permite que las funciones getLatitude(), getLongitude(), etc.
        // sepan que hay datos nuevos disponibles
        packetUBXNAVPVT->moduleQueried.moduleQueried1.all = 0xFFFFFFFF;
        packetUBXNAVPVT->moduleQueried.moduleQueried2.all = 0xFFFFFFFF;
        
        // Copia datos para callbacks si está configurado
        if ((packetUBXNAVPVT->callbackData != nullptr) && 
            (packetUBXNAVPVT->automaticFlags.flags.bits.callbackCopyValid == false))
        {
          memcpy(&packetUBXNAVPVT->callbackData->iTOW, 
                 &packetUBXNAVPVT->data.iTOW, 
                 sizeof(UBX_NAV_PVT_data_t));
          packetUBXNAVPVT->automaticFlags.flags.bits.callbackCopyValid = true;
        }
        
        // Copia al buffer de archivo si está habilitado
        if (packetUBXNAVPVT->automaticFlags.flags.bits.addToFileBuffer)
        {
          addedToFileBuffer = storePacket(msg);
        }
      }
    }
    // [Otros mensajes NAV omitidos...]
    break;
  
  // [Otros casos: RXM, MON, TIM, etc. omitidos...]
  }
}
```

### ¿Qué hace?

1. **Identifica el mensaje**: `switch (msg->cls)` → `case UBX_CLASS_NAV`
2. **Verifica ID y longitud**: `msg->id == UBX_NAV_PVT && msg->len == 92`
3. **Extrae cada campo**: Llama a `extractLong()`, `extractByte()`, etc.
4. **Marca datos frescos**: `moduleQueried.all = 0xFFFFFFFF`
5. **Copia para callbacks**: Si hay callbacks registrados

---

## 10. FUNCIONES DE EXTRACCIÓN (línea 19635)

Estas funciones interpretan los bytes en formato **little-endian**.

### `extractLong()` - Extrae uint32_t (4 bytes)

```cpp
uint32_t DevUBLOXGNSS::extractLong(ubxPacket *msg, uint16_t spotToStart)
{
  uint32_t val = 0;
  for (uint8_t i = 0; i < 4; i++)
    val |= (uint32_t)msg->payload[spotToStart + i] << (8 * i);
  return val;
}
```

**Ejemplo:**

```
Payload bytes 0-3: [E8 03 00 00]

val = 0xE8 | (0x03 << 8) | (0x00 << 16) | (0x00 << 24)
    = 0xE8 | 0x300 | 0x00 | 0x00
    = 0x000003E8
    = 1000 (decimal)

Si esto es iTOW, significa: 1000 ms = 1 segundo en la semana GPS
```

### `extractSignedLong()` - Extrae int32_t con signo

```cpp
int32_t DevUBLOXGNSS::extractSignedLong(ubxPacket *msg, uint16_t spotToStart)
{
  unsignedSigned32 converter;
  converter.unsigned32 = extractLong(msg, spotToStart);
  return converter.signed32;
}
```

Utiliza una unión para reinterpretar los bits como número con signo:

```cpp
typedef union {
  uint32_t unsigned32;
  int32_t signed32;
} unsignedSigned32;
```

**Ejemplo con latitud:**

```
Payload bytes 28-31: [3A 12 FE 17] (little-endian)

unsigned32 = 0x3A | (0x12 << 8) | (0xFE << 16) | (0x17 << 24)
           = 0x17FE123A
           = 402,788,922 (decimal sin signo)

signed32 = 402,788,922 (positivo)

Latitud = 402,788,922 * 10^-7 = 40.2788922° (Buenos Aires)
```

**Ejemplo con número negativo:**

```
Payload bytes: [FF FF FF FF]

unsigned32 = 0xFFFFFFFF = 4,294,967,295
signed32 = -1 (interpretado como complemento a 2)
```

### `extractInt()` - Extrae uint16_t (2 bytes)

```cpp
uint16_t DevUBLOXGNSS::extractInt(ubxPacket *msg, uint16_t spotToStart)
{
  uint16_t val = (uint16_t)msg->payload[spotToStart + 0] << 0;
  val |= (uint16_t)msg->payload[spotToStart + 1] << 8;
  return val;
}
```

### `extractSignedInt()` - Extrae int16_t con signo

```cpp
int16_t DevUBLOXGNSS::extractSignedInt(ubxPacket *msg, uint16_t spotToStart)
{
  unsignedSigned16 converter;
  converter.unsigned16 = extractInt(msg, spotToStart);
  return converter.signed16;
}
```

### `extractByte()` - Extrae uint8_t (1 byte)

```cpp
uint8_t DevUBLOXGNSS::extractByte(ubxPacket *msg, uint16_t spotToStart)
{
  return msg->payload[spotToStart];
}
```

---

## 11. ACCESO A LOS DATOS PARSEADOS

Una vez que `processUBXpacket()` completa, los datos están disponibles en:

```cpp
packetUBXNAVPVT->data.lat   // Latitud (deg * 10^-7)
packetUBXNAVPVT->data.lon   // Longitud (deg * 10^-7)
packetUBXNAVPVT->data.height // Altura (mm)
// ... etc.
```

### Funciones de acceso público

La librería proporciona funciones getter:

```cpp
int32_t DevUBLOXGNSS::getLatitude(uint16_t maxWait)
{
  if (packetUBXNAVPVT == nullptr)
    initPacketUBXNAVPVT();
  if (packetUBXNAVPVT == nullptr)
    return 0;

  if (packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.lat == false)
    getPVT(maxWait);  // Solicita datos si no están frescos
  
  packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.lat = false; // Marca como leído
  packetUBXNAVPVT->moduleQueried.moduleQueried1.bits.all = false;
  
  return (packetUBXNAVPVT->data.lat);
}
```

### Ejemplo de uso

```cpp
int32_t latitude = myGPS.getLatitude();
int32_t longitude = myGPS.getLongitude();

// Convierte a grados decimales
float lat_degrees = latitude * 1e-7;
float lon_degrees = longitude * 1e-7;

Serial.print("Lat: ");
Serial.println(lat_degrees, 7);  // 40.2788922
Serial.print("Lon: ");
Serial.println(lon_degrees, 7);  // -58.1234567
```

---

## RESUMEN COMPLETO DEL FLUJO

```
┌────────────────────────────────────────────────────────────────┐
│                  FLUJO COMPLETO DE getPVT()                    │
└────────────────────────────────────────────────────────────────┘

1. getPVT()
   - Prepara solicitud: UBX-NAV-PVT (Class=0x01, ID=0x07, Len=0)
   ↓

2. sendCommand()
   - Calcula checksum
   - Decide esperar datos (no ACK)
   ↓

3. sendI2cCommand()
   - Envía: B5 62 01 07 00 00 [CRC]
   - GPS lo recibe vía I2C (dirección 0x42, registro 0xFF)
   ↓

4. GPS SAM-M10Q procesa y responde
   - Construye: B5 62 01 07 5C 00 [92 bytes] [CRC]
   - Total: 100 bytes almacenados en buffer FIFO interno
   ↓

5. waitForNoACKResponse()
   - Loop de espera (max 1100 ms por defecto)
   - Llama repetidamente a checkUbloxInternal()
   ↓

6. checkUbloxI2C()
   - Lee registros 0xFD/0xFE → "100 bytes disponibles"
   - Lee registro 0xFF en bloques de 32 bytes
   - Total: 4 transacciones I2C para leer 100 bytes
   ↓

7. process() - por cada byte recibido
   - Detecta inicio UBX: 0xB5 0x62
   - Lee Class (0x01) e ID (0x07)
   - Identifica que coincide con lo solicitado
   - Asigna buffer: packetCfg
   - Rutea a processUBX()
   ↓

8. processUBX() - construye paquete
   - Lee Length: 0x5C 0x00 → 92 bytes
   - Almacena payload en incomingUBX->payload[0..91]
   - Lee checksumA y checksumB
   - Valida checksum: ✓ válido
   - Marca: valid = VALID, classAndIDmatch = VALID
   - Llama a processUBXpacket()
   ↓

9. processUBXpacket() - PARSEA
   - case UBX_CLASS_NAV, UBX_NAV_PVT:
   - Extrae iTOW (bytes 0-3): extractLong()
   - Extrae year (bytes 4-5): extractInt()
   - Extrae lat (bytes 28-31): extractSignedLong()
   - Extrae lon (bytes 24-27): extractSignedLong()
   - ... extrae todos los 92 bytes
   - Almacena en: packetUBXNAVPVT->data
   - Marca: moduleQueried.all = 0xFFFFFFFF
   ↓

10. extractXXX() - interpreta bytes
    - extractLong(): 4 bytes little-endian → uint32_t
    - extractSignedLong(): 4 bytes → int32_t
    - extractInt(): 2 bytes → uint16_t
    - extractByte(): 1 byte → uint8_t
    ↓

11. DATOS LISTOS
    - packetUBXNAVPVT->data contiene todos los datos parseados
    - getLatitude(), getLongitude(), etc. pueden acceder a los datos
    - Conversión: lat/lon en grados = valor * 10^-7
```

---

## ASPECTO CLAVE: Little-Endian

**Todos los valores multi-byte en el protocolo UBX usan little-endian** (byte menos significativo primero).

### Ejemplo 1: Latitud

```
Latitud: 40.2788922° → 402,788,922 (escala 10^-7)

En hexadecimal: 0x17FE123A

Enviado por GPS:  3A 12 FE 17  (LSB primero)
                  ↑           ↑
                 LSB         MSB

extractSignedLong() reconstruye:
  val = 0x3A | (0x12 << 8) | (0xFE << 16) | (0x17 << 24)
      = 0x17FE123A
      = 402,788,922
```

### Ejemplo 2: Año 2025

```
Año: 2025 → 0x07E9

Enviado por GPS:  E9 07  (LSB primero)
                  ↑   ↑
                 LSB MSB

extractInt() reconstruye:
  val = 0xE9 | (0x07 << 8)
      = 0x07E9
      = 2025
```

---

## ESCALAS Y UNIDADES

Los valores en UBX-NAV-PVT tienen escalas específicas:

| Campo     | Tipo | Escala      | Unidad      | Ejemplo                      |
|-----------|------|-------------|-------------|------------------------------|
| lat/lon   | I4   | 10^-7       | deg         | 402788922 → 40.2788922°      |
| height    | I4   | 1           | mm          | 25000 → 25 m                 |
| hMSL      | I4   | 1           | mm          | 25000 → 25 m                 |
| velN/E/D  | I4   | 1           | mm/s        | 1000 → 1 m/s                 |
| gSpeed    | I4   | 1           | mm/s        | 5000 → 5 m/s = 18 km/h       |
| headMot   | I4   | 10^-5       | deg         | 9000000 → 90.00000°          |
| pDOP      | U2   | 0.01        | -           | 150 → 1.50                   |
| magDec    | I2   | 10^-2       | deg         | -350 → -3.50°                |
| iTOW      | U4   | 1           | ms          | 123456000 → 123456 segundos  |
| tAcc      | U4   | 1           | ns          | 50000 → 50 μs                |
| nano      | I4   | 1           | ns          | 500000000 → 0.5 segundos     |

---

## CONCLUSIÓN

El parseo de datos en `getPVT()` es un proceso complejo pero bien estructurado:

1. **Solicitud**: Envía mensaje UBX-NAV-PVT por I2C
2. **Recepción**: Lee bytes del GPS en bloques de 32 bytes
3. **Detección**: Identifica inicio UBX (0xB5 0x62)
4. **Construcción**: Acumula bytes en buffer, valida checksum
5. **Parseo**: Interpreta bytes según estructura UBX-NAV-PVT
6. **Extracción**: Convierte little-endian a tipos nativos C
7. **Almacenamiento**: Guarda en `packetUBXNAVPVT->data`

La clave está en:
- **Protocolo I2C**: Registros 0xFD/0xFE (disponibilidad), 0xFF (datos)
- **Formato UBX**: Header (6 bytes) + Payload (92 bytes) + Checksum (2 bytes)
- **Little-endian**: Byte menos significativo primero
- **Escalas**: Cada campo tiene su escala específica (10^-7 para lat/lon, etc.)

¡Los datos quedan listos para usar con las funciones `getLatitude()`, `getLongitude()`, etc.!
