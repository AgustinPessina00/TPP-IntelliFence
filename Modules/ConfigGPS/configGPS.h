/*
 * configGPS.h
 *
 *  Created on: Mar 11, 2025
 *      Author: Agustín
 */

#ifndef MODULES_CONFIGGPS_CONFIGGPS_H_
#define MODULES_CONFIGGPS_CONFIGGPS_H_


#define UBX_SYNC1  0xB5
#define UBX_SYNC2  0x62


static const uint8_t *enableGPS[] = {{0x02, 0x20, 0x72, 0x10, 0x01, 0x00, 0x00, 0x00},		// ENABLE NMEA
									 {0x02, 0x20, 0x72, 0x10, 0x01, 0x00, 0x00, 0x00},
									 {0x02, 0x20, 0x72, 0x10, 0x01, 0x00, 0x00, 0x00}
};

typedef struct msgGPS {
	uint8_t sync[];
	uint8_t msgClass;
	uint8_t msgID;
	uint16_t length; // Length del payload.
	uint32_t layer;
	uint8_t keyID[];
	uint8_t value[];
	uint8_t checksum[];
} msgGPS_t;


// *msg es la estructura
void ubx_create_message(UBX_Message *msg, uint8_t msgClass, uint8_t msgID, uint8_t *data, uint16_t length, uint32_t layer) {
    msg->sync[0] = UBX_SYNC1;
    msg->sync[1] = UBX_SYNC2;
    msg->msgClass = msgClass;
    msg->msgID = msgID;
    msg->length = length;
    msg->layer = layer;

    // Copiar el payload
    memcpy(msg->payload, data, length);

    // Calcular el checksum
    ubx_calculate_checksum(msg);
}



void configure_nmea_output() {
    msgGPS_t msg[];

    // CFG-I2COUTPROT-NMEA (Key ID: 0x10720002, habilitar salida NMEA)
    uint8_t payload[] = { 0x02, 0x20, 0x72, 0x10, 0x01, 0x00, 0x00, 0x00 };

    // Crear el mensaje UBX
    for(){
        ubx_create_message(msg[i], 0x06, 0x8A, payload, sizeof(payload));
    }


    // Aquí debes enviar el mensaje por I2C al GPS
    // i2c_transmit(UBLOX_I2C_ADDRESS, (uint8_t*)&msg, sizeof(msg.sync1) + sizeof(msg.sync2) + 4 + msg.length + 2);
}




void ubx_calculate_checksum(UBX_Message *msg) {
    uint8_t ck_a = 0, ck_b = 0;
    uint16_t len = msg->length;

    // Sumar desde classID hasta el final del payload
    ck_a += msg->classID; ck_b += ck_a;
    ck_a += msg->msgID;   ck_b += ck_a;
    ck_a += (len & 0xFF); ck_b += ck_a; // Byte bajo de length
    ck_a += (len >> 8);   ck_b += ck_a; // Byte alto de length

    for (uint16_t i = 0; i < len; i++) {
        ck_a += msg->payload[i];
        ck_b += ck_a;
    }

    msg->ck_a = ck_a;
    msg->ck_b = ck_b;
}







#endif /* MODULES_CONFIGGPS_CONFIGGPS_H_ */
