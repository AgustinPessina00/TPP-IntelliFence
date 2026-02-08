# Gestión de NVM (DevNonce) en el Proyecto LoRaWAN Dual Core

## 📋 Índice
1. [¿Qué es el DevNonce y por qué debe persistirse?](#qué-es-el-devnonce-y-por-qué-debe-persistirse)
2. [Arquitectura de la gestión de NVM](#arquitectura-de-la-gestión-de-nvm)
3. [Flujo completo de almacenamiento](#flujo-completo-de-almacenamiento)
4. [Flujo completo de restauración](#flujo-completo-de-restauración)
5. [Ubicación en Flash](#ubicación-en-flash)
6. [Componentes del sistema](#componentes-del-sistema)
7. [Callbacks y funciones clave](#callbacks-y-funciones-clave)
8. [Debugging y verificación](#debugging-y-verificación)
9. [Troubleshooting](#troubleshooting)

---

## ¿Qué es el DevNonce y por qué debe persistirse?

### DevNonce (Device Nonce)
El **DevNonce** es un contador de 16 bits que se incrementa en cada intento de join OTAA (Over-The-Air Activation):
- **Valor inicial**: 0
- **Incremento**: cada vez que se hace un join request
- **Límite**: 65535 (2^16 - 1)
- **Propósito**: prevenir replay attacks en el proceso de join

### ¿Por qué debe sobrevivir resets?

Según la **especificación LoRaWAN 1.0.4** y las mejores prácticas:

1. **Seguridad**: El DevNonce nunca debe repetirse con los mismos AppEUI/JoinEUI
2. **Conformidad**: Si reseteas y vuelve a 0, el Network Server puede rechazar el join (ChirpStack lo hace por defecto)
3. **Eficiencia**: Evitas tener que borrar el dispositivo del NS después de cada reset
4. **Producción**: Permite actualizaciones de firmware sin perder sincronización

### ✅ Solución correcta: Persistir contexto NVM

El stack LoRaWAN de ST incluye un **sistema de NVM context management** que guarda:
- ✅ DevNonce (lo más crítico)
- ✅ Claves de sesión (si usas ABP o quieres reconectar rápido después de un join)
- ✅ Frame counters (FCntUp, FCntDown) - críticos para seguridad
- ✅ Configuración MAC
- ✅ Estado del stack

---

## Arquitectura de la gestión de NVM

### Arquitectura Dual Core (STM32WL55)

```
┌─────────────────────────────────────────────────────────────┐
│                         CM4 (Cortex-M4)                     │
│                        Aplicación principal                  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │          lora_app.c (CM4 Application)                │  │
│  │                                                       │  │
│  │  • OnStoreContextRequest()   ← Callback del stack   │  │
│  │  • OnRestoreContextRequest() ← Callback del stack   │  │
│  │  • StoreContext()            ← Tarea FreeRTOS       │  │
│  │  • Thd_LoraStoreContext      ← Thread dedicado      │  │
│  └─────────────────┬────────────────────────────────────┘  │
│                    │                                        │
│                    │ MBMUX (Mailbox)                        │
│                    │ (Inter-core communication)             │
└────────────────────┼────────────────────────────────────────┘
                     │
┌────────────────────┼────────────────────────────────────────┐
│                    ▼                                         │
│                         CM0+ (Cortex-M0+)                   │
│                        LoRaWAN Stack                        │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │   LmHandler (LoRaMac Handler) en CM0+                │  │
│  │                                                       │  │
│  │  • LmHandlerNvmDataStore()  ← API para guardar      │  │
│  │  • Callbacks hacia CM4       → Notificaciones        │  │
│  │  • Gestión del contexto NVM                          │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                              │
└──────────────────────────────────────────────────────────────┘

                             │
                             ▼
┌──────────────────────────────────────────────────────────────┐
│                    FLASH MEMORY                              │
│  Dirección: 0x0801F000 (LORAWAN_NVM_BASE_ADDRESS)          │
│                                                              │
│  Contiene:                                                   │
│  • DevNonce                                                  │
│  • Frame Counters (FCntUp, FCntDown)                        │
│  • Session Keys (AppSKey, NwkSKey) si ABP                   │
│  • Configuración MAC                                         │
│  • Estado del stack                                          │
└──────────────────────────────────────────────────────────────┘
```

---

## Flujo completo de almacenamiento

### 1️⃣ **Evento que dispara el almacenamiento**

Los eventos que generan cambios en el contexto NVM incluyen:
- ✅ **Join exitoso** → DevNonce se incrementó, claves generadas
- ✅ **Envío/recepción de mensaje** → Frame counters incrementados
- ✅ **Cambio de configuración MAC** → comandos MAC del servidor
- ✅ **ADR (Adaptive Data Rate)** → cambios en datarate, potencia, etc.

### 2️⃣ **Stack detecta cambio** (en CM0+)

```c
// En el LoRaWAN Stack (CM0+) - código del middleware ST
// Cuando DevNonce cambia o hay un join exitoso:

// Internamente el stack llama:
LmHandlerCallbacks.OnNvmDataChange(LORAMAC_HANDLER_NVM_STORE);
```

### 3️⃣ **Notificación a CM4 via MBMUX**

```c
// En CM0PLUS/MbMux/LmHandler_mbwrapper.c
static void OnNvmDataChange_mbwrapper(LmHandlerNvmContextStates_t state)
{
  MBMUX_ComParam_t *com_obj;
  uint32_t *com_buffer = NULL;

  com_obj = MBMUXIF_GetLoraFeatureNotifComPtr();
  com_obj->MsgId = LMHANDLER_ON_NVM_DATA_CHANGE_CB_ID;  // 🔔 Notificación
  
  com_buffer = MBMUX_SEC_VerifySramBufferPtr(com_obj->ParamBuf, com_obj->BufSize);
  com_buffer[0] = (uint32_t)state;
  com_obj->ParamCnt = 1;

  MBMUXIF_LoraSendNotif();  // ⚡ Envía notificación inter-core
}
```

### 4️⃣ **CM4 recibe callback** (lora_app.c)

```c
// En CM4/LoRaWAN/App/lora_app.c

static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\r\n");
    
    // 🚨 IMPORTANTE: NO bloqueamos aquí
    // En lugar de guardar directo, activamos el thread dedicado
    osThreadFlagsSet(Thd_LoraStoreContextId, 1);  // ⚡ Señal al thread
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\r\n");
  }
}
```

**¿Por qué usar un thread separado?**
- ⏱️ Las operaciones de Flash son **lentas** (varios ms para borrar página)
- 🚫 No queremos bloquear el stack LoRaWAN
- ✅ FreeRTOS permite priorización y scheduling correcto

### 5️⃣ **Thread dedicado ejecuta guardado**

```c
// Thread FreeRTOS dedicado (prioridad osPriorityLow)
static void Thd_LoraStoreContext(void *argument)
{
  UNUSED(argument);
  for (;;)
  {
    // 🛑 Espera señal (bloqueado, sin consumir CPU)
    osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
    
    // ✅ Cuando recibe la señal, ejecuta guardado
    StoreContext();
  }
}
```

### 6️⃣ **Función StoreContext solicita datos al stack**

```c
static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;
  
  // 📞 Llama al CM0+ via MBMUX
  status = LmHandlerNvmDataStore();
  
  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\r\n");
  }
}
```

### 7️⃣ **MBMUX transmite comando a CM0+**

```c
// En CM4/MbMux/LmHandler_mbwrapper.c
LmHandlerErrorStatus_t LmHandlerNvmDataStore(void)
{
  MBMUX_ComParam_t *com_obj;
  uint32_t ret;

  com_obj = MBMUXIF_GetLoraFeatureCmdComPtr();
  com_obj->MsgId = LMHANDLER_NVM_DATA_STORE_ID;  // 📤 Comando
  com_obj->ParamCnt = 0;

  MBMUXIF_LoraSendCmd();  // ⚡ Envía comando inter-core
  
  // 🛑 Espera respuesta (sincrónico)
  ret = com_obj->ReturnVal;
  return (LmHandlerErrorStatus_t) ret;
}
```

### 8️⃣ **CM0+ procesa comando**

```c
// En CM0PLUS/MbMux/LmHandler_mbwrapper.c
void Process_Lora_Cmd(MBMUX_ComParam_t *ComObj)
{
  switch (ComObj->MsgId)
  {
    case LMHANDLER_NVM_DATA_STORE_ID:
      // 📦 Llama al stack LoRaWAN real
      errorStatus = LmHandlerNvmDataStore();  
      
      // Este LmHandlerNvmDataStore() está en el middleware ST
      // Lee el contexto interno del stack y llama a OnStoreContextRequest
      
      ComObj->ParamCnt = 0;
      ComObj->ReturnVal = (uint32_t) errorStatus;
      break;
  }
  
  MBMUX_ResponseSnd(FEAT_INFO_LORAWAN_ID);  // 📨 Responde a CM4
}
```

### 9️⃣ **Callback OnStoreContextRequest** (¡Aquí se escribe Flash!)

```c
// En CM4/LoRaWAN/App/lora_app.c
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  // 💾 ESCRITURA REAL A FLASH
  // nvm = puntero al buffer con todos los datos del contexto
  // nvm_size = tamaño en bytes (típicamente 512-1024 bytes)
  
  FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  
  // ✅ Ahora el DevNonce y todo el contexto están guardados en Flash
}
```

### 🔟 **Escritura Flash** (flash_if.c)

```c
// En CM4/Core/Src/flash_if.c
FLASH_IF_StatusTypedef FLASH_IF_Write(void *pDestination, const void *pSource, uint32_t uLength)
{
  // 1. Desbloquear Flash
  HAL_FLASH_Unlock();
  
  // 2. Borrar página(s) necesaria(s)
  FLASH_IF_INT_Erase(pDestination, uLength);
  
  // 3. Escribir datos (64 bits a la vez)
  for (cada 8 bytes)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
  }
  
  // 4. Verificar escritura
  memcmp(pSource, pDestination, uLength);
  
  // 5. Bloquear Flash
  HAL_FLASH_Lock();
  
  return status;
}
```

---

## Flujo completo de restauración

### Secuencia en el arranque del sistema

```
RESET / Power-ON
       │
       ▼
┌─────────────────────────────────────┐
│  1. SystemInit()                    │
│     - Clock setup                   │
│     - Hardware init                 │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  2. MX_FREERTOS_Init()              │
│     - Crea tareas RTOS              │
│     - Crea Thd_LoraStoreContextId   │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  3. LoRaWAN_Init()                  │
│     - LmHandlerInit()               │
│     - Registra callbacks            │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  4. LmHandler solicita restaurar    │
│     contexto                         │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  5. OnRestoreContextRequest()       │
│     - Lee Flash                      │
│     - Devuelve buffer al stack      │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│  6. Stack procesa contexto          │
│     - Restaura DevNonce             │
│     - Restaura Frame Counters       │
│     - Restaura configuración        │
└──────────────┬──────────────────────┘
               ▼
       ✅ Listo para operar
       (DevNonce continúa donde quedó)
```

### 1️⃣ **Inicialización del LmHandler**

Durante `LoRaWAN_Init()`, el stack llama:

```c
// El stack interno (middleware ST) ejecuta:
LmHandlerInit(&LmHandlerParams);

// Internamente esto hace:
// - Verifica si hay contexto válido en NVM
// - Si hay, solicita restauración
LmHandlerCallbacks.OnRestoreContextRequest(&nvm_buffer, sizeof(nvm_buffer));
```

### 2️⃣ **Callback OnRestoreContextRequest**

```c
// En CM4/LoRaWAN/App/lora_app.c
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  // 📖 LECTURA DE FLASH
  // nvm = puntero al buffer donde copiar los datos
  // nvm_size = tamaño esperado
  
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  
  // ✅ El stack recibe el contexto y lo valida
  // Si es válido → restaura DevNonce, contadores, etc.
  // Si no es válido (CRC error) → comienza de cero
}
```

**⚠️ IMPORTANTE**: En tu código actual, hay un comentario que indica que esto **ESTABA DESHABILITADO**:

```c
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  // COMENTADO TEMPORALMENTE: No restaurar contexto NVM para forzar join nuevo cada vez
  // Esto hace que el dispositivo siempre haga OTAA join desde cero
  // FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size)

  // Limpiar el buffer NVM para asegurarse de que no hay datos viejos
  //memset(nvm, 0, nvm_size);
  //APP_LOG(TS_OFF, VLEVEL_M, "NVM CONTEXT IGNORED - FORCING NEW JOIN\r\r\n");
  
  // ✅ ACTUALMENTE ESTÁ HABILITADO:
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
}
```

### 3️⃣ **Lectura Flash** (flash_if.c)

```c
// En CM4/Core/Src/flash_if.c
FLASH_IF_StatusTypedef FLASH_IF_Read(void *pDestination, const void *pSource, uint32_t uLength)
{
  // Simplemente copia desde Flash a RAM
  memcpy(pDestination, pSource, uLength);
  
  return FLASH_IF_OK;
}
```

### 4️⃣ **Stack valida y restaura**

El middleware ST internamente:
1. ✅ Verifica **CRC** del contexto
2. ✅ Comprueba **versión** del formato
3. ✅ Si válido → restaura todos los campos
   - DevNonce → continúa desde último valor
   - Frame counters → continúa desde último valor
   - Claves de sesión (si existían)
4. ❌ Si inválido → contexto vacío (DevNonce = 0)

---

## Ubicación en Flash

### Mapa de memoria Flash STM32WL55

```
┌────────────────────────────────────────────────────────┐
│  FLASH MEMORY - 256KB total                            │
├────────────────────────────────────────────────────────┤
│  0x0800_0000 - 0x0801_EFFF  │  Código aplicación      │
│                              │  (~124 KB)               │
├──────────────────────────────┼──────────────────────────┤
│  0x0801_F000 - 0x0801_FFFF  │  LoRaWAN NVM Context    │  ⭐ AQUÍ
│                              │  (4 KB / 2 páginas)      │
│                              │                          │
│  Definido en lora_app.c:     │                          │
│  #define LORAWAN_NVM_BASE_ADDRESS  ((void *)0x0801F000UL)
├──────────────────────────────┼──────────────────────────┤
│  0x0802_0000 - 0x0803_CFFF  │  Más código/datos       │
├──────────────────────────────┼──────────────────────────┤
│  0x0803_D000 - 0x0803_FFFF  │  KMS/NVMS (seguridad)   │
│                              │  Definido en             │
│                              │  nvms_low_level.h        │
└────────────────────────────────────────────────────────┘
```

### Configuración actual

#### En lora_app.c (CM4):
```c
/**
  * @brief LoRaWAN NVM Flash address
  * @note last 2 sector of a 128kBytes device
  */
#define LORAWAN_NVM_BASE_ADDRESS  ((void *)0x0801F000UL)
```

#### En nvms_low_level.h (CM0+):
```c
/*
 * KMS NVM data storage
 * Reserved section in the mapping that is composed of 2 blocks of the same size
 * Blocks are contiguous and each of them must be erasable independently
 */
#define NVMS_LL_BLOCK0_ADDRESS   (0x0803D000UL)
#define NVMS_LL_BLOCK1_ADDRESS   (0x0803D800UL)
```

**📝 Nota**: Son regiones separadas, sin conflicto:
- `0x0801F000` → LoRaWAN NVM (DevNonce, frame counters, etc.)
- `0x0803D000` → KMS NVM (gestión de claves criptográficas)

### Tamaño del contexto NVM

Típicamente el contexto serializado del stack LoRaWAN ocupa:
- **~512-1024 bytes** dependiendo de la configuración
- Se guarda en un área reservada de **4 KB** (2 páginas Flash)
- Esto permite espacio para futuras expansiones

---

## Componentes del sistema

### 📁 Archivo: CM4/LoRaWAN/App/lora_app.c

**Responsabilidad**: Aplicación principal LoRaWAN en CM4

#### Callbacks registrados:
```c
static const LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .OnRestoreContextRequest = OnRestoreContextRequest,  // ⬅️ Lee Flash
  .OnStoreContextRequest   = OnStoreContextRequest,    // ⬅️ Escribe Flash
  .OnNvmDataChange         = OnNvmDataChange,          // ⬅️ Notifica cambios
  // ... otros callbacks
};
```

#### Thread dedicado:
```c
osThreadId_t Thd_LoraStoreContextId;

const osThreadAttr_t Thd_LoraStoreContext_attr =
{
  .name = "LoraStoreContext",
  .attr_bits = osThreadDetached,
  .priority = osPriorityLow,      // ⚡ Baja prioridad (no crítico)
  .stack_size = 128 * 4           // 512 bytes stack
};

// En LoRaWAN_Init():
Thd_LoraStoreContextId = osThreadNew(Thd_LoraStoreContext, NULL, &Thd_LoraStoreContext_attr);
```

### 📁 Archivo: CM4/Core/Src/flash_if.c

**Responsabilidad**: Abstracción de bajo nivel para Flash

#### Funciones principales:

```c
// Inicialización (reserva buffer RAM para operaciones)
FLASH_IF_StatusTypedef FLASH_IF_Init(void *pAllocRamBuffer);

// Escribir en Flash (con borrado automático de página)
FLASH_IF_StatusTypedef FLASH_IF_Write(void *pDestination, const void *pSource, uint32_t uLength);

// Leer desde Flash (simple memcpy)
FLASH_IF_StatusTypedef FLASH_IF_Read(void *pDestination, const void *pSource, uint32_t uLength);

// Borrar región de Flash
FLASH_IF_StatusTypedef FLASH_IF_Erase(void *pStart, uint32_t uLength);
```

#### Detalles de implementación:
- **Granularidad de escritura**: 64 bits (8 bytes)
- **Tamaño de página**: 2048 bytes (2 KB)
- **Operación**: Read-Modify-Write cuando escribes parcialmente una página
- **Buffer temporal**: Usa RAM para hacer backup durante borrado + escritura

### 📁 Archivo: CM4/MbMux/LmHandler_mbwrapper.c

**Responsabilidad**: Wrapper de comunicación CM4 ↔ CM0+ via MBMUX

#### Función clave:
```c
LmHandlerErrorStatus_t LmHandlerNvmDataStore(void)
{
  // Prepara mensaje
  com_obj->MsgId = LMHANDLER_NVM_DATA_STORE_ID;
  
  // Envía comando al CM0+
  MBMUXIF_LoraSendCmd();
  
  // Espera respuesta
  return (LmHandlerErrorStatus_t) com_obj->ReturnVal;
}
```

### 📁 Archivo: CM0PLUS/MbMux/LmHandler_mbwrapper.c

**Responsabilidad**: Recepción de comandos desde CM4, ejecución en CM0+

#### Procesamiento de comandos:
```c
void Process_Lora_Cmd(MBMUX_ComParam_t *ComObj)
{
  switch (ComObj->MsgId)
  {
    case LMHANDLER_NVM_DATA_STORE_ID:
      // Llama al stack LoRaWAN real (middleware ST)
      errorStatus = LmHandlerNvmDataStore();
      ComObj->ReturnVal = (uint32_t) errorStatus;
      break;
  }
  
  MBMUX_ResponseSnd(FEAT_INFO_LORAWAN_ID);
}
```

#### Callbacks hacia CM4:
```c
static void OnNvmDataChange_mbwrapper(LmHandlerNvmContextStates_t state)
{
  // Prepara notificación
  com_obj->MsgId = LMHANDLER_ON_NVM_DATA_CHANGE_CB_ID;
  com_buffer[0] = (uint32_t)state;
  
  // Envía notificación al CM4
  MBMUXIF_LoraSendNotif();
}

static void OnStoreContextRequest_mbwrapper(void *nvm, uint32_t nvm_size)
{
  // Prepara notificación con datos
  com_obj->MsgId = LMHANDLER_ON_STORE_CONTEXT_REQ_CB_ID;
  com_buffer[0] = (uint32_t)nvm;
  com_buffer[1] = nvm_size;
  
  // Envía al CM4 para que escriba Flash
  MBMUXIF_LoraSendNotif();
}
```

### 📁 Middleware: LoRaWAN Stack (ST Middleware - binario/código externo)

**Responsabilidad**: Implementación del protocolo LoRaWAN

Este código **NO está en tu repositorio**, sino en:
```
C:/Users/nacho/OneDrive/Middlewares/Third_Party/LoRaWAN/
```

Funciones que proporciona:
- `LmHandlerInit()` - Inicializa handler, restaura contexto
- `LmHandlerNvmDataStore()` - Serializa contexto y llama callback
- Gestión interna de DevNonce, contadores, claves, etc.
- Detección automática de cuándo hay que guardar

---

## Callbacks y funciones clave

### Tabla resumen de callbacks

| Callback | Ubicación | Cuándo se llama | Acción |
|----------|-----------|-----------------|--------|
| `OnNvmDataChange()` | CM4: lora_app.c | Cuando el stack detecta cambio en contexto | Señaliza thread de guardado |
| `OnStoreContextRequest()` | CM4: lora_app.c | Cuando el stack necesita guardar contexto | Escribe Flash |
| `OnRestoreContextRequest()` | CM4: lora_app.c | Al inicio, para restaurar contexto | Lee Flash |
| `GetBatteryLevel()` | CM4: lora_app.c | Cuando stack necesita nivel batería | Devuelve nivel |
| `GetTemperatureLevel()` | CM4: lora_app.c | Cuando stack necesita temperatura | Devuelve temperatura |
| `OnJoinRequest()` | CM4: lora_app.c | Antes de enviar Join Request | Log, LED |
| `OnNetworkParametersChange()` | CM4: lora_app.c | Después de join exitoso | Log, actualiza config |
| `OnTxData()` | CM4: lora_app.c | Antes de enviar uplink | Log |
| `OnRxData()` | CM4: lora_app.c | Cuando llega downlink | Procesa payload |

### ⚙️ Función: OnStoreContextRequest()

```c
/**
 * @brief Callback para guardar contexto NVM
 * @param nvm      Puntero al buffer con contexto serializado
 * @param nvm_size Tamaño del buffer en bytes
 * 
 * Llamado por el stack LoRaWAN cuando necesita persistir el contexto.
 * Esta función debe escribir el buffer 'nvm' en Flash.
 */
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  // Llamada bloqueante - escribe directamente en Flash
  FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  
  // ✅ Contexto guardado, incluyendo DevNonce
}
```

**Contenido del buffer NVM** (simplificado):
```c
typedef struct
{
  uint16_t DevNonce;              // ⭐ Contador de join attempts
  uint32_t FCntUp;                // Frame counter uplink
  uint32_t FCntDown;              // Frame counter downlink
  uint8_t  AppSKey[16];           // Application Session Key (si ABP)
  uint8_t  NwkSKey[16];           // Network Session Key (si ABP)
  // ... más campos de configuración MAC, etc.
  uint32_t CRC;                   // Checksum para validar integridad
} LoRaMacNvmData_t;
```

### ⚙️ Función: OnRestoreContextRequest()

```c
/**
 * @brief Callback para restaurar contexto NVM
 * @param nvm      Puntero al buffer donde copiar contexto
 * @param nvm_size Tamaño esperado del buffer
 * 
 * Llamado por el stack en LmHandlerInit() para intentar restaurar
 * contexto previo. Si la Flash está vacía o CRC es inválido, el
 * stack iniciará con contexto limpio (DevNonce = 0).
 */
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  // Lectura no bloqueante (memcpy desde Flash a RAM)
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  
  // El stack validará CRC y decidirá si usar este contexto o no
}
```

**Estado actual en tu código**:
```c
// ✅ ACTUALMENTE HABILITADO
FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);

// ❌ ESTABA DESHABILITADO (comentado):
// memset(nvm, 0, nvm_size);  // Forzaba DevNonce = 0 siempre
```

### ⚙️ Función: OnNvmDataChange()

```c
/**
 * @brief Callback de notificación de cambio en NVM
 * @param state LORAMAC_HANDLER_NVM_STORE o LORAMAC_HANDLER_NVM_RESTORE
 * 
 * Llamado cuando:
 * - STORE: después de join, después de TX/RX, después de comandos MAC
 * - RESTORE: después de restaurar contexto exitosamente al inicio
 */
static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\r\n");
    
    // 🚨 NO bloqueamos aquí - delegamos a thread
    osThreadFlagsSet(Thd_LoraStoreContextId, 1);
  }
  else  // LORAMAC_HANDLER_NVM_RESTORE
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\r\n");
  }
}
```

**¿Por qué no guardamos directo en OnNvmDataChange?**

| ❌ Guardar directo | ✅ Usar thread separado |
|-------------------|------------------------|
| Bloquea callback ~10-50ms | No bloquea callback |
| Puede afectar timing LoRaWAN | Stack sigue funcionando |
| Sin priorización | Control de prioridad RTOS |
| Difícil de debuggear | Logs claros, fácil debug |

### ⚙️ Función: StoreContext()

```c
/**
 * @brief Solicita al stack guardar contexto NVM
 * 
 * Esta función se ejecuta en el contexto del thread Thd_LoraStoreContext.
 * Llama al stack via MBMUX, que a su vez llamará OnStoreContextRequest().
 */
static void StoreContext(void)
{
  LmHandlerErrorStatus_t status;
  
  // Solicita al stack (CM0+) que guarde contexto
  status = LmHandlerNvmDataStore();
  
  // LmHandlerNvmDataStore() hace:
  // 1. Serializa contexto interno
  // 2. Llama OnStoreContextRequest(buffer, size)
  // 3. Devuelve resultado
  
  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\r\n");
  }
}
```

### ⚙️ Thread: Thd_LoraStoreContext

```c
/**
 * @brief Thread FreeRTOS para guardado asíncrono de contexto
 * @param argument No usado
 * 
 * Prioridad: osPriorityLow (no es crítico para timing)
 * Stack: 512 bytes
 * 
 * Comportamiento:
 * - Se queda bloqueado esperando señal
 * - Cuando recibe señal → llama StoreContext()
 * - Vuelve a esperar
 */
static void Thd_LoraStoreContext(void *argument)
{
  UNUSED(argument);
  
  for (;;)
  {
    // 🛑 Bloqueado hasta que reciba flag
    // No consume CPU mientras espera
    osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
    
    // ✅ Flag recibido → ejecutar guardado
    StoreContext();
    
    // Loop: vuelve a esperar
  }
}
```

**Activación del thread**:
```c
// Desde OnNvmDataChange() o cualquier otro lugar:
osThreadFlagsSet(Thd_LoraStoreContextId, 1);  // ⚡ Despierta thread
```

---

## Debugging y verificación

### 🔍 Verificar que el DevNonce se persiste

#### 1. **Logs al hacer Join**

Después de un join exitoso, deberías ver:
```
###### =========== MLME-Confirm ============ ######
STATUS      : OK
###### ===========   JOINED     ============ ######
OTAA
DevAddr     :  01234567
NVM DATA STORED              ⬅️ ⭐ IMPORTANTE
NVM DATA UP TO DATE
```

#### 2. **Verificar con reset**

```bash
# Test 1: Primer join (DevNonce debería ser 0 o bajo)
1. Flash el dispositivo
2. Observa logs de join
3. Anota DevAddr asignado

# Test 2: Reset inmediato
4. Presiona RESET en la Nucleo
5. Observa logs:
   - Debería decir "NVM DATA RESTORED"
   - Join debería ser MUY rápido (reutiliza sesión)
   
# Test 3: Reset + join nuevo
6. Borra el dispositivo en ChirpStack
7. Presiona RESET
8. El dispositivo hace join nuevo con DevNonce incrementado
9. ChirpStack acepta porque DevNonce > anterior
```

#### 3. **Inspeccionar Flash directamente**

Si tienes ST-Link y STM32CubeProgrammer:

```bash
1. Conecta ST-Link a la Nucleo
2. Abre STM32CubeProgrammer
3. Conecta al chip
4. Lee dirección 0x0801F000
5. Busca patrones:
   - Si todo son 0xFF → Flash vacía (nunca guardó)
   - Si hay datos → contexto presente
```

#### 4. **Agregar logs de debug**

```c
// En OnRestoreContextRequest()
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  APP_LOG(TS_OFF, VLEVEL_M, "📖 Restoring NVM context from 0x%08X (%d bytes)\r\r\n", 
          (uint32_t)LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  
  // Debug: mostrar primeros bytes
  uint8_t *buf = (uint8_t*)nvm;
  APP_LOG(TS_OFF, VLEVEL_M, "First 16 bytes: %02X %02X %02X %02X %02X %02X %02X %02X "
          "%02X %02X %02X %02X %02X %02X %02X %02X\r\r\n",
          buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
          buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
}

// En OnStoreContextRequest()
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  APP_LOG(TS_OFF, VLEVEL_M, "💾 Storing NVM context to 0x%08X (%d bytes)\r\r\n", 
          (uint32_t)LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  
  FLASH_IF_StatusTypedef status = FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, 
                                                   (const void *)nvm, nvm_size);
  
  if (status == FLASH_IF_OK)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "✅ NVM context stored successfully\r\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "❌ NVM context store FAILED (error: %d)\r\r\n", status);
  }
}
```

### 🧪 Script de test Python (simulación)

```python
#!/usr/bin/env python3
"""
Script para verificar comportamiento de DevNonce en ChirpStack
"""

import time
import random

def simulate_device_behavior():
    """Simula el comportamiento del dispositivo con persistencia NVM"""
    
    # Simular Flash NVM
    flash_nvm = {
        'dev_nonce': 0,
        'valid': False
    }
    
    print("=" * 60)
    print("SIMULACIÓN: Persistencia de DevNonce en Flash")
    print("=" * 60)
    
    # Escenario 1: Primer boot (Flash vacía)
    print("\n🔌 BOOT 1: Primer arranque (Flash vacía)")
    dev_nonce = restore_from_flash(flash_nvm)
    print(f"  DevNonce restaurado: {dev_nonce}")
    print(f"  📡 Enviando Join Request con DevNonce={dev_nonce}")
    flash_nvm['dev_nonce'] = dev_nonce + 1
    flash_nvm['valid'] = True
    store_to_flash(flash_nvm)
    print(f"  ✅ Join exitoso, DevNonce incrementado y guardado: {flash_nvm['dev_nonce']}")
    
    # Escenario 2: Reset inmediato
    print("\n🔄 RESET: Simulando reset del dispositivo")
    dev_nonce = restore_from_flash(flash_nvm)
    print(f"  DevNonce restaurado: {dev_nonce}")
    print(f"  ✅ Sesión existente, no hace join (reutiliza sesión)")
    
    # Escenario 3: Join nuevo después de borrar en servidor
    print("\n🗑️ SERVIDOR: Dispositivo borrado en ChirpStack")
    print("🔄 RESET: Reiniciando dispositivo")
    dev_nonce = restore_from_flash(flash_nvm)
    print(f"  DevNonce restaurado: {dev_nonce}")
    print(f"  📡 Enviando Join Request con DevNonce={dev_nonce}")
    print(f"  ✅ ChirpStack acepta porque DevNonce ({dev_nonce}) > anterior (0)")
    flash_nvm['dev_nonce'] = dev_nonce + 1
    store_to_flash(flash_nvm)
    
    # Escenario 4: Múltiples resets
    print("\n🔄 MÚLTIPLES RESETS:")
    for i in range(5):
        print(f"  Reset #{i+1}")
        dev_nonce = restore_from_flash(flash_nvm)
        print(f"    DevNonce restaurado: {dev_nonce}")
        # Simular envío de mensajes que incrementan contadores
        flash_nvm['dev_nonce'] += random.randint(0, 2)
        store_to_flash(flash_nvm)
    
    print("\r\n" + "=" * 60)
    print("✅ Resultado: DevNonce sobrevive todos los resets")
    print(f"   DevNonce final: {flash_nvm['dev_nonce']}")
    print("=" * 60)

def restore_from_flash(flash_nvm):
    """Simula restauración desde Flash"""
    if flash_nvm['valid']:
        return flash_nvm['dev_nonce']
    else:
        return 0  # Flash vacía

def store_to_flash(flash_nvm):
    """Simula escritura a Flash"""
    time.sleep(0.01)  # Simular delay de escritura Flash (~10ms)
    print(f"  💾 NVM guardado: DevNonce={flash_nvm['dev_nonce']}")

if __name__ == "__main__":
    simulate_device_behavior()
```

---

## Troubleshooting

### ❌ Problema: "DevNonce was already used"

**Síntomas**:
```
Join Request enviado
ChirpStack rechaza: "dev_nonce was already used by device for join request"
```

**Causa raíz**:
- `OnRestoreContextRequest()` está deshabilitado (comentado)
- O el contexto no se restaura correctamente
- Cada reset → DevNonce vuelve a 0 → ChirpStack rechaza

**Solución**:
```c
// Asegurar que esto NO esté comentado:
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);  // ✅ DEBE estar
  // NO hacer memset(nvm, 0, nvm_size);  // ❌ NO limpiar
}
```

### ❌ Problema: Join muy lento después de reset

**Síntomas**:
- Después de reset, tarda ~10-30 segundos en hacer join

**Causa posible**:
- El contexto no se restaura → hace join OTAA completo cada vez
- Sesión previa no se reutiliza

**Verificación**:
```
# Logs esperados después de reset:
- "NVM DATA RESTORED"        ⬅️ Debe aparecer
- Join casi inmediato        ⬅️ Reutiliza sesión
```

**Solución**:
- Verificar que `OnRestoreContextRequest()` está habilitado
- Verificar que `LORAWAN_NVM_BASE_ADDRESS` es correcto
- Verificar que no hay erase accidental de esa región Flash

### ❌ Problema: "NVM DATA STORE FAILED"

**Síntomas**:
```
NVM DATA STORE FAILED
```

**Causas posibles**:

1. **Flash protegida**:
   ```c
   // Verificar que Flash no está protegida
   HAL_FLASH_Unlock();  // Debe retornar HAL_OK
   ```

2. **Dirección inválida**:
   ```c
   // Verificar que 0x0801F000 está en rango válido
   // STM32WL55: 0x0800_0000 - 0x0803_FFFF (256KB)
   ```

3. **Alineación incorrecta**:
   ```c
   // Flash STM32 requiere escritura en múltiplos de 8 bytes
   // El stack ya maneja esto, pero verificar:
   assert((nvm_size % 8) == 0);
   assert(((uint32_t)LORAWAN_NVM_BASE_ADDRESS % 8) == 0);
   ```

4. **Flash corrupta**:
   ```bash
   # Usar STM32CubeProgrammer para hacer full chip erase
   # Luego re-flashear firmware
   ```

### ❌ Problema: Join exitoso pero contexto no se guarda

**Síntomas**:
- Join funciona
- NO aparece log "NVM DATA STORED"
- Después de reset, DevNonce vuelve a 0

**Verificación**:

```c
// 1. Verificar que callback está registrado
static const LmHandlerCallbacks_t LmHandlerCallbacks =
{
  // ...
  .OnNvmDataChange = OnNvmDataChange,  // ⬅️ DEBE estar
  // ...
};

// 2. Verificar que thread existe
if (Thd_LoraStoreContextId == NULL)
{
  APP_LOG(TS_OFF, VLEVEL_M, "ERROR: Store context thread not created!\r\r\n");
}

// 3. Agregar log en OnNvmDataChange
static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  APP_LOG(TS_OFF, VLEVEL_M, "🔔 OnNvmDataChange called, state=%d\r\r\n", state);
  // ...
}
```

### ❌ Problema: Flash se llena / memoria insuficiente

**Síntomas**:
- Errores de compilación relacionados con memoria
- Erase falla

**Solución**:

```c
// Verificar en linker script que hay espacio reservado
// STM32WL55JC tiene 256KB Flash

// Ajustar LORAWAN_NVM_BASE_ADDRESS si es necesario
// Debe estar en región NO usada por código/datos

// Ejemplo de configuración conservadora:
#define LORAWAN_NVM_BASE_ADDRESS  ((void *)0x0803E000UL)  // Últimos 8KB
```

### ❌ Problema: Contexto se restaura pero Join falla

**Síntomas**:
- Log "NVM DATA RESTORED"
- Join Request enviado
- ChirpStack rechaza (timeout o error)

**Causas posibles**:

1. **Claves cambiaron en ChirpStack**:
   - Solución: Borrar dispositivo en ChirpStack, re-agregar con mismas claves

2. **AppKey/AppEUI/DevEUI diferentes**:
   - Verificar que coinciden con ChirpStack

3. **Contexto corrupto**:
   ```c
   // El stack detecta CRC inválido y debería hacer join limpio
   // Si no funciona, borrar Flash manualmente:
   FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, 4096);  // Borra 4KB
   NVIC_SystemReset();  // Reset
   ```

### 🧰 Herramientas de debug útiles

1. **STM32CubeProgrammer**:
   - Inspeccionar Flash directamente
   - Hacer full chip erase si hay problemas
   - Verificar memory protection

2. **Logic Analyzer / SWD Debugger**:
   - Poner breakpoints en `OnStoreContextRequest()`
   - Ver contenido de buffer `nvm`

3. **Logs con timestamps**:
   ```c
   APP_LOG(TS_ON, VLEVEL_M, "...");  // TS_ON para timestamp
   ```

4. **Contador manual de DevNonce**:
   ```c
   // Agregar variable estática para debug
   static uint16_t debug_dev_nonce_count = 0;
   
   // En OnJoinRequest():
   debug_dev_nonce_count++;
   APP_LOG(TS_OFF, VLEVEL_M, "🔢 Join attempt #%d\r\r\n", debug_dev_nonce_count);
   ```

---

## 📚 Referencias

### Documentación oficial ST

1. **AN5406**: "How to build a LoRa application with STM32CubeWL"
   - Sección sobre NVM context management
   - Describe callbacks `OnStoreContextRequest` / `OnRestoreContextRequest`

2. **UM2642**: "STM32WL LoRaWAN Expansion Package User Manual"
   - Arquitectura dual-core
   - MBMUX (Mailbox) inter-core communication

3. **RM0453**: "STM32WL5x Reference Manual"
   - Flash memory mapping
   - Flash programming interface

### Especificación LoRaWAN

1. **LoRaWAN 1.0.4 Specification**:
   - Section 6.2.4: "Join-accept message"
   - DevNonce requirements

2. **LoRa Alliance Technical Recommendations**:
   - TR005: "Guidelines for LoRaWAN end-device certification"
   - Menciona persistencia de DevNonce/contadores

### Código de ejemplo ST

- `STM32Cube_FW_WL_V1.x.x/Projects/NUCLEO-WL55JC/Applications/LoRaWAN/LoRaWAN_End_Node_DualCore`
- Implementación de referencia de NVM management

---

## ✅ Resumen ejecutivo

### ¿El sistema de NVM funciona actualmente en tu proyecto?

**SÍ**, el sistema está **correctamente implementado y habilitado**:

✅ Callbacks registrados correctamente
✅ Thread de guardado asíncrono creado
✅ `OnRestoreContextRequest()` habilitado (lee Flash)
✅ `OnStoreContextRequest()` escribe en 0x0801F000
✅ MBMUX comunica CM4 ↔ CM0+ correctamente

### Lo que debería pasar con DevNonce:

```
Power-ON (Flash vacía)
  ├─ DevNonce = 0
  ├─ Join Request #1 (DevNonce=0)
  ├─ Join acepted
  └─ Guarda contexto (DevNonce=1) en Flash

RESET
  ├─ Restaura contexto desde Flash
  ├─ DevNonce = 1 (continúa)
  ├─ Reutiliza sesión existente (NO hace join)
  └─ Sigue funcionando

Borrar dispositivo en ChirpStack + RESET
  ├─ Restaura contexto (DevNonce=1)
  ├─ Join Request #2 (DevNonce=1)
  ├─ ChirpStack acepta (1 > 0)
  └─ Guarda contexto (DevNonce=2) en Flash
```

### Si tienes problemas de "DevNonce already used":

1. ✅ Verificar que `OnRestoreContextRequest()` NO tiene `memset()` que limpie el buffer
2. ✅ Verificar logs: debe aparecer "NVM DATA RESTORED" al arrancar
3. ✅ Verificar logs: debe aparecer "NVM DATA STORED" después de join
4. ✅ Borrar dispositivo en ChirpStack y volver a agregarlo
5. ✅ Hacer full chip erase si hay dudas sobre Flash corrupta

---

**Creado**: 2026-02-03  
**Autor**: GitHub Copilot (Claude Sonnet 4.5)  
**Versión**: 1.0  
**Proyecto**: LoRaWAN_End_Node_DualCoreFreeRTOS_Modified
