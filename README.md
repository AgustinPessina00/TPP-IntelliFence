# TPP-IntelliFence - Sistema de Cerco Virtual Inteligente

Sistema embebido de monitoreo y control de ganado mediante cerco virtual inteligente, implementado en STM32WL55JC dual-core con FreeRTOS.

## 🎯 Características Principales

- **Arquitectura RTOS-Safe**: Constructores triviales + inicialización explícita
- **Zero Dynamic Allocation**: Sistema de mensajes con pool estático
- **Thread-Safe I2C/UART**: Managers con mutex para acceso concurrente
- **Comprehensive Testing**: Validación completa de hardware antes de RTOS
- **Optimización de Memoria**: ROM ~95KB / RAM1 ~11.5KB (stack optimizado)

## 🔧 Configuración del Entorno de Desarrollo

### Prerequisitos
- **STM32CubeCLT** (STM32Cube Command Line Tools) 1.19.0+
- **CMake** 3.22+
- **ARM GCC Toolchain** 13.3.1+st.9
- **Git**
- **VS Code** (recomendado) con extensiones:
  - C/C++ Extension Pack
  - CMake Tools
  - Cortex-Debug

### 🚀 Instalación y Compilación

#### 1. Clonar el Repositorio
```bash
git clone https://github.com/AgustinPessina00/TPP-IntelliFence.git
cd TPP-IntelliFence
git checkout develop
```

#### 2. Configurar el Proyecto
```bash
# Configurar CMake con preset de Debug
cmake --preset Debug

# Compilar el proyecto completo
cmake --build build/Debug
```

#### 3. Compilación Específica por Core
```bash
# Solo CM4 (Cortex-M4)
cd CM4 && cmake --build build

# Solo CM0PLUS (Cortex-M0+)  
cd CM0PLUS && cmake --build build
```

## 📁 Estructura del Proyecto

```
TPP-IntelliFence/
├── CM4/                          # Cortex-M4 Core (Principal)
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── Modules/          # Headers de módulos
│   │   │   │   ├── GPS/          # SAM-M10Q GPS
│   │   │   │   ├── INA/          # INA226 Current/Power
│   │   │   │   ├── IMU/          # LSM6DSO Accelerometer/Gyro
│   │   │   │   ├── I2C/          # Thread-Safe I2C Manager
│   │   │   │   ├── UART/         # Thread-Safe UART Manager
│   │   │   │   ├── Messages/     # Sistema de mensajes
│   │   │   │   └── Buzzer/       # Control PWM buzzer
│   │   │   ├── Threads/          # Headers de tareas FreeRTOS
│   │   │   └── Types/            # Tipos de datos (Cow, Fence, Zone)
│   │   └── Src/
│   │       ├── main.c            # Punto de entrada + comprehensive tests
│   │       ├── app_freertos.c    # Configuración FreeRTOS
│   │       ├── rtos_printf.c     # Printf thread-safe
│   │       ├── Modules/          # Implementaciones de módulos
│   │       ├── Threads/          # Tareas FreeRTOS (FSM, dispatcher, etc)
│   │       └── Types/            # Modelo de datos (Cow, Fence)
│   └── CMakeLists.txt            # Build configuration
├── CM0PLUS/                      # Cortex-M0+ Core (Radio LoRa)
├── Common/                       # Código compartido entre cores
├── Drivers/                      # STM32 HAL, CMSIS, BSP
├── Middlewares/                  # FreeRTOS
└── docs/                         # Documentación técnica
```

## 🔌 Hardware y Sensores

### GPS SAM-M10Q
- **Protocolo:** UART (USART1) con parsing NMEA y UBX
- **API Refactorizada:** `SamM10q()` + `init(uint8_t address)`
- **Características:** Posicionamiento, configuración baudrate, modo bajo consumo
- **Test:** `gps_test_wrapper.cpp`, `gps_test_melopero_wrapper.cpp`

### INA226 Current/Power Monitor (×3)
- **Protocolo:** I2C2 thread-safe
- **Direcciones:** 0x40 (MCU), 0x41 (GPS), 0x45 (IMU)
- **API Refactorizada:** `Ina226()` + `init(address, rShunt, currentLSB, ...)`
- **Mediciones:** Corriente, voltaje bus/shunt, potencia
- **Test:** `ina226_comprehensive_test()`, `ina226_configuration_test()`

### LSM6DSO IMU 6DOF
- **Protocolo:** I2C2 thread-safe
- **Dirección:** 0x6A o 0x6B
- **API Refactorizada:** `Lsm6dso()` + `init(address, i3c, odrAcc, fsAcc, ...)`
- **Sensores:** Acelerómetro ±2g/±4g/±8g/±16g, Giroscopio ±250dps/±2000dps
- **Test:** `lsm6dso_comprehensive_test()`, `lsm6dso_configuration_test()`

### Buzzer PWM
- **Control:** TIM1 PWM
- **Funciones:** Tonos, melodías, alarmas configurables
- **Test:** `buzzer_run_all_examples()`

## 🧪 Comprehensive Module Tests

Los tests se ejecutan automáticamente en `main.c` antes del inicio de FreeRTOS mediante `run_comprehensive_module_tests()`:

### Suite de Tests Incluida
1. **Embedded Message System**: Pool estático, zero allocation
2. **INA226 Power Monitors**: MCU, GPS, IMU (3 sensores)
3. **LSM6DSO IMU**: Acelerómetro y giroscopio 6DOF
4. **SAM-M10Q GPS**: NMEA y UBX protocols
5. **Cow & Fence Data Model**: Clases de negocio
6. **Buzzer Actuator**: Tonos y alarmas PWM

### Validaciones por Test
- ✅ Conectividad I2C: Scan de direcciones
- ✅ Verificación de registros: WHO_AM_I, configuración
- ✅ Mediciones en tiempo real: Estadísticas y promedios
- ✅ Configuraciones múltiples: Diferentes rangos y resoluciones
- ✅ API refactorizada: Constructor trivial + init() explícito

### Control de Tests
Para habilitar/deshabilitar tests, comentar líneas en `run_comprehensive_module_tests()` en `main.c`:

```c
// Deshabilitar test de buzzer (ahorra tiempo)
// buzzer_run_all_examples();
```

## ⚙️ Configuración Hardware

### I2C Bus (I2C2)
- **SDA:** PB14
- **SCL:** PB13  
- **Velocidad:** 100kHz (configurar según necesidad)

### UART GPS (USART1)
- **TX:** PA9
- **RX:** PA10
- **Baudrate:** 9600 (configurable hasta 115200)

### Alimentación Sensores
- **VCC:** 3.3V
- **GND:** Tierra común

## 🔄 Arquitectura RTOS-Safe

### Patrón de Inicialización Refactorizado

**ANTES** (stack overflow, non-deterministic):
```cpp
SamM10q gps(0x42);  // ❌ Constructor hace HAL calls
gps.initialize();   // ❌ Métodos inconsistentes
```

**DESPUÉS** (RTOS-safe, deterministic):
```cpp
static SamM10q gps;        // ✅ Constructor trivial (no HAL)
if (!gps.init(0x42)) {     // ✅ Inicialización explícita
    // Error handling
}
```

### Thread-Safe I2C/UART Managers

```cpp
// Inicialización única (antes de FreeRTOS)
I2CManager::initializeAll();
UARTManager::initializeAll();

// Acceso thread-safe desde cualquier tarea
I2CBus& i2c = I2CManager::getBus2();
UARTBus& uart = UARTManager::getUart1();

// Operaciones protegidas con mutex interno
i2c.memRead(address, reg, data, length);
uart.transmit(data, length, timeout);
```

### Sistema de Mensajes FreeRTOS

```cpp
// Pool estático (zero dynamic allocation)
MessagePool_Init();  // En MX_FREERTOS_Init()

// Obtener mensaje del pool
EmbeddedMessage_t* msg = MessagePool_Alloc();
msg->id = MSG_SENSOR_DATA;
msg->sender = MODULE_SENSOR_ACQ;
msg->receiver = MODULE_FSM;

// Enviar a cola FreeRTOS
osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 0);

// Liberar cuando termine
MessagePool_Free(msg);
```

### FreeRTOS Tasks (Optimizadas)

```c
// Stack sizes conservadores post-refactor
dispatcher_Task:  768 bytes  (solo ruteo de punteros)
fsm_Task:         1024 bytes (lógica FSM, objetos static)
stimulus_Task:    512 bytes  (control buzzer)
sensorAcq_Task:   1024 bytes (5 sensores como static)
// Total: ~3.3KB stack (antes ~4.25KB)
```

## 🐛 Debugging y Troubleshooting

### Problemas Comunes

#### Error de Compilación
```bash
# Limpiar build y reconfigurar
rm -rf build/
cmake --preset Debug
cmake --build build/Debug
```

#### Sensor No Detectado
- Verificar conexiones I2C (SDA/SCL)
- Comprobar alimentación 3.3V
- Usar test de conectividad: `I2CBus::scanDevices()`

#### GPS Sin Fix
- Esperar hasta 30s para cold start
- Verificar antena conectada
- Comprobar baudrate UART

### Logs Thread-Safe

El sistema implementa `rtos_printf` para logging desde tareas FreeRTOS:

```c
// Inicialización (en MX_FREERTOS_Init)
rtos_printf_init();

// Logging thread-safe desde cualquier tarea
RTOS_LOG_INFO("[SENSOR] GPS Lat: %.6f, Lon: %.6f\n", lat, lon);
RTOS_LOG_DEBUG("[INA226] Current: %.2f mA\n", current);
RTOS_LOG_ERROR("[IMU] Failed to read acceleration\n");
```

## 📊 Uso de Memoria

### ROM (Flash) - 128 KB
- Usado: ~95.4 KB (74.5%)
- Libre: ~32.6 KB
- **Crítico**: Cerca del límite, comprehensive tests ocupan ~50KB

### RAM1 - 16 KB
- Usado: ~11.5 KB (70.6%)
- Libre: ~4.5 KB
- Distribución: FreeRTOS heap + stacks + BSS/Data

### RAM2 - 16 KB
- Usado: 0 KB (reservado para heap FreeRTOS)
- Configurado en `heap_config.c` para heap_5

## 🚨 Limitaciones Conocidas

1. **ROM casi llena**: Los comprehensive tests agregan ~50KB. Considerar:
   - Compilar en Release mode (-Os) en lugar de Debug (-O0)
   - Deshabilitar tests no esenciales en producción
   - Usar conditional compilation para tests

2. **Stack limitado**: Objetos grandes deben ser `static`:
   ```cpp
   // ❌ MAL: Stack overflow
   void task() { SamM10q gps; }
   
   // ✅ BIEN: Static storage
   void task() { static SamM10q gps; }
   ```

## 🤝 Contribuir

Ver documentación en `docs/` para:
- `01_architecture/`: Diseño del sistema, FSM, messaging
- `02_implementation/`: Guías de implementación (RTOS printf, thread-safety)
- `03_testing/`: Procedimientos de testing
- `04_status/`: Estado actual y roadmap
