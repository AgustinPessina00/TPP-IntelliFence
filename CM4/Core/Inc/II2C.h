#ifndef II2C_H
#define II2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
}
#endif

/**
 * @brief Códigos de retorno para operaciones I2C
 */
typedef enum {
    I2C_OK = 0,
    I2C_ERROR = 1,
    I2C_TIMEOUT = 2,
    I2C_BUSY = 3,
    I2C_NACK = 4
} I2CResult;

#ifdef __cplusplus

/**
 * @brief Interfaz abstracta para comunicación I2C
 * 
 * Esta interfaz define los métodos primitivos necesarios para comunicación I2C:
 * - memRead: Lectura de registros/memoria de dispositivos
 * - memWrite: Escritura a registros/memoria de dispositivos  
 * - writeRead: Escritura seguida de lectura (para dispositivos sin registros)
 */
class II2C {
public:
    virtual ~II2C() = default;

    /**
     * @brief Lee datos de la memoria/registro de un dispositivo I2C
     * @param deviceAddr Dirección 7-bit del dispositivo
     * @param memAddr Dirección del registro/memoria a leer
     * @param memAddrSize Tamaño de la dirección de memoria (1 o 2 bytes)
     * @param pData Buffer donde almacenar los datos leídos
     * @param size Cantidad de bytes a leer
     * @param timeout Timeout en milisegundos
     * @return I2CResult código de resultado
     */
    virtual I2CResult memRead(uint16_t deviceAddr, 
                             uint16_t memAddr, 
                             uint16_t memAddrSize,
                             uint8_t* pData, 
                             uint16_t size, 
                             uint32_t timeout) = 0;

    /**
     * @brief Escribe datos a la memoria/registro de un dispositivo I2C
     * @param deviceAddr Dirección 7-bit del dispositivo
     * @param memAddr Dirección del registro/memoria a escribir
     * @param memAddrSize Tamaño de la dirección de memoria (1 o 2 bytes)
     * @param pData Buffer con los datos a escribir
     * @param size Cantidad de bytes a escribir
     * @param timeout Timeout en milisegundos
     * @return I2CResult código de resultado
     */
    virtual I2CResult memWrite(uint16_t deviceAddr, 
                              uint16_t memAddr, 
                              uint16_t memAddrSize,
                              const uint8_t* pData, 
                              uint16_t size, 
                              uint32_t timeout) = 0;

    /**
     * @brief Realiza una escritura seguida de lectura sin STOP intermedio
     * 
     * Útil para dispositivos que no tienen registros y requieren un comando
     * específico antes de la lectura (como algunos sensores GPS o RTC)
     * 
     * @param deviceAddr Dirección 7-bit del dispositivo
     * @param pWriteData Buffer con los datos a escribir (comando)
     * @param writeSize Cantidad de bytes a escribir
     * @param pReadData Buffer donde almacenar los datos leídos
     * @param readSize Cantidad de bytes a leer
     * @param timeout Timeout en milisegundos
     * @return I2CResult código de resultado
     */
    virtual I2CResult writeRead(uint16_t deviceAddr, 
                               const uint8_t* pWriteData, 
                               uint16_t writeSize,
                               uint8_t* pReadData, 
                               uint16_t readSize, 
                               uint32_t timeout) = 0;
};

#endif // __cplusplus

#endif // II2C_H