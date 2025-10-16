/* Includes ------------------------------------------------------------------*/

#include "sam_m10q.h"
#include "stm32wlxx_hal.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
void BusyDelayMs(uint32_t ms);

//static uint8_t buffer[NMEA_BUFFER_SIZE];

SamM10q::SamM10q(I2C_HandleTypeDef *hi2c, uint8_t i2cAddr) {
	this->i2cAddr = i2cAddr;
    this->hi2c = hi2c;

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
    configure_gps();
}

HAL_StatusTypeDef SamM10q::read_nmea_stream() {
    uint8_t buffer[NMEA_BUFFER_SIZE];
    HAL_StatusTypeDef status = HAL_ERROR;

    if (HAL_I2C_IsDeviceReady(hi2c, i2cAddr, 1, 2) == HAL_OK) {
    	status = HAL_I2C_Mem_Read(hi2c, i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, buffer, NMEA_BUFFER_SIZE, 10);
    }


    if (status != HAL_OK) {
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
    // Respetamos ese “checksum” 1:1 para no romper nada.
    const uint8_t* checksum = m10q_new_acq_ck[idx];
    size_t checksumlen = m10q_new_acq_ck_len[idx];

    std::vector<uint8_t> sendMsgRAM = build_ubx_message(RAM, payload, payloadlen, checksum, checksumlen);
    std::vector<uint8_t> sendMsgBBR = build_ubx_message(BBR, payload, payloadlen, checksum, checksumlen);

	const bool okRAM = (send_message(sendMsgRAM, 15) == HAL_OK);
    const bool okBBR = (send_message(sendMsgBBR, 15) == HAL_OK);

    return okRAM && okBBR;
}

/* Configuración completa: recorre las 43 tuplas (payload + checksum) */
void SamM10q::configure_gps() {
    std::vector<uint8_t> sendMsgRAM, sendMsgBBR;

    HAL_StatusTypeDef status = HAL_ERROR;

    for(size_t i = 0; i < M10Q_NUM_DATA_ELEMENTS; i++){
    	// GENERO EL MENSAJE.
    	sendMsgRAM = build_full_message_from_index(i, RAM);
    	sendMsgBBR = build_full_message_from_index(i, BBR);

    	// ENVÍO EL MENSAJE.
        // NACHO: ¡¡ RECORDAR EL TIEMPO ENTRE MSG DE SIGNAL Y MSG DE SIGNAL!! Ver Interface Description.
        //PESSI: AGREGO EL DELAY EN LA FUNCIÓN send_message.

        // TODO: sacar números mágicos/hardcodeados.
		status = send_message(sendMsgRAM, 15);
		if(status == HAL_OK){
			status = send_message(sendMsgBBR, 15);
			if(status != HAL_OK){
				i--;
			}
		}
		else {
			i--;
		}

    }
}

/* Toma i y arma el frame UBX completo usando las tablas sin heap global */
std::vector<uint8_t> SamM10q::build_full_message_from_index(size_t i, uint8_t layer) {
	// Validación simple
    if (i >= M10Q_NUM_DATA_ELEMENTS) {
    	// Índice fuera de rango → devolvemos mensaje vacío
    	return {};
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
    return build_ubx_message(layer, payload, payloadlen, checksum, checksumlen);
}

/* Arma el frame UBX-VALSET evitando realocaciones innecesarias */
std::vector<uint8_t> SamM10q::build_ubx_message(uint8_t layer, const uint8_t* payload, size_t payload_len, const uint8_t* ck, size_t ck_len) {
	std::vector<uint8_t> message;

	// 1. Sync chars
    message.push_back(header[0]);	// 0xB5
    message.push_back(header[1]);	// 0x62

	// 2. Class & ID
    message.push_back(msgClass);	// 0x06
    message.push_back(msgID);		// 0x8A

    // 3. Payload length = 4 (version, layer, reserved) + payload size
    uint16_t payloadLength = static_cast<uint16_t>(4 + payload_len);
    message.push_back(static_cast<uint8_t>(payloadLength & 0xFF));			// Little endian LSB
    message.push_back(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));	// Little endian MSB

    // 4. Payload header
    message.push_back(version); // 0x00
    message.push_back(layer);   // RAM o BBR

    // Reserved (2 bytes)
    message.push_back(static_cast<uint8_t>(reserved & 0xFF));			// LSB
    message.push_back(static_cast<uint8_t>((reserved >> 8) & 0xFF));	// MSB

    // 5. Append the actual payload (KEY + VALUEs)
    if (payload_len > 0 && payload != nullptr) {
        message.insert(message.end(), payload, payload + payload_len);
    }

	// 6. Append checksum directamente
    if (ck_len >= 2 && ck != nullptr) {
        message.push_back(ck[0]); // CK_A
        message.push_back(ck[1]); // CK_B
    }

	return message;
}

HAL_StatusTypeDef SamM10q::send_message(const std::vector<uint8_t>& message, uint32_t delay_ms) {
	HAL_StatusTypeDef status = HAL_ERROR;

	uint8_t *data = const_cast<uint8_t*>(message.data());
	uint16_t dataLength = message.size();

	if(HAL_I2C_IsDeviceReady(this->hi2c, this->i2cAddr, 100, 100) == HAL_OK){
		status = HAL_I2C_Master_Transmit(this->hi2c, this->i2cAddr, data, dataLength, 100);
	}

    // Delay para que el módulo procese
    BusyDelayMs(delay_ms);

    return status;
}

void BusyDelayMs(uint32_t ms) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (HAL_RCC_GetHCLKFreq() / 1000) * ms;
    while ((DWT->CYCCNT - start) < ticks);
}

void SamM10q::ubx_calculate_checksum(std::vector<uint8_t> msg) {

}

void SamM10q::testGPS() {
    //printf("[TEST GPS] Iniciando test de GPS...\n");

    if (this->read_gps_position() != HAL_OK) {
        //printf("[TEST GPS] Fallo al leer NMEA\n");
        return;
    } else {
    	double lat = this->latitude;
		double lon = this->longitude;
		//printf("[TEST GPS] Posición válida: %.6f, %.6f\n", lat, lon);
    }
}
