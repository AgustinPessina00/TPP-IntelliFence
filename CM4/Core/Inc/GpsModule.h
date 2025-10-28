#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
}
#include "II2C.h"

/**
 * @brief Estructura para datos de posicionamiento GPS
 */
struct GpsPosition {
    double latitude;     ///< Latitud en grados decimales
    double longitude;    ///< Longitud en grados decimales
    float altitude;      ///< Altitud en metros
    uint8_t satellites;  ///< Número de satélites visibles
    bool isValid;        ///< true si la posición es válida
};

/**
 * @brief Estructura para datos de tiempo GPS
 */
struct GpsTime {
    uint8_t hour;        ///< Hora (0-23)
    uint8_t minute;      ///< Minuto (0-59)
    uint8_t second;      ///< Segundo (0-59)
    uint16_t year;       ///< Año
    uint8_t month;       ///< Mes (1-12)
    uint8_t day;         ///< Día (1-31)
    bool isValid;        ///< true si el tiempo es válido
};

/**
 * @brief Estructura para estado del GPS
 */
struct GpsStatus {
    bool isConnected;    ///< true si el GPS responde
    uint8_t fixType;     ///< 0=No fix, 1=Dead reckoning, 2=2D, 3=3D
    float hdop;          ///< Dilución horizontal de precisión
    uint32_t lastUpdate; ///< Timestamp de última actualización (ms)
};

/**
 * @brief Módulo GPS u-blox usando protocolo I2C
 * 
 * Este módulo maneja comunicación con GPS u-blox usando el protocolo UBX
 * sobre I2C. NO crea mutex propio - usa la infraestructura thread-safe
 * del I2CBus inyectado.
 */
class GpsModule {
private:
    II2C& i2cBus;                ///< Referencia al bus I2C thread-safe
    uint16_t deviceAddr;         ///< Dirección I2C del GPS (7-bit)
    uint32_t timeout;            ///< Timeout para operaciones I2C
    
    // Buffers para comunicación UBX
    uint8_t txBuffer[64];        ///< Buffer de transmisión
    uint8_t rxBuffer[128];       ///< Buffer de recepción
    
    // Estado interno
    GpsPosition lastPosition;    ///< Última posición válida
    GpsTime lastTime;           ///< Último tiempo válido
    GpsStatus status;           ///< Estado actual del GPS
    
    /**
     * @brief Calcula checksum UBX para un mensaje
     * @param data Datos del mensaje (sin header ni checksum)
     * @param length Longitud de los datos
     * @param ckA Puntero para checksum A
     * @param ckB Puntero para checksum B
     */
    void calculateUbxChecksum(const uint8_t* data, uint16_t length, 
                             uint8_t* ckA, uint8_t* ckB);
    
    /**
     * @brief Construye un mensaje UBX completo
     * @param msgClass Clase del mensaje UBX
     * @param msgId ID del mensaje UBX
     * @param payload Datos del payload (puede ser nullptr)
     * @param payloadLength Longitud del payload
     * @return Longitud total del mensaje construido
     */
    uint16_t buildUbxMessage(uint8_t msgClass, uint8_t msgId, 
                            const uint8_t* payload, uint16_t payloadLength);
    
    /**
     * @brief Verifica si una respuesta UBX es válida
     * @param data Buffer con la respuesta
     * @param length Longitud de la respuesta
     * @return true si la respuesta es válida UBX
     */
    bool isValidUbxResponse(const uint8_t* data, uint16_t length);
    
    /**
     * @brief Procesa un mensaje UBX de posición (NAV-PVT)
     * @param payload Datos del payload NAV-PVT
     * @param length Longitud del payload
     * @return true si se procesó correctamente
     */
    bool processNavPvtMessage(const uint8_t* payload, uint16_t length);
    
    /**
     * @brief Procesa un mensaje UBX de estado (NAV-STATUS)
     * @param payload Datos del payload NAV-STATUS
     * @param length Longitud del payload
     * @return true si se procesó correctamente
     */
    bool processNavStatusMessage(const uint8_t* payload, uint16_t length);

public:
    /**
     * @brief Constructor del módulo GPS
     * @param i2cBus Referencia al bus I2C thread-safe
     * @param addr Dirección I2C del GPS u-blox (default: 0x42)
     * @param timeout Timeout para operaciones I2C en ms (default: 2000)
     */
    explicit GpsModule(II2C& i2cBus, uint16_t addr = 0x42, uint32_t timeout = 2000);
    
    /**
     * @brief Inicializa el módulo GPS
     * @return I2CResult código de resultado
     */
    I2CResult initialize();
    
    /**
     * @brief Verifica si el GPS está conectado y responde
     * @return true si está conectado
     */
    bool isConnected();
    
    /**
     * @brief Solicita y lee posición GPS actual
     * @param position Estructura donde almacenar la posición
     * @return I2CResult código de resultado
     */
    I2CResult readPosition(GpsPosition& position);
    
    /**
     * @brief Solicita y lee tiempo GPS actual
     * @param time Estructura donde almacenar el tiempo
     * @return I2CResult código de resultado
     */
    I2CResult readTime(GpsTime& time);
    
    /**
     * @brief Obtiene estado actual del GPS
     * @param status Estructura donde almacenar el estado
     * @return I2CResult código de resultado
     */
    I2CResult getStatus(GpsStatus& status);
    
    /**
     * @brief Configura la frecuencia de actualización del GPS
     * @param frequencyHz Frecuencia en Hz (1, 2, 5, 10, etc.)
     * @return I2CResult código de resultado
     */
    I2CResult setUpdateRate(uint8_t frequencyHz);
    
    /**
     * @brief Obtiene versión del firmware del GPS
     * @param version Buffer para almacenar versión (mín. 32 bytes)
     * @param maxLength Tamaño máximo del buffer
     * @return I2CResult código de resultado
     */
    I2CResult getFirmwareVersion(char* version, uint16_t maxLength);
    
    /**
     * @brief Obtiene última posición válida (sin comunicación I2C)
     * @return Referencia a la última posición
     */
    const GpsPosition& getLastPosition() const { return lastPosition; }
    
    /**
     * @brief Obtiene último tiempo válido (sin comunicación I2C)
     * @return Referencia al último tiempo
     */
    const GpsTime& getLastTime() const { return lastTime; }
    
    /**
     * @brief Obtiene estado actual (sin comunicación I2C)
     * @return Referencia al estado actual
     */
    const GpsStatus& getCurrentStatus() const { return status; }
};

#endif // __cplusplus

// ========== INTERFAZ C ==========

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estructura C para posición GPS
 */
typedef struct {
    double latitude;
    double longitude;
    float altitude;
    uint8_t satellites;
    uint8_t isValid;  // 1 = válido, 0 = inválido
} GpsPosition_C;

/**
 * @brief Estructura C para tiempo GPS
 */
typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t isValid;  // 1 = válido, 0 = inválido
} GpsTime_C;

/**
 * @brief Inicializa el módulo GPS con dirección por defecto
 * @return 0=OK, otros=error
 */
int gpsModuleInit(void);

/**
 * @brief Inicializa el módulo GPS con dirección específica
 * @param addr Dirección I2C del GPS (7-bit)
 * @return 0=OK, otros=error
 */
int gpsModuleInitWithAddr(uint16_t addr);

/**
 * @brief Verifica conexión con GPS
 * @return 1 si conectado, 0 si no
 */
int gpsIsConnected(void);

/**
 * @brief Lee posición GPS actual
 * @param position Puntero a estructura para almacenar posición
 * @return 0=OK, otros=error
 */
int gpsReadPosition(GpsPosition_C* position);

/**
 * @brief Lee tiempo GPS actual
 * @param time Puntero a estructura para almacenar tiempo
 * @return 0=OK, otros=error
 */
int gpsReadTime(GpsTime_C* time);

/**
 * @brief Obtiene última posición válida conocida
 * @param position Puntero a estructura para almacenar posición
 * @return 1 si hay posición válida, 0 si no
 */
int gpsGetLastPosition(GpsPosition_C* position);

#ifdef __cplusplus
}
#endif

#endif // GPS_MODULE_H