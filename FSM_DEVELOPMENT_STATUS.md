# FSM Task - Estado de Desarrollo

## Estado Actual: EN DESARROLLO

La implementación de la máquina de estados finitos (FSM Task) está parcialmente completada.

### ✅ Completado

- **Arquitectura base de la FSM**:
  - Main FSM con 3 estados: STARTUP_ROUTINE, NORMAL_OPERATION, FENCE_TRANSITION
  - Sub-FSMs: Initialize, GreenZone, StimulusZone
  - Todas las transiciones de estado implementadas

- **Integración con EmbeddedMessage**:
  - Todo el sistema de mensajería migrado a pool estático (sin malloc/free)
  - Todas las funciones helper actualizadas para usar `EmbeddedMessage_t*`
  - Header `fsmTask.h` sincronizado con tipos embedded

- **Funciones Helper**:
  - `sendMessage()` - Envío de mensajes simples
  - `dequeuedMessage()` - Recepción de mensajes
  - `updatePosition()` - Procesamiento de GPS
  - `updateAcceleration()` - Procesamiento de IMU
  - `sendPosition()` - Envío de posición a LoRa
  - `updateGpsAdqTime()` - Configuración de frecuencia GPS
  - `classifyMotion()` - Clasificación de estado de vaca (Sleep/Grazing/Movement)
  - Y más...

### ⚠️ Pendiente de Implementación

- **Integración con Cow y Fence**:
  ```cpp
  // TODO: Descomentar cuando estén listos
  // fsmTaskParams *fsmParams = static_cast<fsmTaskParams *>(argument);
  // fsmParams->cow->updatePosition(...)
  // fsmParams->fence->createLimits()
  ```

- **Módulos faltantes**:
  - `MODULE_DISTANCE` - Cálculo de distancia al límite del cerco
  - `MODULE_LORA_TX` / `MODULE_LORA_RX` - Comunicación LoRa
  - `MODULE_STIMULUS` - Control de estímulos
  - `MODULE_GPS` - Gestión de frecuencia de adquisición GPS

- **Testing end-to-end**:
  - Validar transiciones entre estados
  - Verificar timeouts y reintentos (MAX_TRIES)
  - Confirmar lógica de zonas (GREEN/ORANGE/RED/BLACK)

### 🔧 Próximos Pasos

1. Implementar clases `Cow` y `Fence` embedded-friendly (sin STL)
2. Crear módulos pendientes (DISTANCE, LORA, STIMULUS)
3. Testing completo con hardware real
4. Optimizar consumo energético en estados SLEEP/GRAZING

### 📝 Notas Técnicas

- **Stack usage**: FSM Task configurado con 1KB stack
- **Message pool**: 16 slots disponibles en fsmQueueHandle
- **Logging**: RTOS_LOG_* macros thread-safe para debugging
- **Timeouts**: MAX_TRIES = 10 para operaciones críticas

---

**Última actualización**: 2025-11-19  
**Responsable**: TPP-IntelliFence Team
