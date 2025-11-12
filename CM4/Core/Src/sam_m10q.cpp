/* Includes ------------------------------------------------------------------*/

#include "../../Modules/GPS/sam_m10q.h"
#include "stm32wlxx_hal.h"
#include "I2CManager.h"
#include "../../Modules/UART/UARTManager.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
void BusyDelayMs(uint32_t ms);

//static uint8_t buffer[NMEA_BUFFER_SIZE];

SamM10q::SamM10q(uint8_t i2cAddr) {
	this->i2cAddr   = i2cAddr;
    this->i2cBus    = nullptr;   // Se inicializa en initSamM10q()
    this->uartBus   = nullptr;  // Se inicializa en initSamM10q()
	this->version	= VALSET_VERSION;
    this->reserved	= RESERVED;
}

/* =========================================================== */
/* ========== LLAMAR LUEGO DE INICIALIZAR HAL E I2C ========== */
/* =========================================================== */
void SamM10q::initSamM10q() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Inicializar el bus I2C thread-safe
    i2cBus = &I2CManager::getBus2();
    
    // Inicializar el bus UART thread-safe
    uartBus = &UARTManager::getInstance().getUART1();
    
    configure_gps();
}

/*
HAL_StatusTypeDef SamM10q::read_nmea_stream() {
    uint8_t buffer[NMEA_BUFFER_SIZE];
    
    if (!i2cBus) {
        return HAL_ERROR;
    }

    // Usar bus I2C thread-safe en lugar de HAL directo
    I2CResult result = i2cBus->memRead(i2cAddr, 0xFF, 1, buffer, NMEA_BUFFER_SIZE, 10);
    
    if (result != I2C_OK) {
        // Podés agregar manejo de error acá si querés
        return HAL_ERROR;
    }

    for (uint8_t i = 0; i < NMEA_BUFFER_SIZE; ++i) {
        trackerGPS.encode(buffer[i]); // Podemos hacer también (*gps).encode(buffer[i])
    }

    return HAL_OK;
}
*/

/*
HAL_StatusTypeDef SamM10q::read_gps_position() {
	if (this->read_nmea_stream() != HAL_OK) {
		return HAL_ERROR;
	}
	this->update_location_and_time();
	return HAL_OK;
}
*/

/*
void SamM10q::update_location_and_time() {
    if (trackerGPS.location.isUpdated() && trackerGPS.location.isValid()) {
        latitude = trackerGPS.location.lat();
        longitude = trackerGPS.location.lng();
    }


    if (trackerGPS.date.isUpdated() && trackerGPS.date.isValid()) {
        uint16_t year = trackerGPS.date.year();   // Ej: 2025
        uint8_t month = trackerGPS.date.month();  // Ej: 6
        uint8_t day   = trackerGPS.date.day();    // Ej: 12

        fechaUTC = (year % 100) * 10000 + month * 100 + day; // yymmdd
    }

    if (trackerGPS.time.isUpdated() && trackerGPS.time.isValid()) {
        uint8_t hour = trackerGPS.time.hour();
        uint8_t minute = trackerGPS.time.minute();
        uint8_t second = trackerGPS.time.second();
        horaUTC = hour * 10000 + minute * 100 + second; // hhmmss como entero
    }
}
*/

/*
bool SamM10q::set_new_acq_time(gpsRateSpeed gpsRate) {
    const size_t idx = static_cast<size_t>(gpsRate);

    // Tablas definidas en sam_m10q_KEYID.h (punteros + len)
    const uint8_t* payload = m10q_new_acq_time[idx];
    size_t payloadlen = m10q_new_acq_time_len[idx];

    // Nota: en el header original m10q_new_adq_time_checksum tenía los mismos 5 bytes.
    // Respetamos ese "checksum" 1:1 para no romper nada.
    const uint8_t* checksum = m10q_new_acq_ck[idx];
    size_t checksumlen = m10q_new_acq_ck_len[idx];

    // Usar arrays estáticos en lugar de std::vector
    uint8_t sendMsgRAM[UBX_MAX_MESSAGE_SIZE];
    uint8_t sendMsgBBR[UBX_MAX_MESSAGE_SIZE];
    
    uint16_t lenRAM = build_ubx_message(VALSET_CLASS, VALSET_ID, RAM, payload, payloadlen, checksum, checksumlen, sendMsgRAM, UBX_MAX_MESSAGE_SIZE);
    uint16_t lenBBR = build_ubx_message(VALSET_CLASS, VALSET_ID, BBR, payload, payloadlen, checksum, checksumlen, sendMsgBBR, UBX_MAX_MESSAGE_SIZE);

	const bool okRAM = (lenRAM > 0) && (send_message(sendMsgRAM, lenRAM, 15) == HAL_OK);
    const bool okBBR = (lenBBR > 0) && (send_message(sendMsgBBR, lenBBR, 15) == HAL_OK);

    return okRAM && okBBR;
}
*/

void SamM10q::testGPS() {
    //printf("[TEST GPS] Iniciando test de GPS...\n");

    if (this->read_gps_position() != HAL_OK) {
        //printf("[TEST GPS] Fallo al leer NMEA\n");
        return;
    } else {
		//printf("[TEST GPS] Posición válida: %.6f, %.6f\n", this->latitude, this->longitude);
    }
}

/* =========================================================== */
/* ========== CONFIGURACION INICIAL DEL GPS VIA I2C ========== */
/* =========================================================== */
void SamM10q::configure_gps() {
    uint8_t response_buffer[UBX_MAX_MESSAGE_SIZE];
    
    write_register_uart(m10q_data_44, sizeof(m10q_data_44), RAM); // Habilitación I2C via UART
    //read_register_uart(m10q_data_44, sizeof(m10q_data_44), response_buffer, UBX_MAX_MESSAGE_SIZE, RAM);
    
    write_register_uart(m10q_data_43, sizeof(m10q_data_43), RAM); // Configuración inicial via UART
    //read_register_uart(m10q_data_43, sizeof(m10q_data_43), response_buffer, UBX_MAX_MESSAGE_SIZE, RAM);

    write_register_uart(m10q_data_43, sizeof(m10q_data_43), BBR); // Configuración inicial via I2C
    //read_register_uart(m10q_data_43, sizeof(m10q_data_43), response_buffer, UBX_MAX_MESSAGE_SIZE, BBR);
    
    write_register(m10q_data_45, sizeof(m10q_data_45), RAM); // Habilitación TRAMAS NMEA UART via I2C

    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++) {
        const uint8_t* payload = m10q_data_payloads[i];
        size_t payload_len = sizeof(m10q_data_payloads[i]);
        
        // Escribir en RAM
        if (!write_register(payload, payload_len, RAM)) {
            // Si falla la escritura en RAM, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en RAM leyendo el registro
        if (payload_len >= 4) {
            if (!read_register(payload, payload_len, response_buffer, UBX_MAX_MESSAGE_SIZE, RAM)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
        
        // Escribir en BBR (persistente)
        if (!write_register(payload, payload_len, BBR)) {
            // Si falla la escritura en BBR, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en BBR
        if (payload_len >= 4) {
            if (!read_register(payload, payload_len, response_buffer, UBX_MAX_MESSAGE_SIZE, BBR)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
    }
}

/* ============================================================ */
/* ========== CONFIGURACION INICIAL DEL GPS VIA UART ========== */
/* ============================================================ */
void SamM10q::configure_gps_uart() {
    uint8_t response_buffer[UBX_MAX_MESSAGE_SIZE];
    
    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++){
        const uint8_t* payload = m10q_data_payloads[i];
        size_t payloadlen = sizeof(m10q_data_payloads[i]);
        
        // Escribir en RAM via UART
        if (!write_register_uart(payload, payloadlen, RAM)) {
            // Si falla la escritura en RAM, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en RAM leyendo el registro
        if (payloadlen >= 4) {
            if (!read_register_uart(payload, payloadlen, response_buffer, UBX_MAX_MESSAGE_SIZE, RAM)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
        
        // Escribir en BBR (persistente) via UART
        if (!write_register_uart(payload, payloadlen, BBR)) {
            // Si falla la escritura en BBR, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en BBR
        if (payloadlen >= 4) {
            if (!read_register_uart(payload, payloadlen, response_buffer, UBX_MAX_MESSAGE_SIZE, BBR)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
    }
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
    buffer[idx++] = version; // 0x00
    buffer[idx++] = layer;   // RAM o BBR

    // 5. Reserved (2 bytes)
    buffer[idx++] = static_cast<uint8_t>(reserved & 0xFF);			// LSB
    buffer[idx++] = static_cast<uint8_t>((reserved >> 8) & 0xFF);	// MSB

    // 6. Append the actual Key Id (first 4 bytes)
    for (size_t i = 0; i < 4; i++) {
        buffer[idx++] = payload_data[i];
    }

    // 7. Append the actual Value if I use VALSET (remaining bytes)
    if(msgID == VALSET_ID) {
        for(size_t i = 4; i < payload_len; i++)
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
    if (!payload_data || payload_len < 4) {
        return false;
    }

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET con Checksum incluido..
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje
    return send_message(buffer, msg_len, 15) == HAL_OK;
}

/* Lectura de registro usando UBX-CFG-VALGET mediante I2C*/
bool SamM10q::read_register(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer) {
    if (!payload_data || payload_len == 0 || (*response_buffer == 0xFF) || buffer_size == 0) {
        return false;
    }

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALGET incluido checksum
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, payload_data, payload_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje de petición
    if (send_message(buffer, msg_len, 15) != HAL_OK) {
        return false;
    }

    HAL_Delay(1000);

    // Leer respuesta del módulo GPS
    if (!i2cBus) {
        return false;
    }

    // Usar bus I2C thread-safe para leer la respuesta
    I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, 1, response_buffer, 1, 100);

    return (result == I2C_OK);
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

/* Lectura de registro usando UBX-CFG-VALGET via UART */
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

    // Usar bus UART thread-safe para leer la respuesta
    UARTResult result = uartBus->receive(response_buffer, buffer_size, 1000);
    
    return (result == UART_OK);
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