
/* Includes ------------------------------------------------------------------*/

#include "configGPS.h"

/* Private includes ----------------------------------------------------------*/

extern I2C_HandleTypeDef hi2c2;

void create_msgGPS(msgGPS_t *msg, uint16_t length, uint16_t layer, uint8_t *data, uint8_t *checksum) {
    msg->header[0] = UBX_HEADER1;
    msg->header[1] = UBX_HEADER2;
    msg->msgClass = VALSET_CLASS;
    msg->msgID = VALSET_ID;
    msg->length = length;			// i.e: 0x0009
    msg->version = VALSET_VERSION;
    msg->layer = layer;
    msg->reserved = RESERVED;

    memcpy(&(msg->keyID), data, 0x04);	// keyID only has 4 bytes. PESSI: Habría que castearlo a puntero o hacer algo como memcpy(&msg->keyID, data, 4);
    									// keyID: i.e : NAVSPG-OUTFIL_PACC = 0xB3 00 11 30

    memcpy(msg->value, &data[4], length - LENGTH_PAYLOAD_OFFSET); //Value begins after the keyID. 0x08 = Number of bytes of the payload before the value.


    msg->checksum[0] = checksum[0];
    msg->checksum[1] = checksum[1];
}

void append_big_endian(vector<uint8_t> *message, const T *value) {
    size_t size = sizeof(T);
    uint8_t bytes[size];

    memcpy(bytes, value, size);

    for (size_t i = 0; i < size; i++) {
        message->push_back(bytes[size - 1 - i]);
    }
}

vector<uint8_t> build_ubx_message(msgGPS_t *msg) {
    vector<uint8_t> message;

    // TOMAMOS DE EJEMPLO: NAVSPG-OUTFIL_PACC -> B5 62 06 8A 0A 00 01 01 00 00 |B3 00 11 30| |14 00| A4 0F

    // UBX Header
    message.push_back(msg->header[0]);	// Mensaje: 0xB5
    message.push_back(msg->header[1]);	// Mensaje: 0xB562

    // Message's class & ID
    message.push_back(msg->msgClass);	// Mensaje: 0xB562 06
    message.push_back(msg->msgID);		// Mensaje: 0xB562 068A

    // Payload lenght
    message.push_back(msg->length & 0xFF);			// MSB. // Mensaje: 0xB562 068A 0A
    message.push_back((msg->length >> 8) & 0xFF);	// LSB. // Mensaje: 0xB562 068A 0A00

    // Payload (version + layer + reserved + key ID + value)
    massage.push_back(msg->version);	// Mensaje: 0xB562 068A 0A00 01

    message.push_back(msg->layer);		// Mensaje: 0xB562 068A 0A00 01 01

    message.push_back(msg->reserved & 0xFF);		// MSB. // Mensaje: 0xB562 068A 0A00 01 01 00
    message.push_back((msg->reserved >> 8) & 0xFF);	// LSB. // Mensaje: 0xB562 068A 0A00 01 01 0000

    //A PARTIR DE ACÁ NECESITO GUARDAR EN BIG ENDIAN PARA NO MODIFICAR EL ORDEN QUE ME DA UCENTER DE LOS VALORES.

    append_big_endian(&message, &(msg->keyID));  // Mensaje: 0xB562 068A 0A00 01 01 0000 B3001130

    // SEGUIMOS GUARDANDO EN BIG ENDIAN.
    message.insert(message.end(), msg->value, msg->value + sizeof(msg->value));	//Mensaje: 0xB562 068A 0A00 01 01 0000 B3001130 1400

    // Checksum
    message.push_back(msg->checksum[0]);	//Mensaje: 0xB562 068A 0A00 01 01 0000 B3001130 1400 A4
    message.push_back(msg->checksum[1]);	//Mensaje: 0xB562 068A 0A00 01 01 0000 B3001130 1400 A4 0F

    return message;
}


void configure_gps() {
	size_t numMsg = ( sizeof(data) / sizeof(data[0]) ) * 2;
    msgGPS_t msgRAM, msgBBR;
    vector<uint8> sendMsgRAM, sendMsgBBR;

    for(size_t i = 0; i < numMsg; i++){
    	size_t j = 0;

    	// CREO ESTRUCTURA PARA GENERAR EL MENSAJE.
    	create_msgGPS(msgRAM, sizeof(data[i]) + 4, RAM, data[i], checksum[j]);
    	create_msgGPS(msgBBR, sizeof(data[i]) + 4, BBR, data[i], checksum[j+1]);

    	// GENERO EL MENSAJE.
    	sendMsgRAM = build_ubx_message(msgRAM);
    	sendMsgBBR = build_ubx_message(msgBBR);

    	j += 2;

    	// ENVÍO EL MENSAJE.
        // NACHO: ¡¡ RECORDAR EL TIEMPO ENTRE MSG DE SIGNAL Y MSG DE SIGNAL!! Ver Interface Description.

		HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDRESS, sendMsgRAM, sizeof(sendMsgRAM), HAL_MAX_DELAY);
		HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDRESS, sendMsgBBR, sizeof(sendMsgBBR), HAL_MAX_DELAY);

    }
}
