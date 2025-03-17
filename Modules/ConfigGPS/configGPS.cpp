/*
 * configGPS.cpp
 *
 *  Created on: Mar 11, 2025
 *      Author: nacho
 */

/* Includes ------------------------------------------------------------------*/

#include "configGPS.h"

/* Private includes ----------------------------------------------------------*/



void gps_create_message(msgGPS_t *msg, uint16_t length, uint16_t layer, uint8_t *data, uint8_t *checksum) {
    msg->header[0] = UBX_HEADER1;
    msg->header[1] = UBX_HEADER2;
    msg->msgClass = VALSET_CLASS;
    msg->msgID = VALSET_ID;
    msg->length = length;
    msg->version = VALSET_VERSION;
    msg->layer = layer;
    msg->reserved = RESERVED;

    memcpy(msg->keyID, data, 0x04); // keyID only has 4 bytes. PESSI: Habría que castearlo a puntero o hacer algo como
    								//								  std::memcpy(&msg->keyID, data, 4);

    memcpy(msg->value.data(), data[3], length - 0x08);	// value begins after the keyID. 0x08 = Number of bytes of the payload before the value.
    													// PESSI: data[3] es un uint8_t, pero memcpy necesita un puntero.
    													//		  habría que hacer algo como memcpy(msg->value.data(), &data[4], length - 0x08);


    msg->checksum[0] = checksum[0];
    msg->checksum[1] = checksum[1];
}


std::vector<uint8_t> to_bytes(const T& msg, size_t numBytes) {
    std::vector<uint8_t> bytes(numBytes);
    T aux = msg;

    for (size_t i = 0; i < numBytes; ++i) {
        bytes[numBytes - 1 - i] = static_cast<uint8_t>(aux& 0xFF);
        aux >>= 8;
    }

    return bytes;
}


std::vector<uint8_t> build_ubx_message(msgGPS_t *msg) {
    std::vector<uint8_t> message;
    std::vector<uint8_t> reserved = to_bytes(msg->reserved, 2);
    std::vector<uint8_t> keyID = to_bytes(msg->keyID, 4);

    // UBX Header
    message.push_back(msg->header[0]);
    message.push_back(msg->header[1]);

    // Message's class & ID
    message.push_back(msg->msgClass);
    message.push_back(msg->msgID);

    // Payload lenght
    message.insert(message.end(), {static_cast<uint8_t>((msj->length >> 8) & 0xFF), static_cast<uint8_t>(msg->length & 0xFF)});
    /*
    message.push_back((msg->length>> 8) & 0xFF); // MSB.
    message.push_back(msg->length& 0xFF);       // LSB.
	*/

    // Payload (version + layer + key ID + value)
    massage.push_back(msg->version);
    message.push_back(msg->layer);
    message.insert(message.end(), reserved.begin(), reserved.end());
    message.insert(message.end(), keyID.begin(), keyID.end());
    //message.insert(message.end(), {static_cast<uint8_t>((msj->reserved >> 8) & 0xFF), static_cast<uint8_t>(msg->reserved & 0xFF)});
    //message.insert(message.end(), {static_cast<uint8_t>((msj->keyID >> 24) & 0xFF), static_cast<uint8_t>((msj->keyID >> 16) & 0xFF), static_cast<uint8_t>((msj->keyID >> 8) & 0xFF), static_cast<uint8_t>(msg->keyID & 0xFF)});
    message.insert(message.end(), msg->value.begin(), msg->value.end());

    // Checksum
    message.push_back(msg->checksum[0]);
    message.push_back(msg->checksum[1]);

    return message;
}


void configure_gps() {
    msgGPS_t msg[];
    std::vector<uint8> sendMsg;

    size_t numMsg = 86;

    // Create UBX message
    for(size_t i = 0; i < numMsg; i+2){
        gps_create_message(msg[i], sizeof(data[i]) + 4, RAM, data, checksum);
        gps_create_message(msg[i+1], sizeof(data[i]) + 4, BBR, data, checksum);
    }

    for(size_t i = 0; i < num_msg; i++){
    	sendMsg = build_ubx_message(msg[i]);
    }

    // NACHO: ¡¡ RECORDAR EL TIEMPO ENTRE MSG DE SIGNAL Y MSG DE SIGNAL!! Ver Interface Description.
    // Aquí debes enviar el mensaje por I2C al GPS
    // i2c_transmit(UBLOX_I2C_ADDRESS, (uint8_t*)&sendMsg[i], sizeof(msg.sync1) + sizeof(msg.sync2) + 4 + msg.length + 2);
    for(size_t i = 0; i < num_msg; i++){
    	if (HAL_I2C_Master_Transmit(&hi2c2, GPS_ADDRESS, sendMsg[i], sizeof(sendMsg[i]), TIMEOUT) != HAL_OK) {
    	    		// Manejo de error

    	}
    }


}
