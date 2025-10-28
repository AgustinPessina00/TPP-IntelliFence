#ifndef LSM6DSO_MODULE_H
#define LSM6DSO_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
}
#include "II2C.h"

/**
 * @brief Estructura para datos del acelerómetro LSM6DSO
 */
struct Lsm6dsoAccelData {
    float x;        ///< Aceleración en X (mg)  
    float y;        ///< Aceleración en Y (mg)
    float z;        ///< Aceleración en Z (mg)
    bool isValid;   ///< true si los datos son válidos
};

/**
 * @brief Estructura para datos del giroscopio LSM6DSO
 */
struct Lsm6dsoGyroData {
    float x;        ///< Velocidad angular en X (mdps)
    float y;        ///< Velocidad angular en Y (mdps) 
    float z;        ///< Velocidad angular en Z (mdps)
    bool isValid;   ///< true si los datos son válidos
};

/**
 * @brief Estructura para datos de temperatura LSM6DSO
 */
struct Lsm6dsoTempData {
    float celsius;  ///< Temperatura en grados Celsius
    bool isValid;   ///< true si los datos son válidos
};

/**
 * @brief Configuración del rango del acelerómetro LSM6DSO
 */
enum Lsm6dsoAccelRange {
    LSM6DSO_ACCEL_RANGE_2G = 0,   ///< ±2g
    LSM6DSO_ACCEL_RANGE_4G = 1,   ///< ±4g
    LSM6DSO_ACCEL_RANGE_8G = 2,   ///< ±8g
    LSM6DSO_ACCEL_RANGE_16G = 3   ///< ±16g
};

/**
 * @brief Configuración del rango del giroscopio LSM6DSO
 */
enum Lsm6dsoGyroRange {
    LSM6DSO_GYRO_RANGE_250DPS = 0,    ///< ±250°/s
    LSM6DSO_GYRO_RANGE_500DPS = 1,    ///< ±500°/s  
    LSM6DSO_GYRO_RANGE_1000DPS = 2,   ///< ±1000°/s
    LSM6DSO_GYRO_RANGE_2000DPS = 3    ///< ±2000°/s
};

/**
 * @brief Configuración ODR (Output Data Rate) del acelerómetro
 */
enum Lsm6dsoAccelOdr {
    LSM6DSO_ACCEL_POWER_DOWN = 0,   ///< Power down
    LSM6DSO_ACCEL_12_5_HZ = 1,      ///< 12.5 Hz
    LSM6DSO_ACCEL_26_HZ = 2,        ///< 26 Hz
    LSM6DSO_ACCEL_52_HZ = 3,        ///< 52 Hz
    LSM6DSO_ACCEL_104_HZ = 4,       ///< 104 Hz
    LSM6DSO_ACCEL_208_HZ = 5,       ///< 208 Hz
    LSM6DSO_ACCEL_416_HZ = 6,       ///< 416 Hz
    LSM6DSO_ACCEL_833_HZ = 7        ///< 833 Hz
};

/**
 * @brief Módulo IMU LSM6DSO usando I2C
 * 
 * Este módulo actúa como wrapper de la implementación existente del LSM6DSO
 * pero usando nuestra arquitectura I2C thread-safe. NO crea mutex propio.
 */
class Lsm6dsoModule {
private:
    II2C& i2cBus;                    ///< Referencia al bus I2C thread-safe
    uint16_t deviceAddr;             ///< Dirección I2C del LSM6DSO (7-bit)
    uint32_t timeout;                ///< Timeout para operaciones I2C
    
    // Configuración actual
    Lsm6dsoAccelRange accelRange;    ///< Rango actual del acelerómetro
    Lsm6dsoGyroRange gyroRange;      ///< Rango actual del giroscopio
    Lsm6dsoAccelOdr accelOdr;        ///< ODR actual del acelerómetro
    
    // Factores de escala para conversión
    float accelScale;                ///< Factor de escala del acelerómetro (mg/LSB)
    float gyroScale;                 ///< Factor de escala del giroscopio (mdps/LSB)
    
    // Últimos datos válidos
    Lsm6dsoAccelData lastAccel;      ///< Últimos datos del acelerómetro
    Lsm6dsoGyroData lastGyro;        ///< Últimos datos del giroscopio
    Lsm6dsoTempData lastTemp;        ///< Últimos datos de temperatura
    
    /**
     * @brief Actualiza los factores de escala según la configuración
     */
    void updateScaleFactors();
    
    /**
     * @brief Convierte datos raw del acelerómetro a mg
     * @param rawValue Valor raw de 16-bit con signo
     * @return Valor en mg
     */
    float convertAccelRaw(int16_t rawValue);
    
    /**
     * @brief Convierte datos raw del giroscopio a mdps
     * @param rawValue Valor raw de 16-bit con signo
     * @return Valor en mdps (milli-degrees per second)
     */
    float convertGyroRaw(int16_t rawValue);
    
    /**
     * @brief Convierte datos raw de temperatura a °C
     * @param rawValue Valor raw de 16-bit con signo
     * @return Valor en °C
     */
    float convertTempRaw(int16_t rawValue);

public:
    /**
     * @brief Constructor del módulo LSM6DSO
     * @param i2cBus Referencia al bus I2C thread-safe
     * @param addr Dirección I2C del LSM6DSO (default: 0x6A)
     * @param timeout Timeout para operaciones I2C en ms (default: 1000)
     */
    explicit Lsm6dsoModule(II2C& i2cBus, uint16_t addr = 0x6A, uint32_t timeout = 1000);
    
    /**
     * @brief Inicializa el módulo LSM6DSO
     * @return I2CResult código de resultado
     */
    I2CResult initialize();
    
    /**
     * @brief Verifica si el LSM6DSO está conectado y responde
     * @return true si está conectado
     */
    bool isConnected();
    
    /**
     * @brief Lee el registro WHO_AM_I para verificar identidad
     * @param whoAmI Puntero donde almacenar el valor (esperado: 0x6C)
     * @return I2CResult código de resultado
     */
    I2CResult readWhoAmI(uint8_t* whoAmI);
    
    /**
     * @brief Configura el rango del acelerómetro
     * @param range Rango deseado
     * @return I2CResult código de resultado
     */
    I2CResult setAccelRange(Lsm6dsoAccelRange range);
    
    /**
     * @brief Configura el rango del giroscopio
     * @param range Rango deseado
     * @return I2CResult código de resultado
     */
    I2CResult setGyroRange(Lsm6dsoGyroRange range);
    
    /**
     * @brief Configura el ODR (Output Data Rate) del acelerómetro
     * @param odr ODR deseado
     * @return I2CResult código de resultado
     */
    I2CResult setAccelOdr(Lsm6dsoAccelOdr odr);
    
    /**
     * @brief Lee datos del acelerómetro
     * @param accel Estructura donde almacenar los datos
     * @return I2CResult código de resultado
     */
    I2CResult readAccel(Lsm6dsoAccelData& accel);
    
    /**
     * @brief Lee datos del giroscopio
     * @param gyro Estructura donde almacenar los datos
     * @return I2CResult código de resultado
     */
    I2CResult readGyro(Lsm6dsoGyroData& gyro);
    
    /**
     * @brief Lee temperatura del sensor
     * @param temp Estructura donde almacenar los datos
     * @return I2CResult código de resultado
     */
    I2CResult readTemperature(Lsm6dsoTempData& temp);
    
    /**
     * @brief Lee todos los datos del IMU en una sola operación
     * @param accel Estructura para datos del acelerómetro
     * @param gyro Estructura para datos del giroscopio  
     * @param temp Estructura para datos de temperatura
     * @return I2CResult código de resultado
     */
    I2CResult readAll(Lsm6dsoAccelData& accel, Lsm6dsoGyroData& gyro, Lsm6dsoTempData& temp);
    
    /**
     * @brief Obtiene últimos datos válidos del acelerómetro
     * @return Referencia a los últimos datos
     */
    const Lsm6dsoAccelData& getLastAccel() const { return lastAccel; }
    
    /**
     * @brief Obtiene últimos datos válidos del giroscopio
     * @return Referencia a los últimos datos
     */
    const Lsm6dsoGyroData& getLastGyro() const { return lastGyro; }
    
    /**
     * @brief Obtiene últimos datos válidos de temperatura
     * @return Referencia a los últimos datos
     */
    const Lsm6dsoTempData& getLastTemp() const { return lastTemp; }
    
    /**
     * @brief Habilita/deshabilita el acelerómetro
     * @param enable true para habilitar, false para deshabilitar
     * @return I2CResult código de resultado
     */
    I2CResult enableAccel(bool enable);
    
    /**
     * @brief Habilita/deshabilita el giroscopio
     * @param enable true para habilitar, false para deshabilitar
     * @return I2CResult código de resultado
     */
    I2CResult enableGyro(bool enable);
    
    /**
     * @brief Realiza soft reset del dispositivo
     * @return I2CResult código de resultado
     */
    I2CResult softReset();
};

#endif // __cplusplus

// ========== INTERFAZ C ==========

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estructura C para datos del acelerómetro LSM6DSO
 */
typedef struct {
    float x, y, z;      // En mg
    uint8_t isValid;    // 1 = válido, 0 = inválido
} Lsm6dsoAccelData_C;

/**
 * @brief Estructura C para datos del giroscopio LSM6DSO
 */
typedef struct {
    float x, y, z;      // En mdps
    uint8_t isValid;    // 1 = válido, 0 = inválido
} Lsm6dsoGyroData_C;

/**
 * @brief Estructura C para datos de temperatura LSM6DSO
 */
typedef struct {
    float celsius;
    uint8_t isValid;    // 1 = válido, 0 = inválido
} Lsm6dsoTempData_C;

/**
 * @brief Inicializa el módulo LSM6DSO con dirección por defecto
 * @return 0=OK, otros=error
 */
int lsm6dsoModuleInit(void);

/**
 * @brief Inicializa el módulo LSM6DSO con dirección específica
 * @param addr Dirección I2C del LSM6DSO (7-bit)
 * @return 0=OK, otros=error
 */
int lsm6dsoModuleInitWithAddr(uint16_t addr);

/**
 * @brief Verifica conexión con LSM6DSO
 * @return 1 si conectado, 0 si no
 */
int lsm6dsoIsConnected(void);

/**
 * @brief Lee datos del acelerómetro
 * @param accel Puntero a estructura para almacenar datos
 * @return 0=OK, otros=error
 */
int lsm6dsoReadAccel(Lsm6dsoAccelData_C* accel);

/**
 * @brief Lee datos del giroscopio
 * @param gyro Puntero a estructura para almacenar datos
 * @return 0=OK, otros=error
 */
int lsm6dsoReadGyro(Lsm6dsoGyroData_C* gyro);

/**
 * @brief Lee temperatura del sensor
 * @param temp Puntero a estructura para almacenar datos
 * @return 0=OK, otros=error
 */
int lsm6dsoReadTemperature(Lsm6dsoTempData_C* temp);

/**
 * @brief Lee todos los datos del LSM6DSO
 * @param accel Puntero a estructura para acelerómetro
 * @param gyro Puntero a estructura para giroscopio
 * @param temp Puntero a estructura para temperatura
 * @return 0=OK, otros=error
 */
int lsm6dsoReadAll(Lsm6dsoAccelData_C* accel, Lsm6dsoGyroData_C* gyro, Lsm6dsoTempData_C* temp);

/**
 * @brief Configura rango del acelerómetro
 * @param range 0=±2g, 1=±4g, 2=±8g, 3=±16g
 * @return 0=OK, otros=error
 */
int lsm6dsoSetAccelRange(uint8_t range);

/**
 * @brief Configura rango del giroscopio
 * @param range 0=±250°/s, 1=±500°/s, 2=±1000°/s, 3=±2000°/s
 * @return 0=OK, otros=error
 */
int lsm6dsoSetGyroRange(uint8_t range);

#ifdef __cplusplus
}
#endif

#endif // LSM6DSO_MODULE_H