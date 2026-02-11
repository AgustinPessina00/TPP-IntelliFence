# Verificación de Configuración GPS con VALGET

## Descripción General

Este documento explica en detalle el sistema de verificación de configuración del GPS SAM-M10Q usando el comando UBX-CFG-VALGET. El objetivo es confirmar que cada configuración escrita en el GPS se aplicó correctamente leyendo el valor configurado y comparándolo con el valor esperado.

---

## 1. Flujo en `configure_gps()`

Por cada configuración (5 configuraciones iniciales), se ejecuta:

```cpp
write_register_uart(payload, size, RAM);     // Escribe en RAM
write_register_uart(payload, size, BBR);     // Escribe en BBR (persistente)
verify_config_with_valget(payload, size, RAM); // Verifica que se escribió correctamente
```

### Ejemplo con el primer payload (CFG-I2C-ENABLED)

- **Payload:** `m10q_data_payloads[0].data` → KeyID (4 bytes) + Value (N bytes)
- **Paso 1:** Se escribe el valor en **RAM** (memoria volátil)
- **Paso 2:** Se escribe el valor en **BBR** (Battery Backed RAM - persistente, sobrevive a resets)
- **Paso 3:** Se verifica que **RAM** tiene el valor correcto usando VALGET

### ¿Por qué verificar solo RAM?

- RAM es la capa activa que usa el GPS en tiempo de ejecución
- Si RAM tiene el valor correcto, la configuración está aplicada
- BBR es solo para persistencia, pero no se usa hasta el próximo reset

---

## 2. Función `verify_config_with_valget()`

Esta es la función principal que coordina todo el proceso de verificación.

### **Paso 1: Validaciones iniciales**

```cpp
if (!payload_data || payload_len <= UBX_KEYID_SIZE || !uartBus) {
    return false;
}
```

**Validaciones:**
- El payload debe ser válido (no NULL)
- Debe tener al menos 5 bytes (4 de KeyID + 1 de valor mínimo)
- El bus UART debe estar disponible

### **Paso 2: Extracción de datos del payload**

```cpp
const uint8_t* key_id = payload_data;                              // Primeros 4 bytes
const uint8_t* expected_value = payload_data + UBX_KEYID_SIZE;    // Resto de bytes
uint8_t value_size = static_cast<uint8_t>(payload_len - UBX_KEYID_SIZE);
```

**Ejemplo concreto con CFG-I2C-ENABLED:**

Si `payload = {0x03, 0x00, 0x51, 0x10, 0x01}` (CFG-I2C-ENABLED = 1)

- `key_id = {0x03, 0x00, 0x51, 0x10}` → ID de la configuración (4 bytes)
- `expected_value = {0x01}` → Valor que esperamos leer (I2C habilitado)
- `value_size = 1` → Tamaño del valor (1 byte - tipo L - boolean)

### **Paso 3: Construir mensaje UBX-CFG-VALGET**

```cpp
uint8_t message[UBX_MAX_MESSAGE_SIZE];
uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, 
                                       key_id, UBX_KEYID_SIZE, message, UBX_MAX_MESSAGE_SIZE);
```

#### ¿Qué hace `build_ubx_message()`?

Construye un frame UBX-CFG-VALGET con la siguiente estructura:

```
┌────────────┬─────────────────────────────────────────────────────────┬──────────┐
│   Header   │                      Payload                            │ Checksum │
├────────────┼─────────────────────────────────────────────────────────┼──────────┤
│ 0xB5 0x62  │ Class │ ID │ Len_L │ Len_H │ Ver │ Layer │ Res_L │ Res_H │ CK_A CK_B│
│   2 bytes  │  0x06 │0x8B│       │       │ 0x00│ 0x01  │ 0x00  │ 0x00  │  2 bytes │
└────────────┴───────┴────┴───────┴───────┴─────┴───────┴───────┴───────┴──────────┘
                                                  ↓
                                          ┌───────────────┐
                                          │ KeyID (4B)    │
                                          └───────────────┘
```

**Ejemplo completo para CFG-I2C-ENABLED:**

```
B5 62       → Sync chars (header UBX)
06 8B       → Class=0x06 (CFG), ID=0x8B (VALGET)
08 00       → Length = 8 bytes (version + layer + reserved + keyID)
00          → Version = 0 (transactionless)
01          → Layer = RAM (0x01)
00 00       → Reserved (2 bytes)
03 00 51 10 → KeyID (CFG-I2C-ENABLED en little-endian)
XX YY       → Checksum UBX (CK_A, CK_B)
```

Total: 16 bytes

### **Paso 4: Enviar mensaje por UART**

```cpp
if (send_message_uart(message, msg_len, 100) != HAL_OK) {
    return false;
}
```

**Detalles:**
- Envía el mensaje VALGET construido al GPS vía UART
- Espera 100ms para que el GPS procese la petición
- Si falla el envío, retorna `false`

### **Paso 5: Leer respuesta del GPS**

```cpp
uint8_t response_buffer[128];
uint16_t bytes_received = 0;
uint32_t start_time = HAL_GetTick();

while ((HAL_GetTick() - start_time) < timeout) {
    uint16_t chunk_received = 0;
    UARTResult result = uartBus->receiveAvailable(
        response_buffer + bytes_received,
        sizeof(response_buffer) - bytes_received,
        &chunk_received,
        100
    );
    
    if (result == UART_OK && chunk_received > 0) {
        bytes_received += chunk_received;
        
        if (parse_valget_response(response_buffer, bytes_received, 
                                   key_id, expected_value, value_size)) {
            return true; // ¡Encontrado y correcto!
        }
    }
    
    BusyDelayMs(10);
}
```

#### ¿Por qué un bucle?

1. **El GPS puede enviar la respuesta en múltiples fragmentos**
   - UART tiene buffer limitado
   - Puede haber otros mensajes NMEA/UBX intercalados

2. **Timeout de 1 segundo (1000ms)**
   - Da tiempo suficiente al GPS para responder
   - Evita bloqueo infinito si no hay respuesta

3. **Acumulación de datos**
   - Cada iteración intenta leer datos disponibles
   - Los acumula en `response_buffer`
   - Intenta parsear después de cada lectura

4. **Salida temprana**
   - Si encuentra y valida la respuesta → `return true`
   - No espera el timeout completo si ya tiene la respuesta

---

## 3. Función `parse_valget_response()`

Esta función busca, valida y compara la respuesta del GPS.

### **Paso 1: Buscar sincronización UBX**

```cpp
for (uint16_t i = 0; i + 8 < buffer_len; i++) {
    if (response_buffer[i] != UBX_HEADER1 || response_buffer[i + 1] != UBX_HEADER2) {
        continue; // No es un mensaje UBX, seguir buscando
    }
```

**Proceso:**
- Recorre todo el buffer byte por byte
- Busca el patrón `0xB5 0x62` (header UBX estándar)
- Si no lo encuentra, salta al siguiente byte
- Puede haber basura o mensajes NMEA antes del UBX

### **Paso 2: Verificar que es un mensaje VALGET**

```cpp
if (response_buffer[i + 2] != VALGET_CLASS || response_buffer[i + 3] != VALGET_ID) {
    continue; // Es UBX pero no VALGET
}
```

**Validación:**
- Verifica Class = `0x06` (CFG)
- Verifica ID = `0x8B` (VALGET)
- Si es otro tipo de mensaje UBX (ej: NAV-PVT), ignora y continúa buscando

### **Paso 3: Leer longitud del payload**

```cpp
uint16_t payload_len = response_buffer[i + 4] | (response_buffer[i + 5] << 8);
uint16_t total_msg_len = 6 + payload_len + 2;

if (i + total_msg_len > buffer_len) {
    continue; // Mensaje incompleto
}
```

**Cálculo del tamaño:**
- Lee los bytes 4 y 5 en formato **little-endian**
- Calcula tamaño total:
  - 6 bytes de header (sync + class + id + length)
  - N bytes de payload
  - 2 bytes de checksum
- Verifica que tengamos el mensaje completo en el buffer

### **Paso 4: Verificar checksum UBX**

```cpp
if (!verifyUBXChecksum(&response_buffer[i], total_msg_len)) {
    continue; // Checksum inválido, mensaje corrupto
}
```

**Validación de integridad:**
- Usa la función existente `verifyUBXChecksum()`
- Calcula checksum Fletcher-8 sobre: Class + ID + Length + Payload
- Compara con los 2 bytes finales del mensaje
- Si no coincide → mensaje corrupto, descartarlo

### **Paso 5: Parsear el payload VALGET**

#### Estructura del payload VALGET (respuesta):

```
┌─────────┬───────┬──────────────┬──────────────┬─────────────┐
│ VERSION │ LAYER │   RESERVED   │    KEYID     │    VALUE    │
│   1B    │  1B   │     2B       │     4B       │     NB      │
└─────────┴───────┴──────────────┴──────────────┴─────────────┘
    0x00     0x01      0x00 0x00   0x03 0x00..   0x01 (ej)
```

```cpp
const uint8_t* payload = &response_buffer[i + 6];
const uint8_t* payload_keyid = payload + 4;      // Saltar header (4 bytes)
const uint8_t* payload_value = payload + 4 + UBX_KEYID_SIZE; // Saltar header + keyID
```

**Extracción:**
- `payload` apunta al inicio del payload (después del header UBX)
- `payload_keyid` apunta al KeyID (offset +4 para saltar version, layer, reserved)
- `payload_value` apunta al valor (offset +4 +4 para saltar header + KeyID)

### **Paso 6: Verificar que el KeyID coincide**

```cpp
if (memcmp(payload_keyid, key_id, UBX_KEYID_SIZE) != 0) {
    continue; // KeyID diferente, no es nuestra respuesta
}
```

**Validación:**
- Compara los 4 bytes del KeyID byte por byte
- Asegura que es la respuesta del registro que pedimos
- Si no coincide → puede ser otro VALGET, seguir buscando

### **Paso 7: Comparar el valor recibido con el esperado**

```cpp
if (memcmp(payload_value, expected_value, value_size) == 0) {
    return true;  // ¡COINCIDE! Configuración correcta ✅
} else {
    return false; // KeyID correcto pero valor diferente ❌
}
```

**Resultado final:**
- **`true`:** El valor configurado coincide con el leído → Configuración correcta ✅
- **`false`:** KeyID correcto pero valor diferente → Configuración incorrecta ❌

**Importante:** Si el KeyID es correcto, esta es la respuesta definitiva (no continúa buscando).

---

## 4. Ejemplo Completo de Flujo

### Configurando **CFG-I2C-ENABLED = 1** (Habilitar I2C)

#### **1. Escribir configuración**

```cpp
write_register_uart({0x03,0x00,0x51,0x10,0x01}, 5, RAM)
```

**Mensaje enviado al GPS (VALSET):**
```
B5 62 06 8A 09 00 00 01 00 00 03 00 51 10 01 XX XX
│  │  │  │  │        │  │        │           │  └─ Checksums
│  │  │  │  │        │  │        │           └─ Value = 0x01 (enabled)
│  │  │  │  │        │  │        └─ KeyID = 0x10510003 (CFG-I2C-ENABLED)
│  │  │  │  │        │  └─ Reserved 0x0000
│  │  │  │  │        └─ Layer = 0x01 (RAM)
│  │  │  │  └─ Length = 9 bytes (version + layer + reserved + keyid + value)
│  │  │  └─ ID = 0x8A (VALSET)
│  │  └─ Class = 0x06 (CFG)
│  └─ Sync char 2
└─ Sync char 1
```

GPS recibe el mensaje y escribe `0x01` en la configuración `CFG-I2C-ENABLED` en RAM.

#### **2. Verificar configuración**

```cpp
verify_config_with_valget({0x03,0x00,0x51,0x10,0x01}, 5, RAM)
```

**Extracción de datos:**
- KeyID = `{0x03, 0x00, 0x51, 0x10}` (4 bytes)
- Valor esperado = `{0x01}` (1 byte - I2C enabled)
- Tamaño valor = 1 byte

#### **3. Enviar mensaje VALGET**

```
B5 62 06 8B 08 00 00 01 00 00 03 00 51 10 XX XX
│  │  │  │  │        │  │        │           └─ Checksums
│  │  │  │  │        │  │        └─ KeyID = 0x10510003 (CFG-I2C-ENABLED)
│  │  │  │  │        │  └─ Reserved 0x0000
│  │  │  │  │        └─ Layer = 0x01 (RAM)
│  │  │  │  └─ Length = 8 bytes (version + layer + reserved + keyid)
│  │  │  └─ ID = 0x8B (VALGET)
│  │  └─ Class = 0x06 (CFG)
│  └─ Sync char 2
└─ Sync char 1
```

GPS recibe la petición VALGET pidiendo el valor de `CFG-I2C-ENABLED` desde RAM.

#### **4. GPS responde**

```
B5 62 06 8B 09 00 00 01 00 00 03 00 51 10 01 XX XX
│  │  │  │  │        │  │        │           │  └─ Checksums
│  │  │  │  │        │  │        │           └─ Value = 0x01 ✅
│  │  │  │  │        │  │        └─ KeyID = 0x10510003
│  │  │  │  │        │  └─ Reserved 0x0000
│  │  │  │  │        └─ Layer = 0x01 (RAM)
│  │  │  │  └─ Length = 9 bytes (header + keyid + value)
│  │  │  └─ ID = 0x8B (VALGET response)
│  │  └─ Class = 0x06 (CFG)
│  └─ Sync char 2
└─ Sync char 1
```

#### **5. Parser procesa la respuesta**

1. **Encuentra sincronización:** `0xB5 0x62` ✅
2. **Verifica tipo:** Class=`0x06`, ID=`0x8B` ✅ (es VALGET)
3. **Lee longitud:** 9 bytes de payload
4. **Verifica checksum:** Válido ✅
5. **Extrae KeyID:** `{0x03, 0x00, 0x51, 0x10}` ✅
6. **Compara KeyID:** Coincide con el solicitado ✅
7. **Extrae valor:** `{0x01}`
8. **Compara valor:** `{0x01}` == `{0x01}` ✅

#### **6. Resultado final**

```cpp
return true; // ✅ Configuración verificada correctamente
```

**Conclusión:** El GPS confirmó que `CFG-I2C-ENABLED` está configurado con el valor `0x01` (habilitado) en RAM.

---

## 5. Manejo de Errores

### Casos en los que retorna `false`:

1. **Payload inválido**
   ```cpp
   if (!payload_data || payload_len <= UBX_KEYID_SIZE || !uartBus)
   ```
   - Datos corruptos o incompletos

2. **Fallo al construir mensaje**
   ```cpp
   if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE)
   ```
   - Buffer insuficiente o error interno

3. **Fallo al enviar por UART**
   ```cpp
   if (send_message_uart(...) != HAL_OK)
   ```
   - Bus UART ocupado o error de hardware

4. **Timeout sin respuesta**
   ```cpp
   while ((HAL_GetTick() - start_time) < timeout) { ... }
   return false; // Si sale del bucle sin encontrar respuesta
   ```
   - GPS no respondió en 1 segundo
   - GPS apagado o cable desconectado

5. **Checksum inválido**
   ```cpp
   if (!verifyUBXChecksum(...))
   ```
   - Mensaje corrupto por ruido en UART
   - Interferencias electromagnéticas

6. **Valor incorrecto**
   ```cpp
   if (memcmp(payload_value, expected_value, value_size) != 0)
   ```
   - La configuración no se aplicó correctamente
   - GPS rechazó el valor (fuera de rango, inválido, etc.)

---

## 6. Ventajas de este Sistema

### ✅ **Robustez**
- Confirma que cada configuración se aplicó correctamente
- Detecta fallos de comunicación o escritura

### ✅ **Debugging**
- Facilita detectar qué configuración falló
- Permite retry logic si es necesario

### ✅ **Confiabilidad**
- No asume que la escritura fue exitosa
- Verifica contra el estado real del GPS

### ✅ **Compatibilidad con múltiples mensajes**
- El parser puede encontrar VALGET entre mensajes NMEA
- Maneja fragmentación de UART correctamente

---

## 7. Consideraciones de Rendimiento

### Tiempo de verificación por registro

- **Envío VALGET:** ~10-20 ms
- **Delay post-envío:** 100 ms
- **Lectura respuesta:** Up to 1000 ms (timeout)
- **Total típico:** ~150-200 ms por registro

### Optimizaciones posibles

1. **Reducir timeout** si el GPS siempre responde rápido
2. **Batch VALGET** para múltiples KeyIDs (requiere cambios en protocolo)
3. **Solo verificar configuraciones críticas** (no todas)

---

## 8. Referencias

- **u-blox Interface Description:** UBX-CFG-VALSET / UBX-CFG-VALGET
- **SAM-M10Q Datasheet:** Configuration Interface
- **Implementación:** `sam_m10q.cpp` / `sam_m10q.h`

---

## Resumen

Este sistema implementa un ciclo **Write → Verify** para cada configuración GPS:

1. Se escribe la configuración usando **VALSET**
2. Se lee el valor usando **VALGET**
3. Se compara con el valor esperado
4. Retorna **true/false** según coincida o no

Esto garantiza que el GPS esté correctamente configurado antes de continuar con la operación normal.
