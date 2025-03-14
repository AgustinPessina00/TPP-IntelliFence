/*
 * configGPS.h
 *
 *  Created on: Mar 11, 2025
 *      Author: Agustín
 */

#ifndef MODULES_CONFIGGPS_CONFIGGPS_H_
#define MODULES_CONFIGGPS_CONFIGGPS_H_


#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A
#define VALSET_VERSION 0x00 // 0x00 = transactionless, 0x01 = w/ transactions //CHECK!!!!
#define RAM 0x01000000		//CHEQUEAR!!! -> PESSI: Agrego para completar los 32 bits.
#define BBR 0x01020000		//CHEQUEAR!!! -> PESSI: Agrego para completar los 32 bits.


static const uint8_t *payload[] = {{0x01, 0x01, 0x00, 0x00, 0x1F, 0x00, 0x31, 0x10, 0x01},		// GPS_ENA (RAM).
								  { 0x01, 0x02, 0x00, 0x00, 0x1F, 0x00, 0x31, 0x10, 0x01}		// GPS_ENA (BBR)
};

static const uint8_t *checksum[] = {{0xFC, 0x89},		// GPS_ENA (RAM).
								   {0xFD, 0x91}			// GPS_ENA (BBR).
};



typedef struct msgGPS {
	uint8_t header[2];
	uint8_t msgClass;
	uint8_t msgID;
	uint16_t length;	// Payload length without checksum.
	uint32_t layer;		// 2 bytes LAYER + 2 bytes RESERVED.
	uint8_t version;
	uint8_t keyID[];
	uint8_t value[];
	uint8_t checksum[];
} msgGPS_t;


// *msg es la estructura del tipo msgGPS_t
void gps_create_message(msgGPS_t *msg, uint16_t length, uint32_t layer, uint8_t *data) {
    msg->header[0] = UBX_HEADER1;
    msg->header[1] = UBX_HEADER2;
    msg->msgClass = VALSET_CLASS;
    msg->msgID = VALSET_ID;
    msg->length = length;
    msg->version = VALSET_VERSION;

    /*
	msg->layer = layer; NO HACE FALTA, PORQUE ESTÁ DENTRO DEL PAYLOAD. -> PESSI: OJO, en ese caso tendriamos que pasarle no solo data,
    																			 sino también el layer al memcpy, es más complicado.
    */

    // Copiar el payload
    memcpy(msg->payload, data, length);

    // Calcular el checksum
    ubx_calculate_checksum(msg);
}



void configure_nmea_output() {
    msgGPS_t msg[];

    /*
    •ESTÁ OK layer?? -> PESSI: Layer debe ser de 32 bits, hay que corregir, tal como lo hice arriba.

    uint8_t payload[] = {~09~ (Length), ~00~ (Version, 00 o 01), 01 00 00 00 (Layer RAM), 00 00 (KEYID CFG-SIGNAL),
     	 	 	 	 	 1F 00 31 10 (Value), 01, ~FC~ (shecksum A), ~89~ (Checksum B)};

    •Los que tienen ~xx~ no cuentan en el payload para la length.

    •Si el "LENGTH" se cuenta desde el primer byte del layer hasta el "VALUE" está ok que sea 9. No sé por qué no se cuenta el byte de "VERSION".
	*/

    // Crear el mensaje UBX
    for(i = 0; i < lenght; i++){
        gps_create_message(msg[i], sizeof(payload), RAM, payload);
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
