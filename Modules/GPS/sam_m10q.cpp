/* Includes ------------------------------------------------------------------*/

#include "sam_m10q.h"
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/

SamM10q::SamM10q(uint8_t i2cAddr, TinyGPSPlus *trackerGPS) {
	this->i2cAddr = i2cAddr;
    this->trackerGPS = trackerGPS;  // Debo declarar previamente:: TinyGPSPlus gps; -> SamM10q gpsModule(0x42, &gps);
	this->header[0] = UBX_HEADER1;
	this->header[1] = UBX_HEADER2;
    this->msgClass 	= VALSET_CLASS;
	this->msgID		= VALSET_ID;
//	this->length 	= Dejo vacio por ahora ya que pensamos hacer lo que dice en el .h.
	this->version	= VALSET_VERSION;
    this->reserved	= RESERVED;
}

void SamM10q::read_nmea_stream(I2C_HandleTypeDef *hi2c) {
    uint8_t buffer[NMEA_BUFFER_SIZE];

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hi2c, i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, buffer, NMEA_BUFFER_SIZE, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        // Podés agregar manejo de error acá si querés
        return;
    }

    for (uint8_t i = 0; i < NMEA_BUFFER_SIZE; ++i) {
        gps->encode(buffer[i]); // Podemos hacer también (*gps).encode(buffer[i])
    }
}

void SamM10q::update_location_and_time() {
    if (trackerGPS->location.isUpdated() && trackerGPS->location.isValid()) {
        latitude = trackerGPS->location.lat();
        longitude = trackerGPS->location.lng();
    }

    if (trackerGPS->date.isUpdated() && trackerGPS->date.isValid()) {
        uint16_t year = trackerGPS->date.year();   // Ej: 2025
        uint8_t month = trackerGPS->date.month();  // Ej: 6
        uint8_t day   = trackerGPS->date.day();    // Ej: 12

        fechaUTC = (year % 100) * 10000 + month * 100 + day; // yymmdd
    }

    if (trackerGPS->time.isUpdated() && trackerGPS->time.isValid()) {
        uint8_t hour = trackerGPS->time.hour();
        uint8_t minute = trackerGPS->time.minute();
        uint8_t second = trackerGPS->time.second();
        horaUTC = hour * 10000 + minute * 100 + second; // hhmmss como entero
    }
}

void SamM10q::configure_gps() {
    std::vector<uint8_t> sendMsgRAM, sendMsgBBR;

    for(size_t i = 0; i < m10q_data.size(); i++){       
    	// GENERO EL MENSAJE.
    	sendMsgRAM = build_full_message_from_index(i, RAM);
    	sendMsgBBR = build_full_message_from_index(i, BBR);

    	// ENVÍO EL MENSAJE.
        // NACHO: ¡¡ RECORDAR EL TIEMPO ENTRE MSG DE SIGNAL Y MSG DE SIGNAL!! Ver Interface Description.
        //PESSI: AGREGO EL DELAY EN LA FUNCIÓN send_message.

		send_message(&hi2c2, sendMsgRAM, 15);
		send_message(&hi2c2, sendMsgBBR, 15);
    }
}

std::vector<uint8_t> SamM10q::build_full_message_from_index(size_t i, uint8_t layer) {
	// Validación simple
    if (i >= m10q_data.size() || i >= m10q_checksum.size()) {
    	// Índice fuera de rango → devolvemos mensaje vacío
    	return {};
    }

    const std::vector<uint8_t>& payload   = m10q_data[i];
    const std::vector<uint8_t>& checksum  = m10q_checksum[i];

    // Llamamos a la función que arma el frame UBX completo
    return build_ubx_message(layer, payload, checksum);
}

vector<uint8_t> SamM10q::build_ubx_message(uint8_t layer, const std::vector<uint8_t> payload, const std::vector<uint8_t> checksum) {
	std::vector<uint8_t> message;

	// 1. Sync chars
    message.push_back(header[0]);	// 0xB5
    message.push_back(header[1]);	// 0x62

	// 2. Class & ID
    message.push_back(msgClass);	// 0x06
    message.push_back(msgID);		// 0x8A

    // 3. Payload length = 4 (version, layer, reserved) + payload size
    uint16_t payloadLength = 4 + payload.size();
    message.push_back(payloadLength & 0xFF);			// Little endian LSB
    message.push_back((payloadLength >> 8) & 0xFF);		// Little endian MSB

    // 4. Payload header
    message.push_back(version); // 0x00
    message.push_back(layer);   // RAM o BBR

    // Reserved (2 bytes)
    message.push_back(reserved & 0xFF);			// LSB
    message.push_back((reserved >> 8) & 0xFF);	// MSB

    // 5. Append the actual payload (KEY + VALUEs)
    message.insert(message.end(), payload.begin(), payload.end());

	// 6. Append checksum directamente
	message.push_back(checksum[0]); // CK_A
    message.push_back(checksum[1]);	// CK_B

	return message;
}

HAL_StatusTypeDef SamM10q::send_message(I2C_HandleTypeDef* hi2c, const std::vector<uint8_t>& message, uint32_t delay_ms) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c,i2cAddr, 0xFF, I2C_MEMADD_SIZE_8BIT, message.data(), message.size(), HAL_MAX_DELAY);

    // Delay para que el módulo procese
    HAL_Delay(delay_ms);

    return status;
}

void SamM10q::ubx_calculate_checksum(UBX_Message *msg) {

}


