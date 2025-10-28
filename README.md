# TPP-IntelliFence - Sistema de Cerco Virtual Inteligente

## 🔧 Configuración del Entorno de Desarrollo

### Prerequisitos
- **STM32CubeCLT** (STM32Cube Command Line Tools)
- **CMake** 3.16+
- **Git**
- **VS Code** (recomendado) con extensiones:
  - C/C++
  - CMake Tools
  - Cortex-Debug (para debugging)

### 🚀 Instalación y Compilación

#### 1. Clonar el Repositorio
```bash
git clone https://github.com/AgustinPessina00/TPP-IntelliFence.git
cd TPP-IntelliFence
git checkout vscode-refactor
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
│   │   ├── Inc/                  # Headers generales
│   │   └── Src/                  # Código fuente principal
│   │       ├── main.c            # Punto de entrada
│   │       ├── *_test_wrapper.cpp # Tests de sensores
│   │       └── *.cpp/.c          # Implementaciones
│   └── Modules/                  # Módulos de sensores
│       ├── GPS/                  # SAM-M10Q GPS
│       ├── INA/                  # INA226 Current/Power
│       ├── IMU/                  # LSM6DSO Accelerometer/Gyro
│       └── I2C/                  # Thread-Safe I2C Bus
├── CM0PLUS/                      # Cortex-M0+ Core (Auxiliar)
├── Common/                       # Código compartido entre cores
├── Drivers/                      # STM32 HAL, CMSIS, FreeRTOS
└── CMakeLists.txt               # Configuración CMake principal
```

## 🔌 Sensores Integrados

### GPS SAM-M10Q
- **Protocolo:** UART con parsing NMEA y UBX
- **Funciones:** Posicionamiento, configuración de baudrate
- **Test:** `gps_test_wrapper.cpp` y `gps_test_melopero_wrapper.cpp`

### INA226 Current/Power Monitor
- **Protocolo:** I2C thread-safe
- **Direcciones:** 0x40, 0x41, 0x45 (multi-sensor)
- **Funciones:** Medición de corriente, voltaje, potencia
- **Test:** `ina226_test_wrapper.cpp`

### LSM6DSO IMU 6DOF
- **Protocolo:** I2C thread-safe  
- **Funciones:** Acelerómetro, giroscopio, temperatura
- **Test:** `lsm6dso_test_wrapper.cpp`

## 🧪 Pruebas y Validación

### Ejecutar Tests
Cada sensor tiene su test wrapper. Para cambiar el test activo, edita `CM4/Core/Src/main.c`:

```cpp
// Cambiar entre estos tests:
gps_comprehensive_test();           // GPS NMEA
gps_melopero_comprehensive_test();  // GPS UBX  
ina226_comprehensive_test();        // INA226 Power
lsm6dso_comprehensive_test();       // LSM6DSO IMU
```

### Tests Disponibles
- **Conectividad I2C:** Detección automática de direcciones
- **Verificación de registros:** WHO_AM_I, configuración
- **Mediciones en tiempo real:** Con estadísticas y promedios
- **Configuraciones múltiples:** Diferentes rangos y resoluciones

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

## 🔄 Arquitectura Thread-Safe

### I2C Manager
```cpp
// Inicialización automática
I2CManager::initializeAll();

// Obtener bus I2C
I2CBus& bus = I2CManager::getBus2();

// Operaciones thread-safe
I2CResult result = bus.memRead(address, reg, data, length);
```

### Patrón de Sensores
```cpp
// Constructor
SensorClass sensor(&i2cBus, deviceAddress);

// Inicialización con error handling
if (!sensor.initialize()) {
    printf("ERROR: Sensor initialization failed\n");
}

// Lecturas thread-safe
I2CResult result = sensor.readData();
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

### Logs de Debug
El sistema usa printf con soporte floating point:
```cpp
printf("GPS Lat: %.6f, Lon: %.6f\n", latitude, longitude);
printf("INA226 Current: %.2f mA\n", current_mA);
printf("LSM6DSO Accel: X=%.2f, Y=%.2f, Z=%.2f g\n", ax, ay, az);
```
