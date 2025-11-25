# RTOS Buzzer Alarm System

Sistema de alarmas no bloqueante para buzzer usando FreeRTOS.

## 📋 Características

- ✅ **No bloqueante** - usa software timers de FreeRTOS
- ✅ **Múltiples patrones predefinidos** - beeps, alertas, pulsos
- ✅ **Patrones personalizables** - crea tus propias alarmas
- ✅ **Integración con zonas** - alarmas automáticas por zona
- ✅ **Thread-safe** - seguro para usar en múltiples tareas
- ✅ **Bajo consumo de recursos** - un solo timer por alarma

## 🚀 Inicio Rápido

### 1. Inicialización

```c
// Primero inicializar el buzzer básico
BuzzerConfig_t config = {
    .htim = &htim1,
    .channel = TIM_CHANNEL_3,
    .frequency_hz = 4000,
    .duty_cycle = 50
};
Buzzer_Init(&config);

// Luego inicializar el sistema de alarmas
BuzzerAlarm_Init();
```

### 2. Usar un patrón predefinido

```c
// Beep simple
BuzzerAlarm_StartPattern(ALARM_BEEP_ONCE);

// Alerta continua
BuzzerAlarm_StartPattern(ALARM_ALERT);

// Detener
BuzzerAlarm_Stop();
```

### 3. Integración con zonas (automático)

```c
// El patrón se selecciona automáticamente según la zona
BuzzerAlarm_StartZone(YELLOW_ZONE);  // Inicia patrón de alerta
```

## 📖 Patrones Disponibles

| Patrón | Descripción | Repeticiones | Uso |
|--------|-------------|--------------|-----|
| `ALARM_BEEP_ONCE` | Beep único | 1 | Confirmación |
| `ALARM_BEEP_DOUBLE` | Doble beep | 2 | Advertencia suave |
| `ALARM_BEEP_TRIPLE` | Triple beep | 3 | Advertencia media |
| `ALARM_WARNING` | Beeps lentos | ∞ | Zona BLUE |
| `ALARM_ALERT` | Beeps rápidos | ∞ | Zona YELLOW |
| `ALARM_CRITICAL` | Beeps muy rápidos | ∞ | Zona RED |
| `ALARM_CONTINUOUS` | Sonido continuo | ∞ | Emergencia |
| `ALARM_PULSE_SLOW` | Pulso lento (500ms) | ∞ | Zona LIGHT_BLUE |
| `ALARM_PULSE_FAST` | Pulso rápido (200ms) | ∞ | Zona DARK_BLUE |

## 🎵 Mapeo de Zonas

| Zona | Patrón | Comportamiento |
|------|--------|----------------|
| `GREEN_ZONE` | Ninguno | Silencio (seguro) |
| `LIGHT_BLUE_ZONE` | `PULSE_SLOW` | Pulso suave 500ms |
| `BLUE_ZONE` | `WARNING` | Beep cada 1s |
| `DARK_BLUE_ZONE` | `PULSE_FAST` | Pulso rápido 200ms |
| `YELLOW_ZONE` | `ALERT` | Beeps rápidos |
| `RED_ZONE` | `CRITICAL` | Beeps muy rápidos |
| `BLACK_ZONE` | Ninguno | Silencio (escape) |

## 🛠️ Ejemplos de Uso

### Ejemplo 1: Patrón simple (no bloqueante)

```c
void myTask(void *argument) {
    BuzzerAlarm_Init();
    
    // Iniciar alarma de advertencia
    BuzzerAlarm_StartPattern(ALARM_WARNING);
    
    // Tu tarea puede seguir trabajando mientras la alarma suena
    for (int i = 0; i < 100; i++) {
        // Hacer trabajo importante
        processData();
        osDelay(50);
    }
    
    // Detener cuando termines
    BuzzerAlarm_Stop();
}
```

### Ejemplo 2: Patrón personalizado

```c
// Crear patrón SOS (corto-corto-corto, largo-largo-largo, corto-corto-corto)
AlarmConfig_t sos_pattern = {
    .pattern = ALARM_CUSTOM,
    .frequency_hz = 4000,
    .duty_cycle = 60,
    .on_time_ms = 100,    // Beep corto
    .off_time_ms = 100,
    .repeat_count = 3      // 3 veces
};

BuzzerAlarm_StartCustom(&sos_pattern);
osDelay(1000);

// Cambiar a beeps largos
sos_pattern.on_time_ms = 300;  // Beep largo
BuzzerAlarm_StartCustom(&sos_pattern);
```

### Ejemplo 3: Uso en stimulusTask

```c
static void handleZoneChange(zone_t newZone) {
    // Un solo llamado - el sistema hace todo automáticamente
    BuzzerAlarm_StartZone(newZone);
    
    // No bloquea - la alarma corre en background
    sendStimulusFeedback();
}
```

### Ejemplo 4: Monitoreo de estado

```c
// Verificar si una alarma está activa
if (BuzzerAlarm_IsActive()) {
    printf("Alarma corriendo\n");
}

// Obtener estado detallado
const AlarmState_t* state = BuzzerAlarm_GetState();
printf("Patrón: %d, Repeticiones: %u\n", 
       state->pattern, state->repetitions);
```

## 🔧 API Completa

### Inicialización

```c
bool BuzzerAlarm_Init(void);
void BuzzerAlarm_DeInit(void);
```

### Control de alarmas

```c
// Iniciar patrón predefinido
bool BuzzerAlarm_StartPattern(AlarmPattern_t pattern);

// Iniciar patrón personalizado
bool BuzzerAlarm_StartCustom(const AlarmConfig_t* config);

// Iniciar por zona (automático)
bool BuzzerAlarm_StartZone(zone_t zone);

// Detener alarma actual
void BuzzerAlarm_Stop(void);
```

### Consulta de estado

```c
bool BuzzerAlarm_IsActive(void);
const AlarmState_t* BuzzerAlarm_GetState(void);
```

## 📊 Diferencia: Blocking vs Non-blocking

### ❌ Método antiguo (BLOQUEANTE)

```c
void handleZone(zone_t zone) {
    if (zone == RED_ZONE) {
        // Bloquea la tarea por 500ms!
        for (int i = 0; i < 10; i++) {
            Buzzer_Beep(4000, 80, 50);
            osDelay(50);  // ❌ Task bloqueada aquí
        }
    }
}
```

### ✅ Método nuevo (NO BLOQUEANTE)

```c
void handleZone(zone_t zone) {
    // No bloquea - alarma corre en background
    BuzzerAlarm_StartZone(zone);
    
    // Task libre para continuar inmediatamente
    sendFeedback();
    processOtherWork();
}
```

## 💡 Ventajas

1. **No bloquea tu task** - puedes seguir haciendo trabajo
2. **Más eficiente** - usa timers de hardware
3. **Código más limpio** - un llamado en vez de loops
4. **Fácil de cambiar** - modifica patrones sin tocar lógica
5. **Thread-safe** - funciona con múltiples tareas

## 📝 Notas Importantes

- **Inicializar primero**: Llama `Buzzer_Init()` antes de `BuzzerAlarm_Init()`
- **Solo una alarma a la vez**: Si inicias una nueva, la anterior se detiene automáticamente
- **Repeticiones infinitas**: `repeat_count = 0` significa repetir forever
- **Timers de software**: Usa los timers de FreeRTOS, no bloquea tasks
- **Stack mínimo**: Muy bajo uso de stack (solo callbacks)

## 🔍 Ejemplo Completo: stimulusTask

```c
void stimulusTask(void *argument) {
    // Inicializar buzzer básico
    BuzzerConfig_t config = {
        .htim = &htim1,
        .channel = TIM_CHANNEL_3,
        .frequency_hz = 4000,
        .duty_cycle = 50
    };
    Buzzer_Init(&config);
    
    // Inicializar sistema de alarmas RTOS
    BuzzerAlarm_Init();
    
    while (1) {
        // Recibir cambio de zona
        if (osMessageQueueGet(stimulusQueueHandle, &msg, NULL, 0) == osOK) {
            zone_t newZone = msg->payload[0];
            
            // Iniciar alarma apropiada (no bloqueante!)
            BuzzerAlarm_StartZone(newZone);
            
            MessagePool_Free(msg);
        }
        
        // Task libre para hacer otras cosas
        osDelay(100);
    }
}
```

## 🚨 Troubleshooting

**P: La alarma no suena**
- Verifica que `Buzzer_Init()` se llamó primero
- Verifica que `BuzzerAlarm_Init()` retornó `true`
- Verifica que el timer htim1 está configurado correctamente

**P: La alarma suena una vez y se detiene**
- Verifica `repeat_count`: debe ser > 1 o 0 (infinito)
- Verifica que nadie más está llamando `BuzzerAlarm_Stop()`

**P: No puedo correr dos alarmas simultáneamente**
- Correcto, solo una alarma a la vez. Si necesitas múltiples sonidos, usa patrones personalizados.

## 📚 Ver También

- `buzzer.h` - Control básico del buzzer
- `buzzer_example.c` - Ejemplos bloqueantes originales
- `buzzer_alarm_example.c` - Ejemplos completos de alarmas RTOS
- `stimulusTask.cpp` - Integración real en el sistema
