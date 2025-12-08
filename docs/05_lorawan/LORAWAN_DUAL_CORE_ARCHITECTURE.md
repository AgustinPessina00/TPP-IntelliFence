# Arquitectura LoRaWAN Dual-Core en STM32WL55

## 📚 Introducción: Separación de Cores

El STM32WL55 tiene **dos cores ARM Cortex-M** que trabajan en conjunto para manejar LoRaWAN:

```
┌─────────────────────────────────────────────────────────────┐
│                         STM32WL55                           │
├─────────────────────────────┬───────────────────────────────┤
│          CM4 (M4)           │       CM0+ (M0PLUS)           │
│   Aplicación Principal      │    Radio LoRa Hardware        │
│                             │                               │
│  - fsmTask                  │  - LoRaWAN MAC Layer          │
│  - loraTask                 │  - Radio Driver               │
│  - lora_app.c (wrapper)     │  - lora_app.c (stack)         │
│  - FreeRTOS threads         │  - Hardware SX1262            │
│  - Aplicación de usuario    │  - Crypto engine              │
│                             │                               │
│         IPCC (Inter-Processor Communication Channel)        │
│              ↕ Mailbox (MbMux) ↕                           │
└─────────────────────────────────────────────────────────────┘
```

### ¿Por qué dos cores?

| Core | Función | Responsabilidad |
|------|---------|-----------------|
| **CM4** | Aplicación principal | Maneja la lógica de aplicación, FreeRTOS, tus tasks (fsmTask, loraTask, etc.) |
| **CM0+** | Radio LoRa | Dedicado exclusivamente a manejar el hardware de radio LoRa (SX1262 integrado) |
| **IPCC/MbMux** | Canal de comunicación | "Teléfono interno" entre cores para intercambiar mensajes |

---

## 🔄 Flujo Completo: Desde fsmTask hasta el Gateway

### **Paso 1: fsmTask envía datos GPS al dispatcher**

```cpp
// En fsmTask.cpp
LoraGpsData_t loraData;
loraData.latitude = -34.603722;
loraData.longitude = -58.381592;
// ⚠️ SOLO lat/lon, NO zone ni distance

// Enviar al dispatcher (NO directo a loraTask)
DispatcherMessage_t msg;
msg.type = MSG_GPS_DATA;
msg.data = &loraData;
msg.size = sizeof(LoraGpsData_t);  // 16 bytes
sendToDispatcher(&msg);
```

**¿Por qué usar dispatcher?**

- ✅ **Centralización:** Un solo punto de control de mensajes
- ✅ **Modularidad:** fsmTask no conoce a loraTask directamente
- ✅ **Escalabilidad:** Fácil agregar otros destinos

**Resultado:** El mensaje va al dispatcher, NO directamente a loraTask.

---

### **Paso 2: dispatcher redirige a loraTxQueue**

```cpp
// En dispatcherTask.cpp
void dispatcherTask(void *argument) {
    while(1) {
        DispatcherMessage_t msg;
        
        // Recibe mensaje desde fsmTask u otros
        if (osMessageQueueGet(dispatcherQueueHandle, &msg, NULL, osWaitForever) == osOK) {
            
            switch(msg.type) {
                case MSG_GPS_DATA:
                    // Redirigir a loraTask
                    LoraGpsData_t *gpsData = (LoraGpsData_t*)msg.data;
                    osMessageQueuePut(loraTxQueueHandle, gpsData, 0U, 0U);
                    break;
                    
                case MSG_SENSOR_DATA:
                    // Otros tipos de mensajes...
                    break;
            }
        }
    }
}
```

**Resultado:** El GPS data llega a `loraTxQueueHandle` que lee loraTask.

---

### **Paso 3: loraTask recibe desde loraTxQueue (CM4)**

```cpp
// En loraTask.cpp - Loop principal
void loraTask(void *argument) {
    while(1) {
        LoraGpsData_t receivedData;
        
        // BLOQUEA hasta recibir desde dispatcher
        if (osMessageQueueGet(loraTxQueueHandle, &receivedData, NULL, osWaitForever) == osOK) {
            
            // Guarda SOLO referencia temporal (NO persistente)
            loraTaskState.currentData = receivedData;
            loraTaskState.hasNewData = true;  // ← Flag para lora_app
            
            // ⚠️ NO ENVÍA INMEDIATAMENTE
            // Solo marca que hay datos nuevos disponibles
        }
    }
}
```

**¿Por qué NO envía inmediatamente?**

Porque el envío lo controla el **stack LoRaWAN** mediante un **timer periódico** configurado en `APP_TX_DUTYCYCLE` (10 segundos por defecto).

**¿Por qué NO guarda estado?**

Solo mantiene una referencia temporal. Después del envío, se marca `hasNewData = false` y espera nuevo mensaje.

---

### **Paso 4: Timer de LoRaWAN dispara envío (CM4)**

```cpp
// En lora_app.c (CM4)
static void OnTxTimerEvent(void *context) {
    // Cada APP_TX_DUTYCYCLE (10 segundos), dispara esta función
    osThreadFlagsSet(Thd_LoraSendProcessId, 1);  // ← Despierta thread
    UTIL_TIMER_Start(&TxTimer);  // Reinicia timer para la próxima vez
}
```

Este timer se configura en `LoRaWAN_Init()`:

```cpp
TxPeriodicity = APP_TX_DUTYCYCLE;  // 10000 ms
UTIL_TIMER_Create(&TxTimer, TxPeriodicity, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
UTIL_TIMER_Start(&TxTimer);
```

**Resultado:** El thread de envío se despierta periódicamente.

---

### **Paso 5: Thread de envío se despierta (CM4)**

```cpp
// En lora_app.c (CM4)
static void Thd_LoraSendProcess(void *argument) {
    for (;;) {
        // BLOQUEA hasta recibir señal del timer
        osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
        
        // Se ejecuta cada 10 segundos
        SendTxData();
    }
}
```

**Resultado:** `SendTxData()` se ejecuta cada vez que el timer dispara.

---

### **Paso 5: SendTxData() obtiene payload de loraTask (CM4)**

```cpp
// En lora_app.c (CM4)
static void SendTxData(void) {
    LmHandlerAppData_t appData;
    uint8_t txBuffer[242];
    
    // 🔹 AQUÍ SE CONECTA CON loraTask
    uint8_t payloadSize = loraTaskGetPayload(txBuffer, sizeof(txBuffer));
    
    if (payloadSize > 0) {
        appData.Port = 2;
        appData.Buffer = txBuffer;
        appData.BufferSize = payloadSize;
        
        // 🚀 ENVÍA AL CM0+ mediante LmHandlerSend
        LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG, false);
    }
}
```

**¿Qué hace `loraTaskGetPayload()`?**

```cpp
// En loraTask.cpp
uint8_t loraTaskGetPayload(uint8_t *buffer, uint8_t maxSize) {
    if (!s_hasValidData) {
        return 0;  // No hay ninguna posición válida todavía
    }
    
    // Copia SIEMPRE la última posición válida (16 bytes)
    memcpy(&buffer[0], &s_lastValidGpsData.latitude, 8);   // 8 bytes
    memcpy(&buffer[8], &s_lastValidGpsData.longitude, 8);  // 8 bytes
    // Total: 16 bytes (NO zone, NO distance)
    
    // NO limpia s_lastValidGpsData (se reenvía si no hay datos nuevos)
    return 16;  // Tamaño del payload
}
```

**Resultado:** El payload de 16 bytes (solo lat/lon) está listo. Si no hay datos nuevos en la cola, reenvía la última posición válida.

---

### **Paso 6: LmHandlerSend() envía al CM0+ (CM4 → CM0+)**

```cpp
// En LmHandler.c (middleware)
LmHandlerErrorStatus_t LmHandlerSend(LmHandlerAppData_t *appData, 
                                     LmHandlerMsgTypes_t msgType, 
                                     bool allowDelayedTx) {
    // Prepara el mensaje LoRaWAN
    // - Agrega port
    // - Agrega headers
    // - Prepara estructura
    
    // 🔹 IPCC: Envía al CM0+ mediante MbMux (Mailbox)
    MbMuxIf_LoraRequest(appData, msgType);
    
    return LORAMAC_HANDLER_SUCCESS;
}
```

**¿Qué es MbMux?**

**MbMux (Mailbox Mux)** es el sistema de mensajería entre cores que usa **IPCC** (Inter-Processor Communication Channel) como hardware subyacente.

```
CM4                              CM0+
 ↓                                ↑
MbMuxIf_LoraRequest()          MbMux_RX_IRQ()
 ↓                                ↑
IPCC Hardware Registers ←→ IPCC Hardware Registers
```

**Resultado:** El mensaje cruza del CM4 al CM0+ mediante hardware IPCC.

---

### **Paso 7: CM0+ procesa y transmite por radio**

```
┌──────────────────────────────────────────────┐
│              CM0+ (M0PLUS)                   │
├──────────────────────────────────────────────┤
│                                              │
│  1. IPCC RX IRQ recibe mensaje del CM4      │
│     ↓                                        │
│  2. MbMux desempaqueta el payload            │
│     ↓                                        │
│  3. LoRaMac stack procesa:                   │
│     - Agrega headers LoRaWAN (MHDR, etc.)    │
│     - Encripta payload con AES-128           │
│     - Calcula MIC (Message Integrity Code)   │
│     - Agrega DevAddr, FCnt, FPort            │
│     ↓                                        │
│  4. Radio driver (SX1262):                   │
│     - Configura frecuencia (AU915)           │
│     - Configura spreading factor (SF7-SF12)  │
│     - Configura potencia TX (0-22 dBm)       │
│     - Configura bandwidth (125/250/500 kHz)  │
│     ↓                                        │
│  5. Hardware SX1262 transmite por antena     │
│     📡 ────────────→ Gateway                │
│                                              │
└──────────────────────────────────────────────┘
```

**Estructura del paquete LoRaWAN transmitido:**

```
┌─────────┬──────────┬──────┬─────────────┬─────────┬─────┐
│  MHDR   │ DevAddr  │ FCtrl│    FCnt     │  FPort  │ MIC │
│ 1 byte  │ 4 bytes  │1 byte│   2 bytes   │ 1 byte  │4 byt│
├─────────┴──────────┴──────┴─────────────┴─────────┴─────┤
│                    MAC Header (9 bytes)                  │
├──────────────────────────────────────────────────────────┤
│          Encrypted Payload (16 bytes en tu caso)         │
│          lat (8) + lon (8)                               │
└──────────────────────────────────────────────────────────┘
Total: 9 + 16 = 25 bytes en el aire (sin contar preámbulo)
```

---

### **Paso 8: Callbacks notifican al CM4**

Cuando el CM0+ termina de transmitir, envía notificación de vuelta al CM4:

```cpp
// En lora_app.c (CM4)
static void OnTxData(LmHandlerTxParams_t *params) {
    // Se ejecuta cuando el CM0+ confirmó que transmitió
    APP_LOG(TS_OFF, VLEVEL_M, "TX Complete: DR=%d, PWR=%d\r\n", 
            params->Datarate, params->TxPower);
    
    // params->Datarate: Data rate usado (DR_0 a DR_5)
    // params->TxPower: Potencia TX (0 a 10 para AU915)
    // params->NbTrans: Número de retransmisiones
}
```

**Resultado:** El CM4 sabe que la transmisión fue exitosa.

---

## ⚙️ Integración LoRaWAN + FreeRTOS

### **¿Cómo se combinan LoRaWAN y FreeRTOS?**

El stack LoRaWAN NO es independiente de FreeRTOS. Está **integrado dentro de FreeRTOS** y usa sus primitivas:

```
┌───────────────────────────────────────────────────────────┐
│                    FreeRTOS Scheduler                      │
│                                                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │   fsmTask    │  │  loraTask    │  │ Thd_LoRa     │   │
│  │  (tu app)    │  │  (buffer)    │  │ SendProcess  │   │
│  │              │  │              │  │ (stack LoRa) │   │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘   │
│         │                 │                 │            │
│         ↓                 ↓                 ↓            │
│  ┌─────────────────────────────────────────────┐         │
│  │         FreeRTOS Queues & Timers            │         │
│  │  - loraTxQueueHandle (cola GPS)             │         │
│  │  - TxTimer (cada 10 seg)                    │         │
│  │  - osThreadFlags (sincronización)           │         │
│  └─────────────────────────────────────────────┘         │
│                                                            │
│  ┌─────────────────────────────────────────────┐         │
│  │         LoRaWAN Sequencer (middleware)      │         │
│  │  - UTIL_SEQ_Run() - loop infinito           │         │
│  │  - Ejecuta callbacks de LoRaWAN             │         │
│  └─────────────────────────────────────────────┘         │
└───────────────────────────────────────────────────────────┘
```

### **Componentes FreeRTOS usados por LoRaWAN:**

| Componente | Dónde se usa | Propósito |
|------------|--------------|-----------|
| **osThread** | `Thd_LoraSendProcess` | Thread que ejecuta `SendTxData()` cada 10 seg |
| **osMessageQueue** | `loraTxQueueHandle` | Cola para pasar GPS de fsmTask → loraTask |
| **osThreadFlags** | Sincronización | Timer despierta thread con `osThreadFlagsSet()` |
| **UTIL_TIMER** | `TxTimer` | Timer software periódico (10 seg) |
| **UTIL_SEQ** | Sequencer | Loop que ejecuta callbacks de LoRaWAN |

---

### **🔄 Ciclo de vida completo:**

```
1. ARRANQUE (main)
   ├─ MX_FREERTOS_Init()
   │   ├─ Crea fsmTask (tu aplicación)
   │   ├─ Crea loraTask (buffer GPS)
   │   └─ Crea loraTxQueueHandle (cola)
   │
   └─ osKernelStart() → Arranca FreeRTOS scheduler

2. loraTask INICIALIZA LoRaWAN (en su primera ejecución)
   ├─ MX_LoRaWAN_Init()
   │   ├─ SystemApp_Init() → Despierta CM0+
   │   ├─ LmHandlerInit() → Configura stack
   │   ├─ Crea Thd_LoraSendProcess (thread de envío)
   │   └─ UTIL_TIMER_Create(&TxTimer) → Timer de 10 seg
   │
   └─ LoRaMacStart() → Inicia JOIN

3. LOOP CONTINUO (mientras corre FreeRTOS)
   │
   ├─ fsmTask (tu aplicación)
   │   ├─ Toma decisiones (FSM)
   │   └─ sendToDispatcher(GPS) cada vez que decide
   │
   ├─ dispatcherTask
   │   └─ Redirige a loraTxQueueHandle
   │
   ├─ loraTask (buffer)
   │   ├─ Lee loraTxQueueHandle (blocking)
   │   ├─ Guarda s_lastValidGpsData (última posición)
   │   └─ NO envía inmediatamente (espera timer)
   │
   └─ Thd_LoraSendProcess (thread LoRa)
       ├─ Espera timer (osThreadFlagsWait)
       │
       ├─ Cada 10 segundos:
       │   ├─ TxTimer dispara OnTxTimerEvent()
       │   ├─ osThreadFlagsSet() despierta thread
       │   ├─ SendTxData() se ejecuta
       │   ├─ loraTaskGetPayload() obtiene última posición
       │   └─ LmHandlerSend() envía al CM0+
       │
       └─ CM0+ transmite por radio → Gateway

4. DOWNLINK (Clase A: ventana RX1 y RX2)
   ├─ Después de cada TX, el CM0+ abre ventanas de escucha
   ├─ Si llega mensaje del gateway:
   │   ├─ CM0+ notifica al CM4 vía IPCC
   │   └─ OnRxData() se ejecuta en CM4
   └─ Procesar puntos de cerco (futuro)
```

---

### **💡 Puntos clave:**

#### **1. ¿Dónde está el loop infinito del stack LoRaWAN?**

En **dos lugares simultáneos**:

- **`Thd_LoraSendProcess`** (thread FreeRTOS): Ejecuta TX periódicamente
- **`UTIL_SEQ_Run()`** (en background): Procesa eventos del stack LoRaWAN

```cpp
// En CM4: Thread de envío (cada 10 seg)
static void Thd_LoraSendProcess(void *argument) {
    for (;;) {  // ← Loop infinito en CM4
        osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
        SendTxData();  // Ejecuta cada 10 seg
    }
}

// En CM0+: Sequencer en main()
int main(void) {  // ← main() del CM0+
    // ... inicialización ...
    
    while (1) {  // ← Loop infinito en CM0+
        LoRaWAN_Process();
    }
}

void LoRaWAN_Process(void) {
    UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);  // ← Procesa callbacks LoRaWAN
    // Ejecuta: OnJoinRequest, OnRxData, OnTxData, OnMacProcess, etc.
}
```

**IMPORTANTE:** `UTIL_SEQ_Run()` debe ejecutarse continuamente. En tu arquitectura dual-core:
- ✅ **Se ejecuta en el CM0+** dentro del `while(1)` de su `main()`
- ✅ El CM0+ llama continuamente a `LoRaWAN_Process()` → `UTIL_SEQ_Run()`
- ✅ Esto procesa callbacks del stack LoRaWAN (OnTxData, OnRxData, OnMacProcess, etc.)

---

#### **2. ¿Por qué loraTask NO envía inmediatamente?**

Porque el **timer** controla el envío, NO la llegada de datos:

```
Cola loraTxQueue       loraTask                Timer (10 seg)
─────────────────────────────────────────────────────────────
GPS llega       ────→  Guarda en s_lastValidGpsData
                       s_hasValidData = true
                       
                       ⏳ Espera...
                       
                                               ⏰ Timer dispara
                                               
                       loraTaskGetPayload() ←─ SendTxData()
                       Retorna última posición
                       
                                               LmHandlerSend()
                                               
                                               📡 TX al gateway
```

**Ventaja:** Puedes recibir 100 updates de GPS, pero solo envías cada 10 seg (ahorra batería).

---

#### **3. ¿Cómo se guarda la última posición?**

```cpp
// Estado interno de loraTask
static LoraGpsData_t s_lastValidGpsData = {0}; // Última posición válida
static bool s_hasValidData = false;            // Flag: tiene posición

// Cada vez que llega GPS nuevo:
if (osMessageQueueGet(loraTxQueueHandle, &receivedData, NULL, 100) == osOK) {
    s_lastValidGpsData = receivedData;  // ← Actualiza última posición
    s_hasValidData = true;
}

// Cada 10 segundos (timer):
uint8_t loraTaskGetPayload(uint8_t *buffer, uint8_t maxSize) {
    if (!s_hasValidData) return 0;  // No hay ninguna posición aún
    
    // Copia SIEMPRE la última posición (aunque no sea nueva)
    memcpy(buffer, &s_lastValidGpsData, 16);
    
    // NO limpia s_lastValidGpsData (se reutiliza la próxima vez)
    return 16;
}
```

**Resultado:**
- Si fsmTask envía GPS cada 2 seg → loraTask actualiza `s_lastValidGpsData` cada 2 seg → Gateway recibe cada 10 seg (última de las 5 posiciones)
- Si fsmTask NO envía nada → loraTask reenvía la misma posición cada 10 seg → Gateway sabe que la vaca no se movió

---

#### **4. ¿Por qué fsmTask se crea ANTES de osKernelStart() y Thd_LoraSendProcess DESPUÉS?**

**✅ Esto es correcto y es así por diseño.**

**Diferencia entre ambos:**

| Aspecto | Tasks de app_freertos.c | Tasks de LoRaWAN |
|---------|------------------------|------------------|
| **Cuándo se crean** | ANTES de `osKernelStart()` | DESPUÉS de `osKernelStart()` |
| **Dónde se crean** | En `main()` (contexto init) | Dentro de `loraTask` (contexto thread) |
| **Tipo** | Estáticas (predefinidas) | Dinámicas (runtime) |
| **Por qué** | Necesarias desde arranque | Dependen de init del stack LoRaWAN |

**Flujo detallado:**

```
main()
├─ HAL_Init()
├─ SystemClock_Config()
├─ MX_FREERTOS_Init()           ← CREA fsmTask, loraTask (NO arranca)
│   ├─ osThreadNew(&fsmTask)    ← Registra (NO ejecuta todavía)
│   ├─ osThreadNew(&loraTask)   ← Registra (NO ejecuta todavía)
│   └─ osMessageQueueNew()      ← Crea loraTxQueueHandle
│
├─ osKernelStart()               ← 🚀 ARRANCA SCHEDULER
│   │
│   │  ⚠️ main() NO CONTINÚA (queda bloqueado aquí)
│   │  Control pasa al scheduler de FreeRTOS
│   │
│   └─ Scheduler ejecuta: fsmTask, loraTask, etc.
│
│      loraTask (ahora corriendo)
│      ├─ MX_LoRaWAN_Init()
│      │   ├─ SystemApp_Init() → Despierta CM0+ (necesita osDelay)
│      │   ├─ LmHandlerInit() → Configura stack
│      │   ├─ osThreadNew(&Thd_LoraSendProcess)  ← CREA AQUÍ
│      │   └─ UTIL_TIMER_Create(&TxTimer)
│      │
│      └─ Loop infinito...
│
└─ ❌ main() NUNCA llega aquí
```

**¿Por qué LoRaWAN no se inicializa en main()?**

Porque necesita **CM0+ despierto**, y eso requiere:

```c
// En sys_app.c - SystemApp_Init() (llamado desde loraTask)
void SystemApp_Init(void) {
    // ... configuración previa ...
    
    // Inicializa IPCC + Mailbox + Arranca CM0+ + Espera respuesta
    MBMUXIF_Init();  // ← AQUÍ ESTÁ TODO
    
    UTIL_TIMER_Init();
}

// Dentro de MBMUXIF_Init() (función privada):
static void MBMUXIF_Init(void)
{
    // 1. Inicializa IPCC (Inter-Processor Communication Channel)
    MBMUXIF_SystemInit();  // → HAL_IPCC_Init() dentro
    
    // 2. Despierta el CM0+ (core secundario)
    HAL_PWREx_ReleaseCore(PWR_CORE_CPU2);  // ⚠️ Toma tiempo
    
    // 3. Sincronización: permite que CM0+ arranque
    MBMUXIF_SetCpusSynchroFlag(CPUS_BOOT_SYNC_ALLOW_CPU2_TO_START);
    
    // 4. Espera a que CM0+ responda
    MBMUXIF_WaitCm0MbmuxIsInitialized();  // ← usa osDelay(10) dentro
    
    // 5. Obtiene capacidades del CM0+
    p_cm0plus_features = MBMUXIF_SystemSendCm0plusInfoListReq();
    
    // 6. Registra LoRa, Trace, etc.
    MBMUXIF_LoraInit();
    MBMUXIF_TraceInit();
}
```

**Si intentaras esto en main() ANTES de osKernelStart():**
- ❌ No podrías usar `osDelay()` → FreeRTOS no está corriendo
- ❌ No podrías usar mutexes → No hay scheduler
- ❌ `MBMUXIF_WaitCm0MbmuxIsInitialized()` se bloquearía indefinidamente

**Conclusión:**
- ✅ Tasks básicas (app) se crean en `main()` antes del kernel
- ✅ Tasks de LoRaWAN se crean **DESPUÉS**, desde `loraTask`
- ✅ **NO modificar** este diseño, es correcto y modular

---

#### **5. ¿Qué es `UTIL_SEQ` (Sequencer)?**

Es un **scheduler simple** (NO reemplaza a FreeRTOS) que ejecuta **callbacks del stack LoRaWAN**:

```cpp
// Registro de callbacks
UTIL_SEQ_RegTask(1 << CFG_SEQ_Task_LoRa, UTIL_SEQ_RFU, LoRaWAN_Process);

// En el loop (ejecuta callbacks registrados)
UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
```

**Callbacks que ejecuta:**
- `OnJoinRequest()` → Cuando se completa JOIN
- `OnRxData()` → Cuando llega downlink del gateway
- `OnTxData()` → Cuando se completa TX
- `OnMacProcess()` → Mantenimiento del stack MAC

**Relación con FreeRTOS:**
- En **CM4:** FreeRTOS maneja `fsmTask`, `loraTask`, `Thd_LoraSendProcess`
- En **CM0+:** NO usa FreeRTOS, usa `UTIL_SEQ` en el `main()` + `while(1)`
- `UTIL_SEQ` es un **scheduler simple** (no es RTOS) que procesa callbacks
- Los dos cores se comunican vía **IPCC/MbMux**

**Aclaración importante:**
- `UTIL_SEQ_Run()` corre en el **CM0+**, NO en el CM4
- El CM0+ tiene su propio `main()` con `while(1) { LoRaWAN_Process(); }`
- El CM0+ arranca **después** de que el CM4 ejecuta `HAL_PWREx_ReleaseCore()`

---

## 📋 Distribución de Archivos por Core

### **Archivos del CM4 (Aplicación)**

| Archivo | Ubicación | Propósito | Cuándo se ejecuta |
|---------|-----------|-----------|-------------------|
| `fsmTask.cpp` | CM4/Core/Src/threads/ | Tu aplicación: Decide cuándo enviar datos GPS | Continuamente (FSM) |
| `loraTask.cpp` | CM4/Core/Src/threads/ | Buffer intermedio: Almacena datos hasta envío | Continuamente (espera cola) |
| `lora_app.c` | CM4/LoRaWAN/App/ | Wrapper del stack: Conecta app con middleware | Cada 10 seg (timer) + callbacks |
| `app_lorawan.c` | CM4/LoRaWAN/App/ | Inicialización del stack LoRaWAN | Al arrancar (setup) |
| `CayenneLpp.c` | CM4/LoRaWAN/App/ | (Opcional) Formato de payload CayenneLpp | Si se usa (no en tu caso) |
| `LmHandler.c` | Middlewares/ | Middleware: Gestor de alto nivel del stack | Llamado por lora_app.c |
| `mbmuxif_lora.c` | CM4/MbMux/ | IPCC TX: Envía mensajes al CM0+ | Cuando hay datos para enviar |

### **Archivos del CM0+ (Radio)**

| Archivo | Ubicación | Propósito | Cuándo se ejecuta |
|---------|-----------|-----------|-------------------|
| `lora_app.c` | CM0PLUS/LoRaWAN/App/ | Recibe comandos desde CM4 vía IPCC | Al recibir mensaje IPCC |
| `LoRaMac.c` | Middlewares/Mac/ | Stack LoRaWAN completo (protocolo MAC) | Procesamiento de cada frame |
| `Region*.c` | Middlewares/Mac/Region/ | Configuración regional (AU915, US915, etc.) | Al configurar región |
| `radio.c` | Middlewares/SubGHz_Phy/ | Driver del radio SX1262 | Durante TX/RX |
| `mbmux.c` | CM0PLUS/MbMux/ | IPCC RX: Recibe mensajes del CM4 | IRQ del IPCC |
| `se-identity.h` | CM0PLUS/LoRaWAN/App/ | Keys LoRaWAN: DevEUI, AppKey, JoinEUI | Al hacer JOIN OTAA |

---

## 🔍 Funciones Clave de `lora_app.c` (CM4)

### **1. `SendTxData()` - Enviar datos**

```cpp
static void SendTxData(void) {
    // Obtiene payload de loraTask
    uint8_t payloadSize = loraTaskGetPayload(txBuffer, sizeof(txBuffer));
    
    // Envía al CM0+
    LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG, false);
}
```

| Aspecto | Detalle |
|---------|---------|
| **Cuándo se llama** | Cada `APP_TX_DUTYCYCLE` (10 seg) automáticamente |
| **Qué hace** | Obtiene datos de loraTask y los envía al CM0+ |
| **Parámetros de envío** | Puerto 2, sin confirmación (UNCONFIRMED) |

---

### **2. `OnJoinRequest()` - Resultado del JOIN OTAA**

```cpp
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams) {
    if (joinParams->Status == LORAMAC_HANDLER_SUCCESS) {
        APP_LOG("JOIN SUCCESS!\r\n");
        loraTaskOnJoinSuccess();  // ← Notifica a loraTask
    } else {
        APP_LOG("JOIN FAILED\r\n");
    }
}
```

| Aspecto | Detalle |
|---------|---------|
| **Cuándo se llama** | Después de que el CM0+ intenta el JOIN OTAA |
| **Qué hace** | Notifica el resultado del JOIN a la aplicación |
| **Estados posibles** | `LORAMAC_HANDLER_SUCCESS` o error |

---

### **3. `OnRxData()` - Recibir datos downlink (Class A)**

```cpp
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params) {
    // appData->Port: Puerto del mensaje
    // appData->Buffer: Datos recibidos
    // appData->BufferSize: Tamaño
    
    // ⚠️ IMPORTANTE: En Class A, solo recibes después de cada TX
    if (appData->Port == 2) {
        // Procesa comandos downlink (ej: nuevos puntos del fence)
    }
}
```

| Aspecto | Detalle |
|---------|---------|
| **Cuándo se llama** | Después de un TX exitoso, si el gateway envió algo |
| **Ventanas RX** | RX1 (1 seg después de TX) y RX2 (2 seg después de TX) |
| **Limitación Class A** | Solo puedes recibir después de transmitir |

---

### **4. `OnTxTimerEvent()` - Timer periódico**

```cpp
static void OnTxTimerEvent(void *context) {
    osThreadFlagsSet(Thd_LoraSendProcessId, 1);  // Despierta thread de envío
    UTIL_TIMER_Start(&TxTimer);  // Reinicia timer
}
```

| Aspecto | Detalle |
|---------|---------|
| **Cuándo se llama** | Cada `APP_TX_DUTYCYCLE` automáticamente |
| **Qué hace** | Dispara el envío periódico de datos |
| **Configuración** | Definido en `lora_app.h`: `#define APP_TX_DUTYCYCLE 10000` |

---

### **5. `Thd_LoraSendProcess()` - Thread de envío**

```cpp
static void Thd_LoraSendProcess(void *argument) {
    for (;;) {
        osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);  // Espera señal del timer
        SendTxData();  // Envía datos
    }
}
```

| Aspecto | Detalle |
|---------|---------|
| **Tipo** | Thread FreeRTOS dedicado |
| **Prioridad** | Definida en `CFG_APP_LORA_PROCESS_PRIORITY` |
| **Cuándo se ejecuta** | Cada vez que el timer dispara `osThreadFlagsSet()` |

---

### **6. `OnTxData()` - Confirmación de TX**

```cpp
static void OnTxData(LmHandlerTxParams_t *params) {
    // params->Datarate: Data rate usado (DR_0 a DR_5)
    // params->TxPower: Potencia TX (0 a 10)
    // params->NbTrans: Número de retransmisiones
}
```

| Aspecto | Detalle |
|---------|---------|
| **Cuándo se llama** | Después de que el CM0+ transmitió el paquete |
| **Utilidad** | Logging, estadísticas, debugging |

---

### **7. `OnMacProcessNotify()` - Eventos del MAC**

```cpp
static void OnMacProcessNotify(void) {
    // Solo relevante en Single Core
    // En Dual Core, el CM0+ maneja esto internamente
}
```

| Aspecto | Detalle |
|---------|---------|
| **En Dual Core** | NO se usa (el CM0+ procesa eventos del MAC) |
| **En Single Core** | Se usa para procesar interrupciones del radio |

---

### **8. `StoreContext()` / `OnStoreContextRequest()` - Guardar estado**

```cpp
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size) {
    // Guarda el estado del stack en Flash
    FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE);
    FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, nvm, nvm_size);
}
```

| Aspecto | Detalle |
|---------|---------|
| **Qué guarda** | DevNonce, FrameCounters, session keys, configuración |
| **Cuándo se llama** | Después del JOIN, al cambiar DevNonce, periódicamente |
| **Ubicación Flash** | `0x0801F000` (últimos 2 sectores de 128KB) |

---

## 🎯 CayenneLpp: ¿Qué es?

**CayenneLpp** es un **formato de payload estándar** para IoT:

### **Sin CayenneLpp (tu implementación actual):**

```cpp
uint8_t payload[16];
memcpy(&payload[0], &latitude, 8);   // 8 bytes
memcpy(&payload[8], &longitude, 8);  // 8 bytes
// Total: 16 bytes (SOLO lat/lon)
```

### **Con CayenneLpp:**

```cpp
CayenneLppReset();
CayenneLppAddGps(1, latitude, longitude, 0);  // Canal 1: GPS (11 bytes)
CayenneLppAddDigitalInput(2, zone);           // Canal 2: Zona (3 bytes)
uint8_t size = CayenneLppGetSize();           // Total: 14 bytes
```

### **Comparación:**

| Característica | Tu formato custom | CayenneLpp |
|----------------|-------------------|------------|
| **Tamaño GPS** | 16 bytes (lat+lon) | 11 bytes |
| **Estándar** | No | Sí |
| **Auto-descriptivo** | No | Sí (incluye tipo de dato) |
| **Decoders disponibles** | Debes crear | Ya existen en ChirpStack |
| **Eficiencia** | ✅ Más simple | ❌ Más overhead |

### **¿Lo necesitas?**

**NO** para tu caso. Tu formato custom de 16 bytes es más simple y suficiente. CayenneLpp es útil si necesitas:
- Múltiples tipos de sensores
- Compatibilidad con plataformas IoT estándar
- Decoders automáticos

---

## 🚀 Propuesta de Simplificación

### **Cambios sugeridos según tus requisitos:**

#### **1. Simplificar `LoraGpsData_t` ✅ IMPLEMENTADO**

```cpp
// ✅ IMPLEMENTADO (16 bytes):
typedef struct {
    double latitude;    // 8 bytes
    double longitude;   // 8 bytes
} LoraGpsData_t;
```

**Ventajas:**
- ✅ Más simple
- ✅ Menos memoria (16 vs 17 bytes)
- ✅ Solo datos esenciales
- ✅ No se envía zone ni distance (innecesarios para el gateway)

---

#### **2. Simplificar loraTask ✅ IMPLEMENTADO**

```cpp
// ✅ IMPLEMENTADO: loraTask guarda ÚLTIMA posición válida
static LoraGpsData_t s_lastValidGpsData = {0}; // Última posición GPS válida
static bool s_hasValidData = false;            // Flag: tiene posición válida

void loraTask(void *argument) {
    LoraGpsData_t receivedData;
    
    while(1) {
        // Si llega GPS nuevo, actualiza última posición
        if (osMessageQueueGet(loraTxQueueHandle, &receivedData, NULL, 100) == osOK) {
            s_lastValidGpsData = receivedData;  // Guarda última posición
            s_hasValidData = true;
        }
        
        // Si NO llega nada, s_lastValidGpsData mantiene la última
        // El timer de LoRa (10 seg) reenviará la última posición conocida
    }
}

uint8_t loraTaskGetPayload(uint8_t *buffer, uint8_t maxSize) {
    if (!s_hasValidData) return 0;  // No hay ninguna posición aún
    
    // SIEMPRE envía la última posición (aunque no sea nueva)
    memcpy(buffer, &s_lastValidGpsData, 16);
    return 16;
}
```

**Ventajas:**
- ✅ Tracking continuo cada 10 seg (aunque no haya datos nuevos)
- ✅ No envía "basura" si no hay datos
- ✅ Reenvía última posición conocida (útil para detectar vaca quieta)

---

#### **3. Integrar con dispatcher ✅ ARQUITECTURA DEFINIDA**

```
fsmTask
   ↓
   │ Envía GPS (lat, lon)
   ↓
dispatcherTask
   │ switch(msgType)
   │ case MSG_GPS_DATA:
   ↓
loraTxQueueHandle (cola FreeRTOS)
   ↓
loraTask (lee cola)
   │ s_lastValidGpsData = receivedData
   │ s_hasValidData = true
   ↓
lora_app.c::SendTxData() (timer cada 10 seg)
   │ loraTaskGetPayload() → 16 bytes
   ↓
LmHandlerSend() → CM0+
   ↓
CM0+ (radio stack)
   │ Encripta + agrega headers LoRaWAN
   ↓
SX1262 (hardware radio)
   ↓
📡 Gateway ChirpStack
```

**Ventajas:**
- ✅ Centralización de mensajes en dispatcher
- ✅ fsmTask desacoplado de loraTask
- ✅ Fácil agregar otros destinos (UART, BLE, etc.)
- ✅ Escalable y modular

---

### **Nuevo flujo simplificado:**

```cpp
// En fsmTask.cpp
void enviarPosicion(double lat, double lon) {
    LoraGpsData_t gpsData;
    gpsData.latitude = lat;
    gpsData.longitude = lon;
    
    // Opción 1: Directo a loraTask
    sendGpsDataToLora(&gpsData);
    
    // Opción 2: A través de dispatcher
    DispatcherMessage_t msg;
    msg.type = MSG_GPS_DATA;
    msg.data = &gpsData;
    msg.size = sizeof(LoraGpsData_t);
    sendToDispatcher(&msg);
}
```

---

## ❓ Preguntas Frecuentes

### **1. ¿Por qué `loraTask` y `lora_app.c` interactúan?**

**Separación de responsabilidades:**

| Módulo | Responsabilidad | Cuándo actúa |
|--------|----------------|--------------|
| `loraTask` | Recibe datos de fsmTask, los almacena temporalmente | Al recibir mensaje en cola |
| `lora_app.c` | Decide cuándo enviar (timer), formatea mensaje LoRaWAN | Cada 10 segundos (timer) |

**¿Por qué no fusionarlos?**

- ✅ **Modularidad:** Puedes cambiar `loraTask` sin tocar el stack LoRaWAN
- ✅ **Testabilidad:** Puedes probar `loraTask` independientemente
- ✅ **Claridad:** Cada módulo tiene una responsabilidad clara

---

### **2. ¿Qué parte usa el CM0+?**

Todo lo que está en **`CM0PLUS/` folder**:

```
CM0PLUS/
├── LoRaWAN/App/
│   ├── lora_app.c          ← Recibe comandos IPCC del CM4
│   └── se-identity.h       ← Keys de seguridad
├── MbMux/
│   └── mbmux.c             ← IPCC RX handler
└── Middlewares/ (compartidos)
    ├── LoRaMac.c           ← Stack LoRaWAN completo
    └── radio.c             ← Driver del SX1262
```

---

### **3. ¿`LmHandlerSend()` es la función clave para transmitir?**

**SÍ**. Es la función que envía desde el CM4 al CM0+.

```cpp
LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG, false);
//              ↑               ↑                          ↑
//          Payload    Tipo de mensaje              allowDelayedTx
```

**Parámetros:**

| Parámetro | Tipo | Descripción |
|-----------|------|-------------|
| `appData` | `LmHandlerAppData_t*` | Payload + puerto + tamaño |
| `msgType` | `LmHandlerMsgTypes_t` | `UNCONFIRMED_MSG` o `CONFIRMED_MSG` |
| `allowDelayedTx` | `bool` | Si permite retrasar TX (false = envía inmediato) |

**Internamente usa IPCC:**

```cpp
// Dentro de LmHandlerSend()
MbMuxIf_LoraRequest(appData, msgType);  // ← Envía al CM0+ via IPCC
```

---

### **4. ¿Cómo recibir downlink en Class A?**

En Class A, solo recibes en las **ventanas RX1 y RX2** después de cada TX:

```cpp
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params) {
    if (appData->Port == 10) {  // Puerto para fence points
        // Parsear puntos del fence
        for (int i = 0; i < appData->BufferSize; i += 16) {
            double lat, lon;
            memcpy(&lat, &appData->Buffer[i], 8);
            memcpy(&lon, &appData->Buffer[i+8], 8);
            
            // Guardar en fence
            addFencePoint(lat, lon);
        }
    }
}
```

**Ventanas de recepción:**

```
TX ────┐
       │
       ├─→ RX1 (1 segundo después, misma frecuencia que TX)
       │
       └─→ RX2 (2 segundos después, frecuencia fija)
```

---

### **5. ¿Cómo cambiar el duty cycle?**

En `CM4/LoRaWAN/App/lora_app.h`:

```c
// Cada 10 segundos (testing)
#define APP_TX_DUTYCYCLE  10000

// Cada 1 minuto
#define APP_TX_DUTYCYCLE  60000

// Cada 5 minutos (producción)
#define APP_TX_DUTYCYCLE  300000
```

**⚠️ Importante:** Respeta el **duty cycle legal** de tu región:
- **AU915:** 1% duty cycle (máximo 36 seg/hora de TX)
- **US915:** Sin restricción (uso comercial)
- **EU868:** 1% duty cycle (máximo 36 seg/hora de TX)

---

## 🛠️ Próximos Pasos Sugeridos

### **Implementación simplificada:**

1. ✅ **Simplificar `LoraGpsData_t`:** Eliminar `zone` y `distance`
2. ✅ **Remover estado guardado** en loraTask
3. ✅ **Integrar con dispatcher:** fsmTask → dispatcher → loraTxQueue → loraTask
4. ✅ **Preparar `OnRxData()`** para recibir fence points

¿Quieres que proceda con estos cambios en el código? 🚀

---

## 📚 Referencias

- [STM32WL LoRaWAN Documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32wl-series.html)
- [LoRaWAN Specification 1.0.4](https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-4/)
- [ChirpStack Documentation](https://www.chirpstack.io/docs/)
- [LoRaWAN Regional Parameters](https://lora-alliance.org/resource_hub/rp2-101-lorawan-regional-parameters-2/)

---

**Última actualización:** Diciembre 2025  
**Branch:** LoRaWAN-EndNode-Test
