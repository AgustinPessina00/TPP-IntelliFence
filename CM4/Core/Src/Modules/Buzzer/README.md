# Módulo Buzzer - TPP-IntelliFence

## 📋 Descripción

Módulo de control de buzzer para el sistema TPP-IntelliFence utilizando **TIM1 Channel 3** con generación PWM.

## ✨ Características

- ✅ **Control de encendido/apagado** - On/Off simple
- ✅ **Control de frecuencia** - Rango 1-10 kHz (nominal: 4 kHz)
- ✅ **Control de volumen** - Duty cycle 0-100% (volumen ajustable)
- ✅ **Thread-safe** - Opcional con mutex FreeRTOS
- ✅ **Funciones de beep** - Beeps simples y secuencias
- ✅ **Patrones de alarma** - Para diferentes zonas del cerco

## 🔧 Hardware

- **Timer**: TIM1
- **Channel**: CH3 (TIM_CHANNEL_3)
- **GPIO**: Configurado en STM32CubeMX (típicamente PA10 para TIM1_CH3)
- **Frecuencia nominal**: 4 kHz
- **Clock source**: APB2 Timer (48 MHz en STM32WL55)

## 📁 Archivos

```
CM4/Core/
├── Inc/
│   └── Modules/
│       └── Buzzer/
│           └── buzzer.h              # API del módulo
└── Src/
    └── Modules/
        └── Buzzer/
            ├── buzzer.c              # Implementación
            └── buzzer_example.c      # Ejemplos de uso
```

## 🚀 Uso Básico

### 1. Inicialización

```c
#include "buzzer.h"

extern TIM_HandleTypeDef htim1;  // Definido en main.c

// Configurar buzzer (4 kHz, 50% duty cycle)
BuzzerConfig_t config = {
    .htim = &htim1,
    .channel = TIM_CHANNEL_3,
    .frequency_hz = 4000,  // 4 kHz
    .duty_cycle = 50       // 50% volumen
};

// Inicializar
if (Buzzer_Init(&config) == BUZZER_OK) {
    printf("Buzzer inicializado correctamente\r\n");
}
```

### 2. Control On/Off

```c
// Encender buzzer
Buzzer_On();

HAL_Delay(1000);  // Esperar 1 segundo

// Apagar buzzer
Buzzer_Off();

// Toggle ON/OFF
Buzzer_Toggle();
```

### 3. Control de Frecuencia

```c
// Cambiar a 3 kHz
Buzzer_SetFrequency(3000);
Buzzer_On();
HAL_Delay(500);
Buzzer_Off();

// Cambiar a 5 kHz
Buzzer_SetFrequency(5000);
Buzzer_On();
HAL_Delay(500);
Buzzer_Off();
```

### 4. Control de Volumen (Duty Cycle)

```c
// Volumen bajo (25%)
Buzzer_SetDutyCycle(25);
Buzzer_On();
HAL_Delay(500);
Buzzer_Off();

// Volumen medio (50%)
Buzzer_SetDutyCycle(50);
Buzzer_On();
HAL_Delay(500);
Buzzer_Off();

// Volumen alto (75%)
Buzzer_SetDutyCycle(75);
Buzzer_On();
HAL_Delay(500);
Buzzer_Off();
```

### 5. Beeps Simples

```c
// Beep de 200ms a 4 kHz con 50% volumen
Buzzer_Beep(4000, 50, 200);

// Beep de advertencia (3 kHz, 75%, 500ms)
Buzzer_Beep(3000, 75, 500);

// Secuencia de 3 beeps (100ms on, 100ms off)
Buzzer_BeepSequence(3, 100, 100);
```

## 🎵 Patrones de Alarma por Zona

### Zona Verde (GREEN_ZONE)
Sin estímulo - Vaca dentro del cerco seguro

### Zona Azul Claro (LIGHT_BLUE_ZONE)
```c
Buzzer_Beep(2500, 20, 100);  // Suave advertencia
```

### Zona Azul (BLUE_ZONE)
```c
Buzzer_Beep(3000, 35, 150);  // Advertencia media
```

### Zona Azul Oscuro (DARK_BLUE_ZONE)
```c
Buzzer_Beep(3500, 50, 200);  // Advertencia fuerte
```

### Zona Amarilla (YELLOW_ZONE)
```c
Buzzer_BeepSequence(3, 150, 100);  // Secuencia intensa
```

### Zona Roja (RED_ZONE)
```c
for (int i = 0; i < 5; i++) {
    Buzzer_Beep(5000, 75, 100);
    HAL_Delay(50);
}
```

### Zona Negra (BLACK_ZONE - Escape)
```c
Buzzer_SetParams(4500, 80);
Buzzer_On();
HAL_Delay(3000);  // Alarma continua 3 segundos
Buzzer_Off();
```

## 📊 API Completa

### Funciones de Inicialización
- `Buzzer_Init()` - Inicializar módulo
- `Buzzer_DeInit()` - Desinicializar módulo

### Control Básico
- `Buzzer_On()` - Encender buzzer
- `Buzzer_Off()` - Apagar buzzer
- `Buzzer_Toggle()` - Alternar estado

### Configuración
- `Buzzer_SetFrequency(uint32_t frequency_hz)` - Ajustar frecuencia (1-10 kHz)
- `Buzzer_SetDutyCycle(uint8_t duty_cycle)` - Ajustar volumen (0-100%)
- `Buzzer_SetParams(uint32_t frequency_hz, uint8_t duty_cycle)` - Ajustar ambos

### Funciones de Beep
- `Buzzer_Beep(frequency, duty, duration_ms)` - Beep simple
- `Buzzer_BeepSequence(count, on_time_ms, off_time_ms)` - Secuencia de beeps

### Estado y Monitoreo
- `Buzzer_GetState()` - Obtener estado actual
- `Buzzer_IsInitialized()` - Verificar inicialización
- `Buzzer_IsEnabled()` - Verificar si está encendido

## ⚙️ Configuración STM32CubeMX

### TIM1 Configuration

1. **Clock Configuration**:
   - APB2 Timer Clock: 48 MHz

2. **TIM1 Settings**:
   - Mode: PWM Generation CH3
   - Prescaler: 0 (calculado internamente)
   - Counter Mode: Up
   - Counter Period: Calculado automáticamente según frecuencia
   - Pulse (CCR3): Calculado según duty cycle

3. **GPIO Settings**:
   - PA10: TIM1_CH3
   - Mode: Alternate Function Push Pull
   - Pull: No pull-up, no pull-down
   - Speed: Low/Medium

### Código de Inicialización en main.c

```c
/* USER CODE BEGIN 2 */

// Timer ya inicializado por MX_TIM1_Init()
// Solo falta configurar el buzzer module

BuzzerConfig_t buzzer_config = {
    .htim = &htim1,
    .channel = TIM_CHANNEL_3,
    .frequency_hz = 4000,
    .duty_cycle = 50
};

Buzzer_Init(&buzzer_config);

/* USER CODE END 2 */
```

## 🧪 Testing

Ejecutar todos los ejemplos:

```c
// En main.c antes de osKernelStart()
buzzer_run_all_examples();
```

Esto ejecutará:
1. Control básico On/Off
2. Barrido de frecuencias
3. Control de volumen
4. Funciones de beep
5. Patrones de alarma
6. Estímulos por zona
7. Monitoreo de estado

## 📐 Cálculos de Timer

### Frecuencia
```
ARR = (TIM_CLOCK / Frecuencia_Deseada) - 1
ARR = (48,000,000 / 4,000) - 1 = 11,999
```

### Duty Cycle
```
CCR = ARR × (DutyCycle / 100)
CCR = 11,999 × (50 / 100) = 5,999
```

Para 4 kHz con 50% duty:
- ARR = 11,999
- CCR3 = 5,999

## 🔒 Thread Safety

Para habilitar thread safety en entornos FreeRTOS:

1. Descomentar en `buzzer.c`:
```c
#define BUZZER_USE_MUTEX
```

2. El módulo creará automáticamente un mutex recursivo

3. Todas las operaciones serán thread-safe

## ⚡ Performance

- **ROM**: ~1.5 KB
- **RAM**: ~100 bytes (estado + configuración)
- **Latencia On/Off**: < 100 µs
- **Cambio frecuencia**: < 50 µs

## 📝 Notas

- El buzzer usa TIM1 CH3 exclusivamente
- No usar `HAL_TIM_PWM_Start()` directamente después de inicializar
- Duty cycle 0% = silencio, 100% = volumen máximo
- Frecuencias recomendadas: 2-6 kHz para mejor audibilidad
- Para producción, considerar agregar anti-rebote por software

## 🐛 Troubleshooting

**Problema**: No se escucha el buzzer
- Verificar GPIO configurado correctamente en CubeMX
- Verificar conexión del buzzer al pin PA10
- Verificar que duty cycle > 0
- Verificar que `Buzzer_On()` fue llamado

**Problema**: Frecuencia incorrecta
- Verificar TIM_CLOCK_FREQ en buzzer.c (debe ser 48 MHz)
- Verificar reloj APB2 en CubeMX

**Problema**: Volumen muy bajo
- Aumentar duty cycle (50-75%)
- Verificar alimentación del buzzer
- Verificar impedancia del buzzer

## 📖 Referencias

- [STM32WL55 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0461-stm32wl5x-advanced-armbased-32bit-mcus-with-subghz-radio-solution-stmicroelectronics.pdf)
- [TIM1 PWM Configuration](https://www.st.com/resource/en/application_note/dm00236305-generalpurpose-timer-cookbook-for-stm32-microcontrollers-stmicroelectronics.pdf)

---

**Última actualización**: 24 de Noviembre de 2025  
**Versión**: v1.0  
**Autor**: TPP-IntelliFence Team
