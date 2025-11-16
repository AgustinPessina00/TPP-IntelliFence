#ifndef IUART_H
#define IUART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
}
#endif

/**
 * @brief Códigos de retorno para operaciones UART
 */
typedef enum {
    UART_OK = 0,
    UART_ERROR = 1,
    UART_TIMEOUT = 2,
    UART_BUSY = 3,
    UART_NACK = 4
} UARTResult;

#ifdef __cplusplus

/**
 * @brief Interfaz abstracta para comunicación UART
 * 
 * Esta interfaz define los métodos primitivos necesarios para comunicación UART:
 * - memRead: Lectura de registros/memoria de dispositivos
 * - memWrite: Escritura a registros/memoria de dispositivos  
 * - writeRead: Escritura seguida de lectura (para dispositivos sin registros)
 */
class IUART {
public:
    virtual ~IUART() = default;

    /**
     * @brief Lee datos de la memoria/registro de un dispositivo UART
     * @param deviceAddr Dirección del dispositivo
     * @param memAddr Dirección del registro/memoria a leer
     * @param memAddrSize Tamaño de la dirección de memoria (1 o 2 bytes)
     * @param pData Buffer donde almacenar los datos leídos
     * @param size Cantidad de bytes a leer
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    virtual UARTResult memRead(uint16_t deviceAddr, 
                              uint16_t memAddr, 
                              uint16_t memAddrSize,
                              uint8_t* pData, 
                              uint16_t size, 
                              uint32_t timeout) = 0;

    /**
     * @brief Escribe datos a la memoria/registro de un dispositivo UART
     * @param deviceAddr Dirección del dispositivo
     * @param memAddr Dirección del registro/memoria a escribir
     * @param memAddrSize Tamaño de la dirección de memoria (1 o 2 bytes)
     * @param pData Buffer con los datos a escribir
     * @param size Cantidad de bytes a escribir
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    virtual UARTResult memWrite(uint16_t deviceAddr, 
                               uint16_t memAddr, 
                               uint16_t memAddrSize,
                               const uint8_t* pData, 
                               uint16_t size, 
                               uint32_t timeout) = 0;

    /**
     * @brief Realiza una escritura seguida de lectura
     * 
     * Útil para dispositivos que requieren un comando específico antes de
     * la lectura (como algunos sensores GPS o módulos de comunicación serial)
     * 
     * @param deviceAddr Dirección del dispositivo
     * @param pWriteData Buffer con los datos a escribir (comando)
     * @param writeSize Cantidad de bytes a escribir
     * @param pReadData Buffer donde almacenar los datos leídos
     * @param readSize Cantidad de bytes a leer
     * @param timeout Timeout en milisegundos
     * @return UARTResult código de resultado
     */
    virtual UARTResult writeRead(uint16_t deviceAddr, 
                                const uint8_t* pWriteData, 
                                uint16_t writeSize,
                                uint8_t* pReadData, 
                                uint16_t readSize, 
                                uint32_t timeout) = 0;
};

#endif // __cplusplus

#endif // IUART_H
