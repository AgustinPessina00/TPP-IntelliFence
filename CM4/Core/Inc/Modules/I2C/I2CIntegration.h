#ifndef I2C_INTEGRATION_H
#define I2C_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Archivo de integración para usar I2C desde C
 * @details Proporciona una interfaz simple para integrar la arquitectura
 *          I2C estática con el código principal en C
 */

/**
 * @brief Inicializa el sistema I2C completo
 * @details Debe llamarse UNA sola vez en main() después de que 
 *          el hardware I2C esté inicializado
 * @return 1 si exitoso, 0 si falló
 */
int initializeI2cArchitecture(void);

/**
 * @brief Ejecuta ejemplos de demostración de la arquitectura I2C
 * @details Crea tasks FreeRTOS que demuestran el uso thread-safe
 */
void startI2cDemonstrations(void);

/**
 * @brief Verifica el estado del sistema I2C
 * @return 1 si está listo y funcional, 0 si hay problemas
 */
int checkI2cSystemHealth(void);

/**
 * @brief Función simple para leer un registro de dispositivo I2C
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro
 * @param value Puntero donde almacenar el valor leído
 * @return 1 si exitoso, 0 si falló
 * 
 * Ejemplo de uso:
 * @code
 * uint8_t whoAmI;
 * if (simpleI2cReadByte(0x68, 0x75, &whoAmI)) {
 *     printf("MPU6050 WHO_AM_I: 0x%02X\r\n", whoAmI);
 * }
 * @endcode
 */
int simpleI2cReadByte(uint8_t deviceAddr, uint8_t regAddr, uint8_t* value);

/**
 * @brief Función simple para escribir un registro de dispositivo I2C
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro
 * @param value Valor a escribir
 * @return 1 si exitoso, 0 si falló
 * 
 * Ejemplo de uso:
 * @code
 * if (simpleI2cWriteByte(0x68, 0x6B, 0x00)) {
 *     printf("MPU6050 despertado\r\n");
 * }
 * @endcode
 */
int simpleI2cWriteByte(uint8_t deviceAddr, uint8_t regAddr, uint8_t value);

/**
 * @brief Lee múltiples bytes de un dispositivo I2C
 * @param deviceAddr Dirección del dispositivo (7-bit)
 * @param regAddr Dirección del registro inicial
 * @param buffer Buffer donde almacenar los datos
 * @param length Cantidad de bytes a leer
 * @return 1 si exitoso, 0 si falló
 * 
 * Ejemplo de uso:
 * @code
 * uint8_t accelData[6];
 * if (simpleI2cReadBytes(0x68, 0x3B, accelData, 6)) {
 *     // Procesar datos del acelerómetro
 * }
 * @endcode
 */
int simpleI2cReadBytes(uint8_t deviceAddr, uint8_t regAddr, 
                      uint8_t* buffer, uint8_t length);

/**
 * @brief Obtiene estadísticas básicas del sistema I2C
 * @param totalOps Puntero para total de operaciones
 * @param successfulOps Puntero para operaciones exitosas  
 * @param errorCount Puntero para contador de errores
 */
void getI2cStatistics(uint32_t* totalOps, uint32_t* successfulOps, 
                     uint32_t* errorCount);

/**
 * @brief Imprime estadísticas del sistema I2C por UART
 */
void printI2cStatistics(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_INTEGRATION_H