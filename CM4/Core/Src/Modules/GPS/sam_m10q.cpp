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
	this->i2cAddr = i2cAddr;
    this->i2cBus = nullptr;   // Se inicializa en initSamM10q()
    this->uartBus = nullptr;  // Se inicializa en initSamM10q()
    //this->length 	= Dejo vacio por ahora ya que pensamos hacer lo que dice en el .h.

	this->version	= VALSET_VERSION;
    this->reserved	= RESERVED;

    //initSamM10q();
    //configure_gps();  //A partir de ahora lo llamamos en initSamM10q.
}

/* Llamar luego de inicializar HAL e I2C */
void SamM10q::initSamM10q() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Inicializar el bus I2C thread-safe
    i2cBus = &I2CManager::getBus2();
    
    // Inicializar el bus UART thread-safe
    uartBus = &UARTManager::getUart1();
    
    configure_gps();
}

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

HAL_StatusTypeDef SamM10q::read_gps_position() {
	if (this->read_nmea_stream() != HAL_OK) {
		return HAL_ERROR;
	}
	this->update_location_and_time();
	return HAL_OK;
}

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

/* Configuración completa: recorre las 43 tuplas (payload + checksum) */
void SamM10q::configure_gps() {
    uint8_t response_buffer[UBX_MAX_MESSAGE_SIZE];
    write_register_uart(m10q_data_44, m10q_data_len[0], RAM); // Configuración inicial via UART
    HAL_Delay(1000);
    //uartBus->flushRxBuffer();
    read_register_uart(m10q_data_44, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, RAM);
    
    write_register_uart(m10q_data_43, m10q_data_len[0], RAM); // Configuración inicial via UART
    read_register_uart(m10q_data_43, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, RAM);

    write_register_uart(m10q_data_43, m10q_data_len[0], BBR); // Configuración inicial via I2C
    read_register_uart(m10q_data_43, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, BBR);
    
    //write_register(m10q_data_45, 5, RAM);

    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++){
        const uint8_t* payload = m10q_data_payloads[i];
        size_t payloadlen = m10q_data_len[i];
        
        // Escribir en RAM
        if (!write_register(payload, payloadlen, RAM)) {
            // Si falla la escritura en RAM, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en RAM leyendo el registro
        // Las primeras 4 bytes del payload son la KEY
        if (payloadlen >= 4) {
            if (!read_register(payload, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, RAM)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
        
        // Escribir en BBR (persistente)
        if (!write_register(payload, payloadlen, BBR)) {
            // Si falla la escritura en BBR, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en BBR
        if (payloadlen >= 4) {
            if (!read_register(payload, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, BBR)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
    }
}


/* Arma el frame UBX-VALSET usando array estático (embedded friendly) */
uint16_t SamM10q::build_ubx_message(uint8_t msgClass, uint8_t msgID, uint8_t layer, const uint8_t* keyId, size_t keyLen, const uint8_t* value, size_t valueLen, uint8_t* buffer, uint16_t buffer_size) {
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
    uint16_t payloadLength = static_cast<uint16_t>(4 + keyLen + valueLen);
    buffer[idx++] = static_cast<uint8_t>(payloadLength & 0xFF);			// Little endian LSB
    buffer[idx++] = static_cast<uint8_t>((payloadLength >> 8) & 0xFF);	// Little endian MSB

    // 4. Payload header
    buffer[idx++] = version; // 0x00
    buffer[idx++] = layer;   // RAM o BBR

    // Reserved (2 bytes)
    buffer[idx++] = static_cast<uint8_t>(reserved & 0xFF);			// LSB
    buffer[idx++] = static_cast<uint8_t>((reserved >> 8) & 0xFF);	// MSB

    // 5. Append the actual payload (KEY + VALUEs)
    if (keyLen > 0 && keyId != nullptr) {
        for (size_t i = 0; i < keyLen; i++) {
            if (idx >= buffer_size) return 0;  // Buffer overflow protection
            buffer[idx++] = keyId[i];
        }
    }

    if (msgID == VALSET_ID && valueLen > 0 && value != nullptr) {
        for (size_t i = 0; i < valueLen; i++) {
            if (idx >= buffer_size) return 0;  // Buffer overflow protection
            buffer[idx++] = value[i];
        }
    }

    //calcular el checksum UBX
    uint8_t ck_a = 0, ck_b = 0;
    ubx_calculate_checksum(buffer, idx, &ck_a, &ck_b);
    buffer[idx++] = ck_a;
    buffer[idx++] = ck_b;

	return idx;  // Retorna la longitud total del mensaje
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

void BusyDelayMs(uint32_t ms) {
    // Usar HAL_Delay que ahora funciona correctamente con TIM2 timebase
    HAL_Delay(ms);
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

void SamM10q::testGPS() {
    //printf("[TEST GPS] Iniciando test de GPS...\n");

    if (this->read_gps_position() != HAL_OK) {
        //printf("[TEST GPS] Fallo al leer NMEA\n");
        return;
    } else {
		//printf("[TEST GPS] Posición válida: %.6f, %.6f\n", this->latitude, this->longitude);
    }
}

/* Escritura de registro usando UBX-CFG-VALSET */
bool SamM10q::write_register(const uint8_t* key_value_data, size_t data_len, uint8_t layer) {
    if (!key_value_data || data_len < 4) {
        return false;
    }
    
    uint8_t keyId[4];
    for (size_t i = 0; i < 4; i++) {
        keyId[i] = key_value_data[i];
    }

    uint8_t value[UBX_MAX_MESSAGE_SIZE];
    for (size_t i = 0; i < data_len - 4; i++) {
        value[i] = key_value_data[i + 4];
    }
    size_t value_len = data_len - 4;

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET sin checksum
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, keyId, 4, value, value_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len + 2 > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }

    // Enviar mensaje
    return send_message(buffer, msg_len, 15) == HAL_OK;
}

/* Lectura de registro usando UBX-CFG-VALGET */
bool SamM10q::read_register(const uint8_t* key_data, size_t len_key_data, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer) {
    if (!key_data || len_key_data == 0 || !response_buffer || buffer_size == 0) {
        return false;
    }

    uint8_t keyId[4];
    for (size_t i = 0; i < 4; i++) {
        keyId[i] = key_data[i];
    }


    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALGET sin checksum (las keys son el payload)
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, keyId, 4, nullptr, 0, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len + 2 > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }
    uint8_t response_buffer_1[UBX_MAX_MESSAGE_SIZE];
    uint8_t response_buffer_2[UBX_MAX_MESSAGE_SIZE];
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
    I2CResult result = i2cBus->memRead(i2cAddr, 0xFD, 1, response_buffer_1, 1, 100);
    I2CResult result1 = i2cBus->memRead(i2cAddr, 0xFE, 1, response_buffer_2, 1, 100);
    I2CResult result2 = i2cBus->memRead(i2cAddr, 0xFF, 1, response_buffer, (response_buffer_1[0] << 8) | response_buffer_2[0], 100);
    return (result == I2C_OK) && (result1 == I2C_OK) && (result2 == I2C_OK);
}

/* ========== FUNCIONES UART PARA CONFIGURACIÓN INICIAL ========== */

HAL_StatusTypeDef SamM10q::send_message_uart(const uint8_t* message, uint16_t message_length, uint32_t delay_ms) {
    if (!uartBus || !message || message_length == 0) {
        return HAL_ERROR;
    }

    // Usar método transmit thread-safe del UARTBus
    // deviceAddr = 0 para UART puro (sin addressing)
    UARTResult result = uartBus->transmit(0, message, message_length, 100);
    
    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return (result == UART_OK) ? HAL_OK : HAL_ERROR;
}

/* Escritura de registro usando UBX-CFG-VALSET via UART */
bool SamM10q::write_register_uart(const uint8_t* key_value_data, size_t data_len, uint8_t layer) {
    if (!key_value_data || data_len == 0) {
        return false;
    }

    uint8_t keyId[4];
    for (size_t i = 0; i < 4; i++) {
        keyId[i] = key_value_data[i];
    }

    uint8_t value[UBX_MAX_MESSAGE_SIZE];
    for (size_t i = 0; i < data_len - 4; i++) {
        value[i] = key_value_data[i + 4];
    }
    size_t value_len = data_len - 4;

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALSET sin checksum
    uint16_t msg_len = build_ubx_message(VALSET_CLASS, VALSET_ID, layer, keyId, 4, value, value_len, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len + 2 > UBX_MAX_MESSAGE_SIZE) {
        return false;
    }
    
    // Enviar mensaje via UART
    return send_message_uart(buffer, msg_len, 15) == HAL_OK;
}

/* Lectura de registro usando UBX-CFG-VALGET via UART */
bool SamM10q::read_register_uart(const uint8_t* key_data, size_t len_key_data, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer) {
    if (!key_data || len_key_data == 0 || !response_buffer || buffer_size == 0) {
        return false;
    }

    uint8_t keyId[4];
    for (size_t i = 0; i < 4; i++) {
        keyId[i] = key_data[i];
    }

    uint8_t buffer[UBX_MAX_MESSAGE_SIZE];

    // Construir mensaje VALGET sin checksum (las keys son el payload)
    uint16_t msg_len = build_ubx_message(VALGET_CLASS, VALGET_ID, layer, keyId, 4, nullptr, 0, buffer, UBX_MAX_MESSAGE_SIZE);
    
    if (msg_len == 0 || msg_len + 2 > UBX_MAX_MESSAGE_SIZE) {
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
    printf("[GPS] Recibidos %u bytes, resultado: %d\n", bytesReceived, result);
    
    return (result == UART_OK && bytesReceived > 0);
}

/* Configuración inicial del GPS via UART */
void SamM10q::configure_gps_uart() {
    uint8_t response_buffer[UBX_MAX_MESSAGE_SIZE];
    
    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++){
        const uint8_t* payload = m10q_data_payloads[i];
        size_t payloadlen = m10q_data_len[i];
        
        // Escribir en RAM via UART
        if (!write_register_uart(payload, payloadlen, RAM)) {
            // Si falla la escritura en RAM, reintentar
            i--;
            continue;
        }
        
        // Verificar escritura en RAM leyendo el registro
        // Las primeras 4 bytes del payload son la KEY
        if (payloadlen >= 4) {
            if (!read_register_uart(payload, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, RAM)) {
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
            if (!read_register_uart(payload, 4, response_buffer, UBX_MAX_MESSAGE_SIZE, BBR)) {
                // Si falla la lectura de verificación, reintentar
                i--;
                continue;
            }
        }
    }
}