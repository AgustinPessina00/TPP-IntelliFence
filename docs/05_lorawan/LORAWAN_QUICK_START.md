# Guía Rápida de Integración - LoRaWAN con fsmTask

## Ejemplo: Enviar Posición GPS Periódicamente

### Opción 1: Integración en la Rutina de Startup

En `fsmTask.cpp`, envía la posición después de obtenerla exitosamente:

```cpp
case STARTUP_ROUTINE_WAIT_NEW_POSITION:
    if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
        if (updatePosition(cow, *msgReceived) == HAL_OK) {
            
            // ⭐ AGREGAR: Enviar posición por LoRa
            LoraGpsData_t loraData;
            Position pos = cow.getPosition();
            loraData.latitude = pos.latitude;
            loraData.longitude = pos.longitude;
            loraData.zone = cow.getCurrentZone();
            loraData.distance = cow.getDistanceToLimit();
            sendGpsDataToLora(&loraData);
            
            *state = STARTUP_ROUTINE_REQUEST_ZONE;
            tries = 0;
        }
    } else {
        tries++;
        if (tries >= MAX_TRIES) {
            *state = STARTUP_ROUTINE_REQUEST_NEW_POSITION;
        }
    }
    break;
```

### Opción 2: Integración en Operación Normal

En el estado `INITIALIZE_END`, envía la posición:

```cpp
case INITIALIZE_END:
    initializeState = INITIALIZE_BEGIN;
    if (isInFence(cow) == HAL_OK) {
        // ⭐ AGREGAR: Enviar posición por LoRa
        LoraGpsData_t loraData;
        Position pos = cow.getPosition();
        loraData.latitude = pos.latitude;
        loraData.longitude = pos.longitude;
        loraData.zone = cow.getCurrentZone();
        loraData.distance = cow.getDistanceToLimit();
        sendGpsDataToLora(&loraData);
        
        normalOpFSM = NormalOpFSM_t::GREEN_ZONE;
        sendZoneToStimulus(cow.getCurrentZone(), MODULE_STIMULUS);
    } else {
        normalOpFSM = NormalOpFSM_t::STIMULUS_ZONE;
    }
    break;
```

### Opción 3: Código de Prueba Simple (Recomendado para Testing)

Agrega este código temporal al inicio de `fsmTask()` para probar sin depender del GPS:

```cpp
void fsmTask(void *argument) {
    (void)argument;

    DeviceUID deviceUID = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    static Cow cow(deviceUID);
    static Fence fence;
    
    s_cow = &cow;
    s_fence = &fence;
    
    EmbeddedMessage_t *msgReceived = NULL;
    uint8_t tries = 0;
    
    RTOS_LOG_INFO("[FSM] Task initialized successfully\n");

    // ⭐⭐⭐ CÓDIGO DE PRUEBA LORA - COMENTAR DESPUÉS DE PROBAR ⭐⭐⭐
    static uint32_t loraTestCounter = 0;
    static const uint32_t LORA_TEST_INTERVAL = 100; // Envía cada 10 seg (100 * 100ms)
    
    while(1) {
        // Código de prueba LoRa (cada 10 segundos)
        if (loraTestCounter % LORA_TEST_INTERVAL == 0) {
            LoraGpsData_t testData;
            testData.latitude = -34.603722 + (loraTestCounter % 10) * 0.0001;  // Simula movimiento
            testData.longitude = -58.381592 + (loraTestCounter % 10) * 0.0001;
            testData.zone = (loraTestCounter / LORA_TEST_INTERVAL) % 5;  // Cicla entre zonas
            testData.distance = 50.0f + (loraTestCounter % 100);
            
            if (sendGpsDataToLora(&testData) == HAL_OK) {
                RTOS_LOG_INFO("[FSM_TEST] ✅ Datos LoRa enviados #%lu\n", 
                             loraTestCounter / LORA_TEST_INTERVAL);
            } else {
                RTOS_LOG_ERROR("[FSM_TEST] ❌ Error enviando datos LoRa\n");
            }
        }
        loraTestCounter++;
        // ⭐⭐⭐ FIN CÓDIGO DE PRUEBA ⭐⭐⭐

        switch (s_mainFSM) {
            case MainFSM_t::STARTUP_ROUTINE:
                runStartupRoutineFSM(s_mainFSM, &s_startupRoutineState, &msgReceived, tries, *s_cow, *s_fence);
                break;
                
            case MainFSM_t::NORMAL_OPERATION:
                runNormalOperationFSM(s_normalOpFSM, s_initializeState, s_greenZoneState, 
                                     s_stimulusZoneState, &msgReceived, tries, *s_cow, *s_fence);
                if (receivedMsgLoraRX) {
                    s_mainFSM = MainFSM_t::FENCE_TRANSITION;
                    receivedMsgLoraRX = false;
                }
                break;
                
            case MainFSM_t::FENCE_TRANSITION:
                runFenceTransitionFSM(s_mainFSM, s_fenceTransitionState, &msgReceived, tries, *s_cow, *s_fence);
                break;
        }
        
        osDelay(100);
    }
}
```

## Verificación Paso a Paso

### 1. Compilar el Proyecto

```bash
cd CM4/build
cmake --build .
```

### 2. Flashear el Device

Usa STM32CubeProgrammer o tu método preferido para flashear el `.elf` generado.

### 3. Monitorear los Logs

Conecta un UART y abre un monitor serial (115200 baud):

```
[FSM] Task initialized successfully
[LORA_TASK] Inicializando LoRaWAN...
[LORA_TASK] LoRaWAN iniciado, esperando JOIN...
JOIN SUCCESS!
[LORA_TASK] JOIN exitoso! Red LoRaWAN conectada
[FSM_TEST] ✅ Datos LoRa enviados #1
[LORA_TASK] Datos GPS recibidos: lat=-34.603722, lon=-58.381592, zone=0, dist=50.00
Sending GPS data: 17 bytes
```

### 4. Verificar en ChirpStack

En la interfaz web de ChirpStack:

1. Ve a **Applications** → **IntelliFence** → **Devices** → **Cow-Collar-001**
2. Haz click en **LoRaWAN frames**
3. Deberías ver:
   - **JoinRequest** y **JoinAccept** al inicio
   - **Uplink** cada 10 segundos con 17 bytes de payload
   - El payload decodificado mostrando lat/lon/zone

## Troubleshooting Rápido

| Problema | Solución |
|----------|----------|
| No compila | Verifica que `loraTask.cpp` esté en `CMakeLists.txt` |
| No inicia LoRa | Verifica que `loraQueueHandle` se inicialice correctamente |
| JOIN falla | Revisa las keys en `se-identity.h` y ChirpStack |
| No envía datos | Asegúrate de llamar a `sendGpsDataToLora()` |
| Payload corrupto | Verifica el orden de bytes (little endian) |

## Integración Final

Una vez probado el código de prueba:

1. **Comentar el código de prueba** en `fsmTask()`
2. **Descomentar la integración real** en los estados de la FSM
3. **Ajustar `APP_TX_DUTYCYCLE`** según tus necesidades (en `lora_app.h`)
4. **Agregar lógica de confirmación** si es necesario

## Ejemplo con Datos Reales

```cpp
// En INITIALIZE_WAIT_POSITION o similar
if (dequeuedMessage(msgReceived, fence) == HAL_OK) {
    if (updatePosition(cow, *msgReceived) == HAL_OK) {
        // Obtener datos actualizados
        Position pos = cow.getPosition();
        zone_t currentZone = cow.getCurrentZone();
        float distToLimit = cow.getDistanceToLimit();
        
        // Preparar payload
        LoraGpsData_t loraData;
        loraData.latitude = pos.latitude;
        loraData.longitude = pos.longitude;
        loraData.zone = static_cast<uint8_t>(currentZone);
        loraData.distance = distToLimit;
        
        // Enviar por LoRa
        HAL_StatusTypeDef result = sendGpsDataToLora(&loraData);
        
        if (result == HAL_OK) {
            RTOS_LOG_INFO("[FSM] 📡 Posición enviada por LoRa: %.6f, %.6f, zona=%d\n",
                         pos.latitude, pos.longitude, currentZone);
        } else {
            RTOS_LOG_WARN("[FSM] ⚠️ Cola LoRa llena, reintentando...\n");
        }
        
        initializeState = INITIALIZE_REQUEST_ZONE;
        tries = 0;
    }
}
```

## Próximos Pasos

1. **Optimizar el consumo:** Ajustar duty cycle según estado de la vaca
2. **Implementar downlink:** Recibir nuevos fences desde el servidor
3. **Agregar ACK:** Confirmar recepción de mensajes críticos
4. **Telemetría adicional:** Agregar batería, temperatura, etc.

¡Listo para probar! 🚀
