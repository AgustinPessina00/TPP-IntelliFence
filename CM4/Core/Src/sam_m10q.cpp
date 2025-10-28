/* Includes ------------------------------------------------------------------*/

#include "../../Modules/GPS/sam_m10q.h"
#include "stm32wlxx_hal.h"
#include "I2CManager.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
void BusyDelayMs(uint32_t ms);

//static uint8_t buffer[NMEA_BUFFER_SIZE];

SamM10q::SamM10q(uint8_t i2cAddr) {
	this->i2cAddr = i2cAddr;
    this->i2cBus = nullptr;  // Se inicializa en initSamM10q()

    this->header[0] = UBX_HEADER1;
	this->header[1] = UBX_HEADER2;
    this->msgClass 	= VALSET_CLASS;
	this->msgID		= VALSET_ID;

    //this->length 	= Dejo vacio por ahora ya que pensamos hacer lo que dice en el .h.

	this->version	= VALSET_VERSION;
    this->reserved	= RESERVED;

    //configure_gps();  //A partir de ahora lo llamamos en initSamM10q.
}

/* Llamar luego de inicializar HAL e I2C */
void SamM10q::initSamM10q() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Inicializar el bus I2C thread-safe
    i2cBus = &I2CManager::getBus2();
    
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
    
    uint16_t lenRAM = build_ubx_message(RAM, payload, payloadlen, checksum, checksumlen, sendMsgRAM, UBX_MAX_MESSAGE_SIZE);
    uint16_t lenBBR = build_ubx_message(BBR, payload, payloadlen, checksum, checksumlen, sendMsgBBR, UBX_MAX_MESSAGE_SIZE);

	const bool okRAM = (lenRAM > 0) && (send_message(sendMsgRAM, lenRAM, 15) == HAL_OK);
    const bool okBBR = (lenBBR > 0) && (send_message(sendMsgBBR, lenBBR, 15) == HAL_OK);

    return okRAM && okBBR;
}

/* Configuración completa: recorre las 43 tuplas (payload + checksum) */
void SamM10q::configure_gps() {
    // Usar arrays estáticos en lugar de std::vector
    uint8_t sendMsgRAM[UBX_MAX_MESSAGE_SIZE];
    uint8_t sendMsgBBR[UBX_MAX_MESSAGE_SIZE];

    HAL_StatusTypeDef status = HAL_ERROR;

    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++){
    	// GENERO EL MENSAJE.
    	uint16_t lenRAM = build_full_message_from_index(i, RAM, sendMsgRAM, UBX_MAX_MESSAGE_SIZE);
    	uint16_t lenBBR = build_full_message_from_index(i, BBR, sendMsgBBR, UBX_MAX_MESSAGE_SIZE);

    	// ENVÍO EL MENSAJE.
        // NACHO: ¡¡ RECORDAR EL TIEMPO ENTRE MSG DE SIGNAL Y MSG DE SIGNAL!! Ver Interface Description.
        //PESSI: AGREGO EL DELAY EN LA FUNCIÓN send_message.

        // TODO: sacar números mágicos/hardcodeados.
        if (lenRAM > 0) {
            status = send_message(sendMsgRAM, lenRAM, 15);
            if(status == HAL_OK && lenBBR > 0){
                status = send_message(sendMsgBBR, lenBBR, 15);
                if(status != HAL_OK){
                    i--;
                }
            }
            else {
                i--;
            }
        }
        else {
            i--;
        }
    }
}

/* Toma i y arma el frame UBX completo usando las tablas sin heap global */
uint16_t SamM10q::build_full_message_from_index(size_t i, uint8_t layer, uint8_t* buffer, uint16_t buffer_size) {
	// Validación simple
    if (i >= M10Q_NUM_DATA_ELEMENTS || !buffer || buffer_size < UBX_MAX_MESSAGE_SIZE) {
    	// Índice fuera de rango o buffer inválido → devolvemos 0
    	return 0;
    }

    const uint8_t* payload = m10q_data_payloads[i];
    size_t payloadlen = m10q_data_len[i];

    // Cada payload tiene un par de 2 bytes de checksum precalculado
    const uint8_t* checksum = nullptr;
    if(layer == RAM){
    	checksum = m10q_checksum_vals[2*i];
    }
    else if(layer == BBR){
    	checksum = m10q_checksum_vals[2*i+1];
    }

    size_t checksumlen = 2;

    // Llamamos a la función que arma el frame UBX completo
    return build_ubx_message(layer, payload, payloadlen, checksum, checksumlen, buffer, buffer_size);
}

/* Arma el frame UBX-VALSET usando array estático (embedded friendly) */
uint16_t SamM10q::build_ubx_message(uint8_t layer, const uint8_t* payload, size_t payload_len, const uint8_t* ck, size_t ck_len, uint8_t* buffer, uint16_t buffer_size) {
    if (!buffer || buffer_size < UBX_MAX_MESSAGE_SIZE) {
        return 0;  // Buffer inválido
    }

    uint16_t idx = 0;

	// 1. Sync chars
    buffer[idx++] = header[0];	// 0xB5
    buffer[idx++] = header[1];	// 0x62

	// 2. Class & ID
    buffer[idx++] = msgClass;	// 0x06
    buffer[idx++] = msgID;		// 0x8A

    // 3. Payload length = 4 (version, layer, reserved) + payload size
    uint16_t payloadLength = static_cast<uint16_t>(4 + payload_len);
    buffer[idx++] = static_cast<uint8_t>(payloadLength & 0xFF);			// Little endian LSB
    buffer[idx++] = static_cast<uint8_t>((payloadLength >> 8) & 0xFF);	// Little endian MSB

    // 4. Payload header
    buffer[idx++] = version; // 0x00
    buffer[idx++] = layer;   // RAM o BBR

    // Reserved (2 bytes)
    buffer[idx++] = static_cast<uint8_t>(reserved & 0xFF);			// LSB
    buffer[idx++] = static_cast<uint8_t>((reserved >> 8) & 0xFF);	// MSB

    // 5. Append the actual payload (KEY + VALUEs)
    if (payload_len > 0 && payload != nullptr) {
        for (size_t i = 0; i < payload_len; i++) {
            if (idx >= buffer_size) return 0;  // Buffer overflow protection
            buffer[idx++] = payload[i];
        }
    }

	// 6. Append checksum directamente
    if (ck_len >= 2 && ck != nullptr) {
        if (idx + 1 >= buffer_size) return 0;  // Buffer overflow protection
        buffer[idx++] = ck[0]; // CK_A
        buffer[idx++] = ck[1]; // CK_B
    }

	return idx;  // Retorna la longitud total del mensaje
}

HAL_StatusTypeDef SamM10q::send_message(const uint8_t* message, uint16_t message_length, uint32_t delay_ms) {
    if (!i2cBus || !message || message_length == 0) {
        return HAL_ERROR;
    }

    // Usar método transmit thread-safe del I2CBus para envío directo sin registros
    I2CResult result = i2cBus->transmit(i2cAddr, message, message_length, 100);
    
    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return (result == I2C_OK) ? HAL_OK : HAL_ERROR;
}

void BusyDelayMs(uint32_t ms) {
    // Usar HAL_Delay que ahora funciona correctamente con TIM2 timebase
    HAL_Delay(ms);
}

void SamM10q::ubx_calculate_checksum(const uint8_t* msg, uint16_t length, uint8_t* ck_a, uint8_t* ck_b) {
    if (!msg || length == 0 || !ck_a || !ck_b) {
        return;
    }
    
    *ck_a = 0;
    *ck_b = 0;
    
    // Calcular checksum UBX Fletcher (Class + ID + Length + Payload)
    for (uint16_t i = 2; i < length - 2; i++) {  // Excluir sync chars y checksum final
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
