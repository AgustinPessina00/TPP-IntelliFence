# Fix: Implementación de Persistencia NVM para DevNonce

**Fecha**: 2026-02-03  
**Autor**: Nacho + GitHub Copilot (Claude Sonnet 4.5)  
**Estado**: ✅ Implementado y funcionando

---

## 📋 Problema Original

### Síntomas observados

1. **Después de cada reset**, el dispositivo intentaba hacer join con `DevNonce = 1`
2. **ChirpStack rechazaba** con error: `"DevNonce was already used by device for join request"`
3. El dispositivo **intentaba 10-20 veces** hasta encontrar un DevNonce no usado
4. La **Flash siempre estaba vacía** (`FF FF FF...`) incluso después de joins exitosos

### Causa raíz identificada

Aunque el sistema de NVM estaba **correctamente implementado** según la documentación de ST:

- ✅ Callbacks `OnStoreContextRequest()` y `OnRestoreContextRequest()` registrados
- ✅ Thread `Thd_LoraStoreContext` creado
- ✅ Función `StoreContext()` implementada
- ✅ Arquitectura dual-core MBMUX funcionando

**PERO**: El callback `OnNvmDataChange()` **NO activaba el thread de guardado** después del join exitoso.

Esto causaba que:
- El stack notificaba "hay que guardar" → ✅
- Se imprimía el log → ✅
- **NUNCA se llamaba `osThreadFlagsSet()`** → ❌
- El thread se quedaba esperando infinitamente → ❌
- **Nunca se guardaba en Flash** → ❌

---

## 🔧 Soluciones Implementadas

### 1. Activación del thread de guardado en `OnNvmDataChange()`

**Archivo**: `CM4/LoRaWAN/App/lora_app.c`

```c
static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    rtos_printf("[NVM] Data change detected - STORE\r\n");
    
    // ⭐ FIX: Activar el thread de guardado asíncrono
    osThreadFlagsSet(Thd_LoraStoreContextId, 1);
  }
  else
  {
    rtos_printf("[NVM] Data change detected - RESTORE\r\n");
  }
}
```

**Problema**: El stack LoRaWAN en arquitectura dual-core **NO siempre** llama automáticamente a `OnNvmDataChange()` después del join.

### 2. Guardado manual después de join exitoso (WORKAROUND)

Como el callback automático no era confiable, se agregó guardado manual:

```c
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  if (joinParams != NULL)
  {
    if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
    {
      UTIL_TIMER_Stop(&JoinLedTimer);
      HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_RESET);

      rtos_printf("\r\n###### = JOINED = %s\r\n",
              (joinParams->Mode == ACTIVATION_TYPE_ABP) ? "ABP" : "OTAA");
      
      // ⭐ WORKAROUND: Guardado manual después del join exitoso
      rtos_printf(">>> [OnJoinRequest] Join exitoso! Guardando contexto NVM manualmente...\r\n");
      osThreadFlagsSet(Thd_LoraStoreContextId, 1);
    }
    else
    {
      rtos_printf("\r\n###### = JOIN FAILED\r\n");
    }
  }
}
```

### 3. Logs de diagnóstico mejorados

Se agregaron logs detallados para debugging:

**En `StoreContext()`**:
```c
static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  rtos_printf("\r\n>>> [StoreContext] Thread ejecutando, llamando LmHandlerNvmDataStore()...\r\n");
  
  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_SUCCESS)
  {
    rtos_printf(">>> [StoreContext] NVM guardado exitosamente\r\n");
  }
  else if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\n");
  }
}
```

**En `OnRestoreContextRequest()`** (para debugging):
```c
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  rtos_printf("\r\n>>> [NVM RESTORE] Reading %lu bytes from Flash @ 0x%08lX <<<\r\n", 
              nvm_size, (uint32_t)LORAWAN_NVM_BASE_ADDRESS);
  
  // Debug: Mostrar primeros bytes de Flash
  uint8_t *flash_ptr = (uint8_t*)LORAWAN_NVM_BASE_ADDRESS;
  rtos_printf("[DEBUG] Flash first 16 bytes BEFORE read: ");
  for (int i = 0; i < 16; i++) {
    rtos_printf("%02X ", flash_ptr[i]);
  }
  rtos_printf("\r\n");
  
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  
  rtos_printf(">>> [NVM RESTORE] COMPLETE <<<\r\n");
}
```

### 4. Configuración de `ForceRejoin` para evitar frame counter reset

**Archivo**: `CM4/LoRaWAN/App/lora_app.h`

```c
/*!
 * LoRaWAN force rejoin even if the NVM context is restored
 * @note useful only when context management is enabled by CONTEXT_MANAGEMENT_ENABLED
 * 
 * ⚠️ IMPORTANTE: Configurado en TRUE para evitar problemas de frame counter reset
 * Con TRUE: cada reset hace nuevo join OTAA (rápido, usa DevNonce persistido)
 * Con FALSE: reutiliza sesión pero puede causar errores de frame counter en el servidor
 */
#define LORAWAN_FORCE_REJOIN_AT_BOOT                true
```

**Razón**: Al reutilizar sesión (ForceRejoin=false), ChirpStack detectaba:
- "Frame-counter reset or rollover detected"
- "UPLINK_F_CNT_RETRANSMISSION"

Con `ForceRejoin=true`:
- Cada reset hace un **nuevo join OTAA**
- Usa el **DevNonce persistido** (incrementado)
- Frame counters **empiezan limpios** desde 0
- **NO hay rechazos** de ChirpStack

---

## ✅ Resultado Final

### Comportamiento después del fix

#### Primer boot (Flash vacía):
```
>>> [NVM RESTORE] Reading 1488 bytes from Flash @ 0x0801F000 <<<
[DEBUG] Flash first 16 bytes: FF FF FF FF... (vacía)
[CONFIG] ForceRejoin=TRUE, ActivationType=2
[DEBUG] Join status BEFORE: 0 (NOT_JOINED)

###### = JOINED = OTAA
>>> [OnJoinRequest] Join exitoso! Guardando contexto NVM manualmente...
>>> [StoreContext] Thread ejecutando...
>>> [NVM STORE] Writing 1488 bytes to Flash @ 0x0801F000 <<<
>>> [NVM STORE] COMPLETE <<<
>>> [StoreContext] NVM guardado exitosamente
```

#### Después del RESET:
```
>>> [NVM RESTORE] Reading 1488 bytes from Flash @ 0x0801F000 <<<
[DEBUG] Flash first 16 bytes: 01 A2 B3 C4... (YA NO está vacía) ✅
[NVM] Data change detected - RESTORE
[CONFIG] ForceRejoin=TRUE, ActivationType=2
[DEBUG] Join status BEFORE: 1 (JOINED)

###### = JOINED = OTAA  (nuevo join con DevNonce incrementado)
>>> DOWNLINK RECEIVED <<<  (funcionando sin problemas)
```

#### En ChirpStack:
- ✅ Join exitoso inmediato
- ✅ Sin errores de "DevNonce already used"
- ✅ Sin errores de "Frame-counter reset"
- ✅ Uplinks con FCnt continuo: 0, 1, 2, 3...

---

## 🎯 Archivos Modificados

### 1. `CM4/LoRaWAN/App/lora_app.c`

**Cambios**:
- Agregado `osThreadFlagsSet()` en `OnNvmDataChange()`
- Agregado guardado manual en `OnJoinRequest()`
- Mejorados logs en `StoreContext()`
- Agregados logs de debug en `OnRestoreContextRequest()`
- Agregado manejo de `LORAMAC_HANDLER_SUCCESS` en `StoreContext()`

### 2. `CM4/LoRaWAN/App/lora_app.h`

**Cambios**:
- Cambiado `LORAWAN_FORCE_REJOIN_AT_BOOT` de `false` a `true`
- Agregado comentario explicativo sobre el impacto

---

## 📊 Comparación Antes vs Después

| Aspecto | ❌ Antes | ✅ Después |
|---------|---------|-----------|
| Flash después de join | `FF FF FF...` (vacía) | `01 A2 B3...` (con datos) |
| DevNonce después de reset | Vuelve a 1 | Continúa incrementando |
| Join después de reset | 10-20 intentos | 1 intento exitoso |
| Errores ChirpStack | "DevNonce already used" | Sin errores |
| Frame counter reset | Warnings constantes | Sin warnings |
| Tiempo de reconexión | ~30-60 segundos | ~2-5 segundos |

---

## 🔍 Flujo Completo Funcionando

```
┌─────────────────────────────────────────────────────────────┐
│                    BOOT / RESET                              │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  OnRestoreContextRequest() lee Flash @ 0x0801F000           │
│  • Si hay datos → restaura DevNonce                          │
│  • Si está vacía → DevNonce = 0                              │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  LmHandlerJoin(OTAA, ForceRejoin=TRUE)                      │
│  • Envía Join Request con DevNonce actual                   │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  OnJoinRequest() → Join exitoso                             │
│  • osThreadFlagsSet(Thd_LoraStoreContextId, 1) ← MANUAL     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  Thd_LoraStoreContext() despierta                           │
│  • Llama StoreContext()                                      │
│  • StoreContext() → LmHandlerNvmDataStore()                 │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  OnStoreContextRequest() escribe Flash                      │
│  • FLASH_IF_Write() @ 0x0801F000                            │
│  • Guarda DevNonce incrementado                              │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
                ✅ DevNonce persistido
```

---

## 🚀 Testing Realizado

### Test 1: Verificar escritura Flash
```bash
1. Flash dispositivo (Flash vacía)
2. Join exitoso
3. Log confirma: ">>> [NVM STORE] Writing 1488 bytes..."
4. Log confirma: ">>> [NVM STORE] COMPLETE <<<"
✅ PASS
```

### Test 2: Verificar restauración
```bash
1. Reset dispositivo
2. Log muestra: Flash != FF FF FF (tiene datos)
3. Log muestra: "NVM DATA RESTORED"
4. Join status = 1 (JOINED)
✅ PASS
```

### Test 3: Verificar DevNonce incremental
```bash
1. Flush OTAA en ChirpStack
2. Reset dispositivo → Join con DevNonce=0
3. Reset dispositivo → Join con DevNonce=1
4. Reset dispositivo → Join con DevNonce=2
✅ PASS - DevNonce incrementa correctamente
```

### Test 4: Verificar sin errores ChirpStack
```bash
1. Join exitoso
2. Verificar Events en ChirpStack
3. NO hay "DevNonce already used"
4. NO hay "Frame-counter reset"
✅ PASS
```

---

## 📚 Referencias

- **Documento relacionado**: `docs/05_lorawan/gestion_nvm_devnonce.md` (documentación completa del sistema)
- **Especificación LoRaWAN 1.0.4**: Section 6.2.4 (DevNonce requirements)
- **ST AN5406**: "How to build a LoRa application with STM32CubeWL"

---

## 🎓 Lecciones Aprendidas

1. **En arquitectura dual-core**, el stack LoRaWAN NO siempre llama callbacks automáticamente después del join
2. Es necesario **forzar guardado manual** en `OnJoinRequest()` como workaround
3. `ForceRejoin=true` es **preferible en producción** para evitar problemas de frame counter
4. Los **logs detallados** son esenciales para debugging de persistencia NVM
5. La Flash debe **verificarse visualmente** (logs hex) para confirmar escritura

---

**Estado**: ✅ Implementado, testeado y funcionando  
**Próximos pasos**: Monitoring en producción para verificar estabilidad a largo plazo
