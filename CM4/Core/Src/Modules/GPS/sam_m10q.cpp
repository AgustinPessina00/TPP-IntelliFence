/* Includes ------------------------------------------------------------------*/

#include "../../Modules/GPS/sam_m10q.h"
#include "stm32wlxx_hal.h"
#include "I2CManager.h"
#include "../../Modules/UART/UARTManager.h"
#include <stdio.h>
#include <cstring>
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"

// Declaración externa de función para iniciar recepción GPS por interrupción
extern "C" {
    void GPS_StartReception(void);
}


/* Private includes ----------------------------------------------------------*/
void BusyDelayMs(uint32_t ms);

//static uint8_t buffer[NMEA_BUFFER_SIZE];

// Trivial constructor - does NOT access hardware, allocate memory, or block
SamM10q::SamM10q() {
	this->i2cAddr = 0;
    this->i2cBus = nullptr;
    this->uartBus = nullptr;
    this->initialized = false;
}

/* Must be called after HAL_Init() and MX_I2C_Init() */
bool SamM10q::init(uint8_t i2cAddr) {
    if (initialized) {
        return true; // Already initialized
    }
    
    this->i2cAddr = i2cAddr;
    
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Initialize thread-safe I2C bus
    i2cBus = &I2CManager::getBus2();
    if (!i2cBus) {
        return false;
    }
    
    // Initialize thread-safe UART bus
    uartBus = &UARTManager::getUart1();
    if (!uartBus) {
        return false;
    }
    
    // Iniciar recepción por interrupción para GPS (USART1)
    GPS_StartReception();
    
    configure_gps(0, M10Q_NUM_INIT_PSM_DATA_ELEMENTS); // Configuración inicial del GPS (sin PSMOO)
    
    initialized = true;
    return true;
}

HAL_StatusTypeDef SamM10q::read_gps_position() {
    // Intentar obtener datos PVT del GPS
    bool status = update_location_and_time();
    if (status) {
        return HAL_OK;
    } else {
        return HAL_ERROR;
    }
}

/**
 * @brief Actualiza los atributos de ubicación y tiempo de la clase
 * @details Obtiene datos PVT del GPS y actualiza latitude, longitude, fechaUTC y horaUTC
 * @return true si se obtuvieron datos válidos, false en caso contrario
 */
bool SamM10q::update_location_and_time() {
    UBX_NAV_PVT_data_t pvtData;
    
    // Obtener datos PVT del GPS con timeout de 1 segundo
    bool status = getPVT(&pvtData, 1000); 
    if (status == true) {
        // Actualizar latitud y longitud (convertir de deg*1e-7 a grados decimales)
        latitude = pvtData.lat * 1e-7;
        longitude = pvtData.lon * 1e-7;
        
        // Actualizar fecha UTC en formato YYMMDD
        // fechaUTC = (pvtData.year % 100) * 10000 + 
        //            pvtData.month * 100 + 
        //            pvtData.day;
        
        // // Actualizar hora UTC en formato HHMMSS
        // horaUTC = pvtData.hour * 10000 + 
        //           pvtData.min * 100 + 
        //           pvtData.sec;

        return true;
    }
    
    // Si falla, mantener valores anteriores (no los sobrescribimos con 0)
    // Esto evita perder la última posición válida en caso de pérdida temporal de señal
    return false;
}

bool SamM10q::set_new_acq_time(gpsRateSpeed gpsRate) {
    if (gpsRate > gpsRateSpeed::CONTINUOUS && gpsRate < gpsRateSpeed::GREEN_ZONE) {
        return false;
    }

    switch (gpsRate)
    {
    case gpsRateSpeed::GREEN_ZONE:
        configure_all_registers(m10q_new_acq_time_green_zone, 0, M10Q_NUM_RATE_OPTIONS);
        break;
    case gpsRateSpeed::NEAR_LIMIT:
        configure_all_registers(m10q_new_acq_time_near_limit, 0, M10Q_NUM_RATE_OPTIONS);
        break;
    case gpsRateSpeed::CONTINUOUS:
        configure_all_registers(m10q_new_acq_time_continuous, 0, 1);
        break;
    default:
        break;
    }

    return true;
}

/* =========================================================== */
/* ========== CONFIGURACION INICIAL DEL GPS VIA I2C ========== */
/* =========================================================== */

void SamM10q::configure_all_registers(const M10QPayload configPayloads[], size_t startIndex, size_t numPayloads) {
    const uint8_t* payload;
    size_t payload_len;
    for(size_t i = startIndex; i < numPayloads; i++) {
        payload = configPayloads[i].data;
        payload_len = configPayloads[i].size;
        
        bool ramSuccess = false;
        bool bbrSuccess = false;
        bool valgetRamSuccess = false;
        bool valgetBbrSuccess = false;

        // Escribir en RAM
        ramSuccess = write_register(payload, payload_len, RAM);
        valgetRamSuccess = verify_config_with_valget_i2c(payload, payload_len, 0);
        
        // Escribir en BBR (persistente)
        bbrSuccess = write_register(payload, payload_len, BBR);
        valgetBbrSuccess = verify_config_with_valget_i2c(payload, payload_len, 1);
        
        // Si falla alguna de las dos escrituras, reintentar
        if (!ramSuccess || !bbrSuccess || !valgetRamSuccess || !valgetBbrSuccess) {
            i--;
        } else {
            // Si ambas escrituras fueron exitosas, imprimir debug
            RTOS_LOG_DEBUG("[GPS CONFIG] Payload %u configurado correctamente. Verificación ValGet: %s\r\n", i, valgetRamSuccess ? "OK" : "FALLA");
        }
    }
}

void SamM10q::configure_gps(size_t startIndex, size_t numPayloads) {

    bool configSuccessRam = false;
    bool configSuccessBbr = false;

    if (startIndex == 0) {
        write_register_uart(m10q_data_payloads[0].data, m10q_data_payloads[0].size, RAM); // Habilito I2C
        write_register_uart(m10q_data_payloads[0].data, m10q_data_payloads[0].size, BBR);
        startIndex++;

        write_register_uart(m10q_data_payloads[1].data, m10q_data_payloads[1].size, RAM); // Habilita UBX por I2C
        write_register_uart(m10q_data_payloads[1].data, m10q_data_payloads[1].size, BBR);
        startIndex++;
        
        write_register_uart(m10q_data_payloads[2].data, m10q_data_payloads[2].size, RAM); // Desabilita NMEA por I2C
        write_register_uart(m10q_data_payloads[2].data, m10q_data_payloads[2].size, BBR);
        startIndex++;

        configSuccessRam = verify_config_with_valget_i2c(m10q_data_payloads[0].data, m10q_data_payloads[0].size, 0);
        configSuccessBbr = verify_config_with_valget_i2c(m10q_data_payloads[0].data, m10q_data_payloads[0].size, 1);
        if(configSuccessRam && configSuccessBbr)
            RTOS_LOG_DEBUG("[GPS CONFIG] Paso 0 completado: Habilito I2C por UART y verificado\r\n");

        configSuccessRam = verify_config_with_valget_i2c(m10q_data_payloads[1].data, m10q_data_payloads[1].size, 0);
        configSuccessBbr = verify_config_with_valget_i2c(m10q_data_payloads[1].data, m10q_data_payloads[1].size, 1);
        if(configSuccessRam && configSuccessBbr)
            RTOS_LOG_DEBUG("[GPS CONFIG] Paso 1 completado: UBX habilitado por UART y verificado\r\n");

        configSuccessRam = verify_config_with_valget_i2c(m10q_data_payloads[2].data, m10q_data_payloads[2].size, 0);
        configSuccessBbr = verify_config_with_valget_i2c(m10q_data_payloads[2].data, m10q_data_payloads[2].size, 1);
        if(configSuccessRam && configSuccessBbr)
            RTOS_LOG_DEBUG("[GPS CONFIG] Paso 2 completado: NMEA deshabilitado por UART y verificado\r\n");

    }

    // Resto de configuraciones (señales GNSS, power management, etc.) se harán después
    configure_all_registers(m10q_data_payloads, startIndex, numPayloads);
}

/* ================================================================================= */
/* ========== ARMA EL FRAME UBX-VALSET / UBX-VALGET USANDO ARRAY ESTÁTICO ========== */
/* ================================================================================= */
uint16_t SamM10q::build_ubx_message(uint8_t msgClass, uint8_t msgID, uint8_t layer, const uint8_t* payload_data, size_t payload_len, uint8_t* buffer, uint16_t buffer_size) {
    if (!buffer || buffer_size < UBX_MAX_MESSAGE_SIZE) {
        return 0;  // Buffer inválido
    }

    uint16_t idx = 0;

	// 1. Sync chars
    buffer[idx++] = UBX_HEADER1;	// 0xB5
    buffer[idx++] = UBX_HEADER2;	// 0x62

	// 2. Class & ID
    buffer[idx++] = msgClass;
    buffer[idx++] = msgID;

    // 3. Payload length = 4 (version, layer, reserved) + payload size
    uint16_t payloadLength = static_cast<uint16_t>(4 + payload_len);
    buffer[idx++] = static_cast<uint8_t>(payloadLength & 0xFF);			// Little endian LSB
    buffer[idx++] = static_cast<uint8_t>((payloadLength >> 8) & 0xFF);	// Little endian MSB

    // 4. Payload header
    buffer[idx++] = VALSET_VERSION; // 0x00
    buffer[idx++] = layer;   // RAM o BBR

    // 5. Reserved (2 bytes)
    buffer[idx++] = static_cast<uint8_t>(RESERVED & 0xFF);			// LSB
    buffer[idx++] = static_cast<uint8_t>((RESERVED >> 8) & 0xFF);	// MSB

    // 6. Append the actual Key Id (first 4 bytes)
    for (size_t i = 0; i < UBX_KEYID_SIZE; i++) {
        buffer[idx++] = payload_data[i];
    }

    // 7. Append the actual Value if I use VALSET (remaining bytes)
    if(msgID == VALSET_ID) {
        for(size_t i = UBX_KEYID_SIZE; i < payload_len; i++)
        buffer[idx++] = payload_data[i];
    }

    // 8. Calcular el checksum UBX
    uint8_t ck_a = 0, ck_b = 0;
    ubx_calculate_checksum(buffer, idx, &ck_a, &ck_b);
    buffer[idx++] = ck_a;
    buffer[idx++] = ck_b;

	return idx;  // Retorna la longitud total del mensaje
}

void SamM10q::ubx_calculate_checksum(const uint8_t* msg, uint16_t length, uint8_t* ck_a, uint8_t* ck_b) {
    if (!msg || length < 6 || !ck_a || !ck_b) {  // Mínimo: sync(2) + class(1) + id(1) + len(2)
        return;
    }
    
    *ck_a = 0;
    *ck_b = 0;
    
    // Calcular checksum UBX Fletcher sobre Class, ID, Length y Payload
    // Excluir los 2 bytes de sync chars (0xB5 0x62) al inicio
    
    // Class (índice 2)
    *ck_a += msg[2];
    *ck_b += *ck_a;
    
    // ID (índice 3)
    *ck_a += msg[3];
    *ck_b += *ck_a;
    
    // Length LSB (índice 4)
    *ck_a += msg[4];
    *ck_b += *ck_a;
    
    // Length MSB (índice 5)
    *ck_a += msg[5];
    *ck_b += *ck_a;
    
    // Payload (desde índice 6 hasta length-2 para excluir checksum final)
    for (uint16_t i = 6; i < length; i++) {
        *ck_a += msg[i];
        *ck_b += *ck_a;
    }
}

void BusyDelayMs(uint32_t ms) {
    // Usar HAL_Delay que ahora funciona correctamente con TIM2 timebase
    HAL_Delay(ms);
}

/* =================================================================================== */
/* ========== FUNCIONES I2C PARA CONFIGURACIÓN INICIAL LUEGO DE HABILITARLO ========== */
/* =================================================================================== */

/* Escritura de registro usando UBX-CFG-VALSET mediante I2C */
bool SamM10q::write_register(const uint8_t* payload_data, size_t payload_len, uint8_t layer) {
    if (!payload_data || payload_len < UBX_KEYID_SIZE) {
        return false;
    }

    uint8_t message[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET con Checksum incluido..
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, payload_data, payload_len, message, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje
    return send_message(message, msg_len, 15) == HAL_OK;
}


HAL_StatusTypeDef SamM10q::send_message(const uint8_t* message, uint16_t message_length, uint32_t delay_ms) {
    if (!i2cBus || !message || message_length == 0) {
        return HAL_ERROR;
    }

    // Usar método transmit thread-safe del I2CBus para envío directo sin registros
    I2CResult result = i2cBus->memWrite(i2cAddr, 0xFF, 1, message, message_length, 100);
    
    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return (result == I2C_OK) ? HAL_OK : HAL_ERROR;
}

/* =============================================================== */
/* ========== FUNCIONES UART PARA CONFIGURACIÓN INICIAL ========== */
/* =============================================================== */

/* Escritura de registro usando UBX-CFG-VALSET via UART */
bool SamM10q::write_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t layer) {
    if (!payload_data || payload_len == 0) {
        return false;
    }

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET con checksum incluido.
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }
    
    // Enviar mensaje via UART
    return send_message_uart(buffer, msg_len, 15) == HAL_OK;
}

/* Lectura de configuración de un KeyID usando UBX-CFG-VALGET via UART */
bool SamM10q::read_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer) {
    if (!payload_data || payload_len == 0 || !response_buffer || buffer_size == 0) {
        return false;
    }
    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALGET sin checksum (las keys son el payload)
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje de petición via UART
    if (send_message_uart(buffer, msg_len, 1000) != HAL_OK) {
        return false;
    }

    // Leer respuesta del módulo GPS via UART
    if (!uartBus) {
        return false;
    }

    // Usar bus UART thread-safe para leer la respuesta disponible
    // El GPS puede enviar mensajes de longitud variable, por lo que usamos receiveAvailable
    uint16_t bytesReceived = 0;
    UARTResult result = uartBus->receiveAvailable(response_buffer, buffer_size, &bytesReceived, 500);
    
    // Opcional: Log de diagnóstico
    printf("[GPS] Recibidos %u bytes, resultado: %d\r\n", bytesReceived, result);
    
    return (result == UART_OK && bytesReceived > 0);
}

HAL_StatusTypeDef SamM10q::send_message_uart(const uint8_t* message, uint16_t message_length, uint32_t delay_ms) {
    if (!uartBus || !message || message_length == 0) {
        return HAL_ERROR;
    }

    // Usar método transmit thread-safe del UARTBus
    UARTResult result = uartBus->transmit(message, message_length, 100);
    
    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return (result == UART_OK) ? HAL_OK : HAL_ERROR;
}

/* =========================================================================== */
/* ========== FUNCIONES PARA VERIFICACION DE CONFIGURACION CON VALGET ========== */
/* =========================================================================== */

/**
 * @brief Limpia el buffer UART descartando todos los datos residuales
 * @param timeout_ms Tiempo máximo en milisegundos para drenar el buffer (default: 500ms)
 * @details Lee y descarta todos los bytes disponibles en UART hasta que esté vacío o timeout
 */
void SamM10q::flush_uart_buffer(uint32_t timeout_ms) {
    if (!uartBus) {
        return;
    }
    
    uint8_t flush_buffer[128];
    uint16_t bytes_read = 0;
    uint32_t start_time = HAL_GetTick();
    uint8_t empty_reads = 0;
    const uint8_t MAX_EMPTY_READS = 3;
    
    RTOS_LOG_DEBUG("[UART FLUSH] Limpiando buffer UART...\r\n");
    
    while ((HAL_GetTick() - start_time) < timeout_ms) {
        UARTResult result = uartBus->receiveAvailable(flush_buffer, sizeof(flush_buffer), &bytes_read, 50);
        
        if (result == UART_OK && bytes_read > 0) {
            RTOS_LOG_DEBUG("[UART FLUSH] Descartados %u bytes\r\n", bytes_read);
            empty_reads = 0; // Reset contador
        } else {
            empty_reads++;
            if (empty_reads >= MAX_EMPTY_READS) {
                RTOS_LOG_DEBUG("[UART FLUSH] Buffer vacío\r\n");
                break; // Buffer vacío
            }
        }
        
        BusyDelayMs(50); // Esperar entre lecturas
    }
}

/**
 * @brief Verifica que la configuración escrita coincida con lo que devuelve VALGET vía I2C
 * @param payload_data Datos del payload (keyID + value configurado)
 * @param payload_len Longitud del payload
 * @param layer Capa a verificar (RAM o BBR)
 * @return true si el valor coincide, false si no
 */
bool SamM10q::verify_config_with_valget_i2c(const uint8_t* payload_data, size_t payload_len, uint8_t layer) {
    bool configOk = false;
    
    if (!payload_data || payload_len <= UBX_KEYID_SIZE || !i2cBus) {
        return configOk;
    }

    // Extraer keyID (primeros 4 bytes)
    const uint8_t* key_id = payload_data;
    
    // Extraer valor esperado (resto de bytes después del keyID)
    const uint8_t* expected_value = payload_data + UBX_KEYID_SIZE;
    uint8_t value_size = static_cast<uint8_t>(payload_len - UBX_KEYID_SIZE);

    // Construir mensaje VALGET solo con el keyID
    uint8_t message[UBX_MAX_MESSAGE_SIZE];
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, key_id, UBX_KEYID_SIZE, message, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return configOk;
    }

    // Enviar mensaje VALGET vía I2C
    if (send_message(message, msg_len, 200) != HAL_OK) {
        return configOk;
    }

    // Dar tiempo al GPS para procesar antes de empezar a leer
    BusyDelayMs(150);

    // Esperar y leer respuesta del GPS vía I2C
    uint8_t response_buffer[128];
    uint16_t bytes_received = 0;
    uint32_t timeout = 2000; // 2 segundos timeout
    uint32_t start_time = HAL_GetTick();
    uint8_t intentos_sin_datos = 0;
    const uint8_t MAX_INTENTOS_SIN_DATOS = 5; // Salir si no hay datos después de 5 intentos
    bool valget_found = false; // Flag para indicar que se encontró el VALGET
    uint32_t time_after_found = 0; // Tiempo adicional para leer ACK después de encontrar VALGET
    
    while ((HAL_GetTick() - start_time) < timeout) {
        // Verificar disponibilidad de datos en I2C
        uint8_t bytesAvailable[2] = {0, 0};
        I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, I2C_MEMADD_SIZE_8BIT, bytesAvailable, 2, 100);
        
        if (result != I2C_OK) {
            BusyDelayMs(50);
            continue;
        }
        
        uint16_t available = (bytesAvailable[1] << 8) | bytesAvailable[0];
        available &= 0x7FFF; // Limpiar bit 15 (bug conocido del firmware GPS)
        
        if (available > 0) {
            // Leer datos disponibles (limitado al espacio restante en buffer)
            uint16_t space_left = static_cast<uint16_t>(sizeof(response_buffer) - bytes_received);
            uint16_t to_read = (available > space_left) ? space_left : available;
            
            if (to_read > 0) {
                uint8_t chunk_buffer[64]; // Buffer temporal para leer chunks
                uint16_t chunk_size = (to_read > 64) ? 64 : to_read;
                
                result = i2cBus->memRead(i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, chunk_buffer, chunk_size, 100);
                
                if (result == I2C_OK) {
                    // Copiar chunk al buffer principal
                    memcpy(response_buffer + bytes_received, chunk_buffer, chunk_size);
                    bytes_received = static_cast<uint16_t>(bytes_received + chunk_size);
                    intentos_sin_datos = 0; // Reset contador
                    
                    // Intentar parsear la respuesta
                    if (!valget_found && parse_valget_response(response_buffer, bytes_received, key_id, expected_value, value_size)) {
                        valget_found = true;
                        time_after_found = HAL_GetTick();
                        // NO retornar aquí, continuar leyendo para vaciar el ACK
                    }
                    
                    // Si ya encontramos el VALGET y han pasado 300ms leyendo el ACK, validar y salir
                    if (valget_found && (HAL_GetTick() - time_after_found) > 300) {
                        // Validar ACK antes de retornar
                        int8_t ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                        if (ack_result == 1) {
                            configOk = true;
                        } else if (ack_result == 0) {
                            RTOS_LOG_DEBUG("[VALGET I2C DEBUG] ACK-NAK recibido, configuración rechazada!\r\n");
                            configOk = false;
                        } else {
                            // No se encontró ACK, pero VALGET fue exitoso
                            RTOS_LOG_DEBUG("[VALGET I2C DEBUG] Sin ACK, pero VALGET válido\r\n");
                            configOk = true;
                        }
                        break;
                    }
                }
            }
        } else {
            intentos_sin_datos++;
            
            // Si ya encontramos el VALGET y no hay más datos, validar ACK y salir
            if (valget_found && intentos_sin_datos >= 3) {
                int8_t ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                if (ack_result == 1) {
                    configOk = true;
                } else if (ack_result == 0) {
                    RTOS_LOG_DEBUG("[VALGET I2C DEBUG] ACK-NAK - configuración rechazada\r\n");
                    configOk = false;
                } else {
                    RTOS_LOG_DEBUG("[VALGET I2C DEBUG] Sin ACK pero VALGET válido\r\n");
                    configOk = true;
                }
                break;
            }
            
            if (intentos_sin_datos >= MAX_INTENTOS_SIN_DATOS && bytes_received > 0) {
                // Ya tenemos datos y no llegan más, intentar parsear lo que tengamos
                RTOS_LOG_DEBUG("[VALGET I2C DEBUG] No llegan más datos, parseando buffer final\r\n");
                if (parse_valget_response(response_buffer, bytes_received, key_id, expected_value, value_size)) {
                    // Encontrado al final, intentar leer ACK restante
                    RTOS_LOG_DEBUG("[VALGET I2C DEBUG] VALGET encontrado al final, leyendo ACK residual...\r\n");
                    BusyDelayMs(100);
                    
                    // Verificar si hay más datos disponibles
                    uint8_t ack_avail[2] = {0, 0};
                    i2cBus->memRead(i2cAddr, 0xFD, I2C_MEMADD_SIZE_8BIT, ack_avail, 2, 100);
                    uint16_t ack_available = ((ack_avail[1] << 8) | ack_avail[0]) & 0x7FFF;
                    
                    if (ack_available > 0) {
                        uint8_t ack_buffer[32];
                        uint16_t ack_read = (ack_available > 32) ? 32 : ack_available;
                        i2cBus->memRead(i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, ack_buffer, ack_read, 100);
                        
                        // Validar ACK en el buffer adicional
                        int8_t ack_result = check_ack_response(ack_buffer, ack_read, VALGET_CLASS, VALGET_ID);
                        if (ack_result == 1) {
                            configOk = true;
                        } else if (ack_result == 0) {
                            RTOS_LOG_DEBUG("[VALGET I2C DEBUG] ACK-NAK - configuración rechazada\r\n");
                            configOk = false;
                        } else {
                            // Intentar buscar en el buffer principal
                            ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                            if (ack_result == 0) {
                                RTOS_LOG_DEBUG("[VALGET I2C DEBUG] ACK-NAK en buffer principal\r\n");
                                configOk = false;
                            } else {
                                RTOS_LOG_DEBUG("[VALGET I2C DEBUG] Sin ACK claro, asumiendo éxito por VALGET válido\r\n");
                                configOk = true;
                            }
                        }
                    } else {
                        // No hay más datos, validar en buffer principal
                        int8_t ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                        if (ack_result == 0) {
                            RTOS_LOG_DEBUG("[VALGET I2C DEBUG] ACK-NAK en buffer principal\r\n");
                            configOk = false;
                        } else {
                            RTOS_LOG_DEBUG("[VALGET I2C DEBUG] Sin ACK claro, asumiendo éxito por VALGET válido\r\n");
                            configOk = true;
                        }
                    }
                }
                break; // Salir del bucle
            }
        }
        
        BusyDelayMs(50); // Delay entre intentos de lectura
    }
    
    if (!configOk) {
        printf("[VALGET I2C DEBUG] Timeout - Total recibido: %u bytes\r\n", bytes_received);
    }
    
    return configOk;
}

/**
 * @brief Verifica que la configuración escrita coincida con lo que devuelve VALGET vía UART
 * @param payload_data Datos del payload (keyID + value configurado)
 * @param payload_len Longitud del payload
 * @param layer Capa a verificar (RAM o BBR)
 * @return true si el valor coincide, false si no
 */
bool SamM10q::verify_config_with_valget_uart(const uint8_t* payload_data, size_t payload_len, uint8_t layer) {
    bool configOk = false;
    
    if (!payload_data || payload_len <= UBX_KEYID_SIZE || !uartBus) {
        return configOk;
    }

    // Extraer keyID (primeros 4 bytes)
    const uint8_t* key_id = payload_data;
    
    // Extraer valor esperado (resto de bytes después del keyID)
    const uint8_t* expected_value = payload_data + UBX_KEYID_SIZE;
    uint8_t value_size = static_cast<uint8_t>(payload_len - UBX_KEYID_SIZE);

    // Construir mensaje VALGET solo con el keyID
    uint8_t message[UBX_MAX_MESSAGE_SIZE];
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, key_id, UBX_KEYID_SIZE, message, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return configOk;
    }

    // Enviar mensaje VALGET vía UART
    if (send_message_uart(message, msg_len, 200) != HAL_OK) {
        return configOk;
    }

    // Dar tiempo al GPS para procesar antes de empezar a leer
    BusyDelayMs(150);

    // Esperar y leer respuesta del GPS
    uint8_t response_buffer[128];
    uint16_t bytes_received = 0;
    uint32_t timeout = 2000; // 2 segundos timeout
    uint32_t start_time = HAL_GetTick();
    uint8_t intentos_sin_datos = 0;
    const uint8_t MAX_INTENTOS_SIN_DATOS = 5; // Salir si no hay datos después de 5 intentos
    bool valget_found = false; // Flag para indicar que se encontró el VALGET
    uint32_t time_after_found = 0; // Tiempo adicional para leer ACK después de encontrar VALGET
    
    while ((HAL_GetTick() - start_time) < timeout) {
        uint16_t chunk_received = 0;
        UARTResult result = uartBus->receiveAvailable(
            response_buffer + bytes_received,
            static_cast<uint16_t>(sizeof(response_buffer) - bytes_received),
            &chunk_received,
            100
        );
        
        if (result == UART_OK && chunk_received > 0) {
            intentos_sin_datos = 0; // Reset contador
            
            bytes_received = static_cast<uint16_t>(bytes_received + chunk_received);
            
            // Intentar parsear la respuesta
            if (!valget_found && parse_valget_response(response_buffer, bytes_received, key_id, expected_value, value_size)) {
                RTOS_LOG_DEBUG("[VALGET DEBUG] Parseado exitoso! Continuando lectura para vaciar ACK...\r\n");
                valget_found = true;
                time_after_found = HAL_GetTick();
                // NO retornar aquí, continuar leyendo para vaciar el ACK
            }
            
            // Si ya encontramos el VALGET y han pasado 300ms leyendo el ACK, validar y salir
            if (valget_found && (HAL_GetTick() - time_after_found) > 300) {
                // Validar ACK antes de retornar
                int8_t ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                if (ack_result == 1) {
                    RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-ACK confirmado, configuración aceptada\r\n");
                    configOk = true;
                } else if (ack_result == 0) {
                    RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-NAK recibido, configuración rechazada!\r\n");
                    configOk = false;
                } else {
                    // No se encontró ACK, pero VALGET fue exitoso
                    RTOS_LOG_DEBUG("[VALGET DEBUG] Sin ACK, pero VALGET válido\r\n");
                    configOk = true;
                }
                break;
            }
        } else {
            intentos_sin_datos++;
            
            // Si ya encontramos el VALGET y no hay más datos, validar ACK y salir
            if (valget_found && intentos_sin_datos >= 3) {
                int8_t ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                if (ack_result == 1) {
                    RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-ACK confirmado\r\n");
                    configOk = true;
                } else if (ack_result == 0) {
                    RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-NAK - configuración rechazada\r\n");
                    configOk = false;
                } else {
                    RTOS_LOG_DEBUG("[VALGET DEBUG] Sin ACK pero VALGET válido\r\n");
                    configOk = true;
                }
                break;
            }
            
            if (intentos_sin_datos >= MAX_INTENTOS_SIN_DATOS && bytes_received > 0) {
                // Ya tenemos datos y no llegan más, intentar parsear lo que tengamos
                RTOS_LOG_DEBUG("[VALGET DEBUG] No llegan más datos, parseando buffer final\r\n");
                if (parse_valget_response(response_buffer, bytes_received, key_id, expected_value, value_size)) {
                    // Encontrado al final, intentar leer ACK restante
                    RTOS_LOG_DEBUG("[VALGET DEBUG] VALGET encontrado al final, leyendo ACK residual...\r\n");
                    BusyDelayMs(100);
                    uint8_t ack_buffer[32];
                    uint16_t ack_bytes = 0;
                    uartBus->receiveAvailable(ack_buffer, sizeof(ack_buffer), &ack_bytes, 50);
                    
                    // Validar ACK en el buffer adicional
                    int8_t ack_result = check_ack_response(ack_buffer, ack_bytes, VALGET_CLASS, VALGET_ID);
                    if (ack_result == 1) {
                        RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-ACK confirmado\r\n");
                        configOk = true;
                    } else if (ack_result == 0) {
                        RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-NAK - configuración rechazada\r\n");
                        configOk = false;
                    } else {
                        // Intentar buscar en el buffer principal
                        ack_result = check_ack_response(response_buffer, bytes_received, VALGET_CLASS, VALGET_ID);
                        if (ack_result == 0) {
                            RTOS_LOG_DEBUG("[VALGET DEBUG] ACK-NAK en buffer principal\r\n");
                            configOk = false;
                        } else {
                            RTOS_LOG_DEBUG("[VALGET DEBUG] Sin ACK claro, asumiendo éxito por VALGET válido\r\n");
                            configOk = true;
                        }
                    }
                }
                break; // Salir del bucle
            }
        }
        
        BusyDelayMs(50); // Delay más largo entre intentos de lectura
    }
    
    // Imprimir buffer completo al final
    if (bytes_received > 0) {
        RTOS_LOG_DEBUG("[VALGET DEBUG] Buffer completo recibido (%u bytes): ", bytes_received);
        for (uint16_t i = 0; i < bytes_received; i++) {
            RTOS_LOG_DEBUG("%02X ", response_buffer[i]);
        }
        RTOS_LOG_DEBUG("\r\n");
    }
    
    if (!configOk) {
        printf("[VALGET DEBUG] Timeout - Total recibido: %u bytes\r\n", bytes_received);
    }
    
    return configOk;
}

/**
 * @brief Parsea la respuesta VALGET y compara con el valor esperado
 * @param response_buffer Buffer con la respuesta UART
 * @param buffer_len Longitud del buffer
 * @param key_id KeyID a buscar (4 bytes)
 * @param expected_value Valor esperado
 * @param value_size Tamaño del valor
 * @return true si encuentra el mensaje y el valor coincide, false si no
 */
bool SamM10q::parse_valget_response(const uint8_t* response_buffer, uint16_t buffer_len, 
                                     const uint8_t* key_id, const uint8_t* expected_value, 
                                     uint8_t value_size) {
    if (!response_buffer || buffer_len < 8 || !key_id || !expected_value || value_size == 0) {
        return false;
    }

    // Buscar el inicio del mensaje UBX-VALGET en el buffer
    for (uint16_t i = 0; i + 8 < buffer_len; i++) {
        // Buscar sincronización UBX
        if (response_buffer[i] != UBX_HEADER1 || response_buffer[i + 1] != UBX_HEADER2) {
            continue;
        }
        
        // Verificar que es un mensaje VALGET
        if (response_buffer[i + 2] != VALGET_CLASS || response_buffer[i + 3] != VALGET_ID) {
            continue;
        }
        
        // Leer longitud del payload
        uint16_t payload_len = response_buffer[i + 4] | (response_buffer[i + 5] << 8);
        uint16_t total_msg_len = static_cast<uint16_t>(6 + payload_len + 2); // header(2) + class(1) + id(1) + len(2) + payload + checksum(2)
        
        // Verificar que tenemos el mensaje completo
        if (i + total_msg_len > buffer_len) {
            continue;
        }
        
        // Verificar checksum del mensaje
        if (!verifyUBXChecksum(&response_buffer[i], total_msg_len)) {
            continue;
        }
        
        // Payload comienza en i+6
        // Estructura del payload VALGET: version(1) + layer(1) + reserved(2) + keyID(4) + value(N)
        const uint8_t* payload = &response_buffer[i + 6];
        
        // Verificar que el payload tiene suficiente tamaño
        if (payload_len < (4 + UBX_KEYID_SIZE + value_size)) {
            continue;
        }
        
        // Saltar header del payload (version + layer + reserved = 4 bytes)
        const uint8_t* payload_keyid = payload + 4;
        const uint8_t* payload_value = payload + 4 + UBX_KEYID_SIZE;
        
        // Verificar que el keyID coincide
        if (memcmp(payload_keyid, key_id, UBX_KEYID_SIZE) != 0) {
            continue;
        }
        
        // Comparar el valor recibido con el esperado
        if (memcmp(payload_value, expected_value, value_size) == 0) {
            return true; // Coincide!
        } else {
            return false; // KeyID correcto pero valor diferente
        }
    }
    
    return false; // No se encontró el mensaje VALGET con este keyID
}

/**
 * @brief Busca y valida mensaje ACK-ACK o ACK-NAK en el buffer
 * @param response_buffer Buffer con respuestas UBX
 * @param buffer_len Longitud del buffer
 * @param expected_class Class del mensaje que esperamos confirmar (ej: 0x06 para VALGET)
 * @param expected_id ID del mensaje que esperamos confirmar (ej: 0x8B para VALGET)
 * @return 1 = ACK-ACK encontrado, 0 = ACK-NAK encontrado, -1 = No encontrado
 */
int8_t SamM10q::check_ack_response(const uint8_t* response_buffer, uint16_t buffer_len, 
                                    uint8_t expected_class, uint8_t expected_id) {
    if (!response_buffer || buffer_len < 10) {
        return -1; // Buffer muy pequeño para contener un ACK
    }

    // Buscar ACK en el buffer
    for (uint16_t i = 0; i + 10 <= buffer_len; i++) {
        // Buscar sincronización UBX
        if (response_buffer[i] != UBX_HEADER1 || response_buffer[i + 1] != UBX_HEADER2) {
            continue;
        }
        
        // Verificar que es un mensaje ACK (Class = 0x05)
        if (response_buffer[i + 2] != ACK_CLASS) {
            continue;
        }
        
        // Verificar ID: ACK-ACK (0x01) o ACK-NAK (0x00)
        uint8_t ack_id = response_buffer[i + 3];
        if (ack_id != ACK_ACK_ID && ack_id != ACK_NAK_ID) {
            continue;
        }
        
        // Verificar longitud (debe ser 2 bytes)
        uint16_t payload_len = response_buffer[i + 4] | (response_buffer[i + 5] << 8);
        if (payload_len != 2) {
            continue;
        }
        
        // Verificar checksum
        if (!verifyUBXChecksum(&response_buffer[i], 10)) {
            continue;
        }
        
        // Verificar que el ACK es para nuestro mensaje (Class + ID en payload)
        uint8_t ack_class = response_buffer[i + 6];
        uint8_t ack_msg_id = response_buffer[i + 7];
        
        if (ack_class == expected_class && ack_msg_id == expected_id) {
            // Encontrado! Retornar tipo de ACK
            if (ack_id == ACK_ACK_ID) {
                return 1; // ACK-ACK
            } else {
                RTOS_LOG_DEBUG("[VALGET ACK] ACK-NAK recibido para Class=0x%02X ID=0x%02X\r\n", 
                               expected_class, expected_id);
                return 0; // ACK-NAK
            }
        }
    }
    
    return -1; // No encontrado
}

/* =========================================================== */
/* ============== FUNCIONES PARA OBTENER PVT ================= */
/* =========================================================== */

/**
 * @brief Función principal para obtener datos PVT (Position, Velocity, Time)
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @param maxWaitMs Tiempo máximo de espera en milisegundos (default: 1000ms)
 * @return true si se obtuvieron datos válidos, false en caso contrario
 */
bool SamM10q::getPVT(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs) {
    if (pvtData == nullptr) {
        return false;
    }

    // Limpiar la estructura antes de usarla
    memset(pvtData, 0, sizeof(UBX_NAV_PVT_data_t));

    // 1. Solicitar mensaje PVT al GPS
    bool status = requestPVT();
    if (!status) {
        this->psmStateActive = false; // Si no pudimos obtener datos, asumimos que el GPS está en PSM INACTIVE (no responde)
        return false;
    }

    //HAL_Delay(1500);

    status = receivePVT(pvtData, maxWaitMs);
    // 2. Esperar y recibir la respuesta
    if (!status) {
        return false;
    }

    this->flags = pvtData->flags; // Guardar flags para diagnóstico o uso futuro
    this->iTow = pvtData->iTOW;   // Guardar iTOW para diagnóstico o uso futuro

    // 3. Verificar que tenemos un fix válido
    // fixType: 0=no fix, 2=2D fix, 3=3D fix
    if (pvtData->fixType < 3) {
        this->psmStateActive = true; // Si pudimos obtener datos, el GPS no está en PSM INACTIVE
        return false; // No hay fix válido
    }

    this->psmStateActive = true; // Si pudimos obtener datos, el GPS no está en PSM INACTIVE

    return true;
}

/**
 * @brief Solicita datos PVT al módulo GPS (polling message)
 * @return true si el mensaje se envió correctamente
 */
bool SamM10q::requestPVT() {
    uint8_t message[8]; // Header(2) + Class(1) + ID(1) + Length(2) + Checksum(2)
    
    // Construir mensaje UBX-NAV-PVT (sin payload, es un polling message)
    message[0] = UBX_HEADER1;  // 0xB5
    message[1] = UBX_HEADER2;  // 0x62
    message[2] = NAV_CLASS;    // 0x01
    message[3] = PVT_ID;       // 0x07
    message[4] = 0x00;         // Length LSB (0 bytes de payload)
    message[5] = 0x00;         // Length MSB
    
    // Calcular checksum sobre el mensaje completo (sin checksum final)
    uint8_t ck_a, ck_b;
    ubx_calculate_checksum(message, 6, &ck_a, &ck_b);
    message[6] = ck_a;
    message[7] = ck_b;
    
    // Enviar mensaje por I2C
    HAL_StatusTypeDef status = send_message(message, 8, 10);
    
    return (status == HAL_OK);
}

/**
 * @brief Recibe y parsea la respuesta PVT del GPS
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @param maxWaitMs Tiempo máximo de espera en milisegundos
 * @return true si se recibió y parseó correctamente
 */
bool SamM10q::receivePVT(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs) {
    // El mensaje UBX-NAV-PVT completo tiene:
    // Header(2) + Class(1) + ID(1) + Length(2) + Payload(92) + Checksum(2) = 100 bytes
    const uint16_t PVT_MESSAGE_SIZE = 100;
    uint8_t buffer[PVT_MESSAGE_SIZE];
    
    uint32_t startTime = HAL_GetTick();
    uint16_t bytesRead = 0;
    
    // Polling: intentar leer hasta que lleguen datos o timeout
    while ((HAL_GetTick() - startTime) < maxWaitMs) {
        // Leer 2 bytes para verificar disponibilidad de datos
        uint8_t bytesAvailable[2] = {10, 10};
        I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, I2C_MEMADD_SIZE_8BIT, bytesAvailable, 2, 100);
        
        if (result != I2C_OK) {
            BusyDelayMs(10);
            continue;
        }
        
        // Los bytes disponibles están en formato little-endian
        uint16_t available = (bytesAvailable[1] << 8) | bytesAvailable[0];
        available &= 0x7FFF; // Limpiar bit 15 (bug conocido del firmware GPS)
        
        // Si hay suficientes bytes disponibles, leer el mensaje completo en chunks de 64 Bytes.
        if (available >= PVT_MESSAGE_SIZE) {
            const uint8_t CHUNK_SIZE = 128;
            uint16_t totalRead = 0;
            bool readSuccess = true;
            
            while (totalRead < PVT_MESSAGE_SIZE) {
                uint8_t toRead = (PVT_MESSAGE_SIZE - totalRead) > CHUNK_SIZE ? CHUNK_SIZE : (PVT_MESSAGE_SIZE - totalRead);
                result = i2cBus->memRead(i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, &buffer[totalRead], toRead, 100);
                
                if (result != I2C_OK) {
                    readSuccess = false;
                    break;
                }
                totalRead += toRead;
            }
            
            if (readSuccess) {
                bytesRead = PVT_MESSAGE_SIZE;
                break;
            }
        }
        
        BusyDelayMs(1); // Esperar 1ms antes de reintentar (optimizado)
    }
    
    if (bytesRead == 0) {
        return false; // Timeout sin recibir datos
    }
    
    // Parsear el mensaje recibido
    return parseUBXMessage(buffer, bytesRead, pvtData);
}

/**
 * @brief Parsea un mensaje UBX y extrae los datos PVT
 * @param buffer Buffer con el mensaje UBX completo
 * @param bufferLen Longitud del buffer
 * @param pvtData Puntero a estructura donde se almacenarán los datos
 * @return true si el mensaje es válido y se parseó correctamente
 */
bool SamM10q::parseUBXMessage(const uint8_t* buffer, uint16_t bufferLen, UBX_NAV_PVT_data_t* pvtData) {
    // Verificar que el buffer tiene el tamaño mínimo
    if (bufferLen < 100) {
        return false;
    }
    
    // Verificar header UBX
    if (buffer[0] != UBX_HEADER1 || buffer[1] != UBX_HEADER2) {
        return false;
    }
    
    // Verificar que es un mensaje NAV-PVT
    if (buffer[2] != NAV_CLASS || buffer[3] != PVT_ID) {
        return false;
    }
    
    // Verificar longitud del payload (debe ser 92 bytes)
    uint16_t payloadLen = buffer[4] | (buffer[5] << 8);
    if (payloadLen != 92) {
        return false;
    }
    
    // Verificar checksum
    if (!verifyUBXChecksum(buffer, bufferLen)) {
        return false;
    }
    
    // Parsear payload (comienza en buffer[6])
    const uint8_t* payload = &buffer[6];
    uint16_t offset = 0;
    
    // Extraer datos según la especificación UBX-NAV-PVT
    // Nota: Los datos están en formato little-endian
    
    pvtData->iTOW = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->year = payload[offset] | (payload[offset+1] << 8);
    offset += 2;
    
    pvtData->month = payload[offset++];
    pvtData->day = payload[offset++];
    pvtData->hour = payload[offset++];
    pvtData->min = payload[offset++];
    pvtData->sec = payload[offset++];
    pvtData->valid = payload[offset++];
    
    pvtData->tAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->nano = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->fixType = payload[offset++];
    pvtData->flags = payload[offset++];
    pvtData->flags2 = payload[offset++];
    pvtData->numSV = payload[offset++];
    
    pvtData->lon = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                             (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->lat = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                             (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->height = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->hMSL = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->hAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->vAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->velN = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->velE = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->velD = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                              (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->gSpeed = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->headMot = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                 (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->sAcc = payload[offset] | (payload[offset+1] << 8) | 
                    (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->headAcc = payload[offset] | (payload[offset+1] << 8) | 
                       (payload[offset+2] << 16) | (payload[offset+3] << 24);
    offset += 4;
    
    pvtData->pDOP = payload[offset] | (payload[offset+1] << 8);
    offset += 2;
    
    pvtData->flags3 = payload[offset] | (payload[offset+1] << 8);
    offset += 2;
    
    // Saltar reserved[4]
    offset += 4;
    
    pvtData->headVeh = (int32_t)(payload[offset] | (payload[offset+1] << 8) | 
                                 (payload[offset+2] << 16) | (payload[offset+3] << 24));
    offset += 4;
    
    pvtData->magDec = (int16_t)(payload[offset] | (payload[offset+1] << 8));
    offset += 2;
    
    pvtData->magAcc = payload[offset] | (payload[offset+1] << 8);
    
    return true;
}

/**
 * @brief Verifica el checksum de un mensaje UBX
 * @param buffer Buffer con el mensaje UBX completo
 * @param msgLen Longitud del mensaje
 * @return true si el checksum es correcto
 */
bool SamM10q::verifyUBXChecksum(const uint8_t* buffer, uint16_t msgLen) {
    if (msgLen < 8) {
        return false; // Mensaje muy corto
    }
    
    // Calcular checksum sobre Class + ID + Length + Payload
    uint8_t ck_a_calc, ck_b_calc;
    ubx_calculate_checksum(buffer, msgLen - 2, &ck_a_calc, &ck_b_calc);
    
    // Comparar con el checksum recibido (últimos 2 bytes)
    uint8_t ck_a_recv = buffer[msgLen - 2];
    uint8_t ck_b_recv = buffer[msgLen - 1];
    
    return (ck_a_calc == ck_a_recv) && (ck_b_calc == ck_b_recv);
}
