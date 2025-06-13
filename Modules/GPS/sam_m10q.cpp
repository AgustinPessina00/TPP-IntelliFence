/* Includes ------------------------------------------------------------------*/

#include "sam_m10q.h"
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/

SamM10q::SamM10q(uint8_t i2cAddr) {
	this->i2cAddr = i2cAddr;
	this->header[0] = UBX_HEADER1;
	this->header[1] = UBX_HEADER2;
    this->msgClass 	= VALSET_CLASS;
	this->msgID		= VALSET_ID;
//	this->length 	= Dejo vacio por ahora ya que pensamos hacer lo que dice en el .h.
	this->version	= VALSET_VERSION;
    this->reserved	= RESERVED;
}

void SamM10q::create_msgGPS() {

}

void SamM10q::configure_gps() {

}

std::vector<uint8_t> SamM10q::build_full_message_from_index(size_t i) {
	// Validación simple
    if (i >= m10q_data.size() || i >= m10q_checksum.size()) {
    	// Índice fuera de rango → devolvemos mensaje vacío
    	return {};
    }

    const std::vector<uint8_t>& payload   = m10q_data[i];
    const std::vector<uint8_t>& checksum  = m10q_checksum[i];

    uint8_t layer = RAM; // Por defecto usamos RAM. Habría que ver si le pasamos el parámetro a la función.

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

	return vector<uint8_t>();
}

void SamM10q::ubx_calculate_checksum(UBX_Message *msg) {

}


