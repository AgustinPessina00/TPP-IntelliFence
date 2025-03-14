/*
 * configGPS.h
 *
 *  Created on: Mar 11, 2025
 *      Author: Agustín | Ignacio | Ezequiel
 */

#ifndef MODULES_CONFIGGPS_CONFIGGPS_H_
#define MODULES_CONFIGGPS_CONFIGGPS_H_


#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A
#define VALSET_VERSION 0x00 // 0x00 = transactionless, 0x01 = w/ transactions
#define RAM 0x01
#define BBR 0x02
#define RESERVED 0x0000


/*
static const uint8_t *payload[] = {{0x01, 0x01, 0x00, 0x00, 0x1F, 0x00, 0x31, 0x10, 0x01},		// GPS_ENA (RAM).
								  { 0x01, 0x02, 0x00, 0x00, 0x1F, 0x00, 0x31, 0x10, 0x01}		// GPS_ENA (BBR).
};
*/


// NACHO: OJO CON EL ENDIANESS!! i.e.: uint16_t length = 0x0900 -> Me devuelve 0x00 0x09 (en la mayoría de STM32..)
// NACHO: 2 SOLUCIONES.
// NACHO: 1. Cambiar tanto LENGTH como DATA a little endian.
// NACHO: 2. Cambiar tanto LENGTH como DATA de tipo, uint8_t [2] y uint8_t [4] respectivamente.
// NACHO: PARECE QUE NO PASA NADA SI COPIAMOS UINT16_T A UINT16_T: Si copias uint16_t a un buffer de uint8_t, verás que los bytes se almacenan en orden little-endian


static const uint8_t *data[] = {
    {0x1F, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-GPS_ENA
    {0x01, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-GPS_L1CA_ENA
    {0x20, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-SBAS_ENA
    {0x05, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-SBAS_L1CA_ENA
    {0x21, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-GAL_ENA
    {0x07, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-GAL_E1_ENA
    {0x22, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-BDS_ENA
    {0x0D, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-BDS_B1_ENA
    {0x0F, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-BDS_B1C_ENA
    {0x24, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-QZSS_ENA
    {0x12, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-QZSS_L1CA_ENA
    {0x14, 0x00, 0x31, 0x10, 0x01},		// SIGNAL-QZSS_L1S_ENA
    {0x25, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-GLO_ENA
    {0x18, 0x00, 0x31, 0x10, 0x00},		// SIGNAL-GLO_L1_ENA
    {0x21, 0x00, 0x11, 0x20, 0x03},		// NAVSPG-DYNMODEL
    {0x05, 0x00, 0x22, 0x20, 0x00},		// ODO-PROFILE
    {0xB5, 0x62, 0x06, 0x8A, 0x0A, 0x00, 0x01, 0x01, 0x00, 0x00, 0xB3, 0x00, 0x11, 0x30, 0x14, 0x00},	// NAVSPG-OUTFIL_PACC
    {0xBA, 0x00, 0x91, 0x20, 0x01},		// MSGOUT-NMEA_ID_GGA_I2C
    {0xBE, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GGA_SPI
    {0xBB, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GGA_UART1
    {0xAB, 0x00, 0x91, 0x20, 0x01},		// MSGOUT-NMEA_ID_RMC_I2C
    {0xAF, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_RMC_SPI
    {0xAC, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_RMC_UART1
    {0xC9, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GLL_I2C
    {0xCD, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GLL_SPI
    {0xCA, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GLL_UART1
    {0xBF, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GSA_I2C
    {0xC3, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GSA_SPI
    {0xC0, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GSA_UART1
    {0xC4, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GSV_I2C
    {0xC8, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GSV_SPI
    {0xC5, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_GSV_UART1
    {0xB0, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_VTG_I2C
    {0xB4, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_VTG_SPI
    {0xB1, 0x00, 0x91, 0x20, 0x00},		// MSGOUT-NMEA_ID_VTG_UART1
    {0x02, 0x00, 0xD0, 0x40, 0x3C, 0x00, 0x00, 0x00},	// PM-POSUPDATEPERIOD
    {0x03, 0x00, 0xD0, 0x40, 0x0A, 0x00, 0x00, 0x00},	// PM-ACQPERIOD
    {0x05, 0x00, 0xD0, 0x30, 0x05, 0x00},				// PM-ONTIME
    {0x06, 0x00, 0xD0, 0x20, 0x02},						// PM-MINACQTIME
    {0x07, 0x00, 0xD0, 0x20, 0x0A},						// PM-MAXACQTIME
    {0x09, 0x00, 0xD0, 0x10, 0x01},						// PM-WAITTIMEFIX
    {0x0C, 0x00, 0xD0, 0x10, 0x01},						// PM-EXTINTWAKE
    {0x01, 0x00, 0xD0, 0x20, 0x01}						// PM-OPERATEMODE
};


static const uint8_t *checksum[] = {
    {0xFC, 0x89},	// SIGNAL-GPS_ENA (RAM)
    {0xFD, 0x91},	// SIGNAL-GPS_ENA (BBR)
    {0xDE, 0xF3},	// SIGNAL-GPS_L1CA_ENA (RAM)
    {0xDF, 0xFB},	// SIGNAL-GPS_L1CA_ENA (BBR)
    {0xFD, 0x8E},	// SIGNAL-SBAS_ENA (RAM)
    {0xFE, 0x96},	// SIGNAL-SBAS_ENA (BBR)
    {0xE2, 0x07},	// SIGNAL-SBAS_L1CA_ENA (RAM)
    {0xE3, 0x0F},	// SIGNAL-SBAS_L1CA_ENA (BBR)
    {0xFD, 0x92},	// SIGNAL-GAL_ENA (RAM)
    {0xFE, 0x9A},	// SIGNAL-GAL_ENA (BBR)
    {0xE3, 0x10},	// SIGNAL-GAL_E1_ENA (RAM)
    {0xE4, 0x18},	// SIGNAL-GAL_E1_ENA (BBR)
    {0xFE, 0x97},	// SIGNAL-BDS_ENA (RAM)
    {0xFF, 0x9F},	// SIGNAL-BDS_ENA (BBR)
    {0xE9, 0x2E},	// SIGNAL-BDS_B1_ENA (RAM)
    {0xEA, 0x36},	// SIGNAL-BDS_B1_ENA (BBR)
    {0xEB, 0x38},	// SIGNAL-BDS_B1C_ENA (RAM)
    {0xEC, 0x40},	// SIGNAL-BDS_B1C_ENA (BBR)
    {0x01, 0xA2},	// SIGNAL-QZSS_ENA (RAM)
    {0x02, 0xAA},	// SIGNAL-QZSS_ENA (BBR)
    {0xEF, 0x48},	// SIGNAL-QZSS_L1CA_ENA (RAM)
    {0xF0, 0x50},	// SIGNAL-QZSS_L1CA_ENA (BBR)
    {0xF1, 0x52},	// SIGNAL-QZSS_L1S_ENA (RAM)
    {0xF2, 0x5A},	// SIGNAL-QZSS_L1S_ENA (BBR)
    {0x01, 0xA6},	// SIGNAL-GLO_ENA (RAM)
    {0x02, 0xAE},	// SIGNAL-GLO_ENA (BBR)
    {0xF4, 0x65},	// SIGNAL-GLO_L1_ENA (RAM)
    {0xF5, 0x6D},	// SIGNAL-GLO_L1_ENA (BBR)
    {0xF0, 0x55},	// NAVSPG-DYNMODEL (RAM)
    {0xF1, 0x5D},	// NAVSPG-DYNMODEL (BBR)
    {0xE2, 0xF9},	// ODO-PROFILE (RAM)
    {0xE3, 0x01},	// ODO-PROFILE (BBR)
    {0xA4, 0x0F},	// NAVSPG-OUTFIL_PACC (RAM)
    {0xA5, 0x18},	// NAVSPG-OUTFIL_PACC (BBR)
    {0x07, 0xD0},	// MSGOUT-NMEA_ID_GGA_I2C (RAM)
    {0x08, 0xD8},	// MSGOUT-NMEA_ID_GGA_I2C (BBR)
    {0x0A, 0xE3},	// MSGOUT-NMEA_ID_GGA_SPI (RAM)
    {0x0B, 0xEB},	// MSGOUT-NMEA_ID_GGA_SPI (BBR)
    {0x07, 0xD4},	// MSGOUT-NMEA_ID_GGA_UART1 (RAM)
    {0x08, 0xDC},	// MSGOUT-NMEA_ID_GGA_UART1 (BBR)
    {0xF8, 0x85},	// MSGOUT-NMEA_ID_RMC_I2C (RAM)
    {0xF9, 0x8D},	// MSGOUT-NMEA_ID_RMC_I2C (BBR)
    {0xFB, 0x98},	// MSGOUT-NMEA_ID_RMC_SPI (RAM)
    {0xFC, 0xA0},	// MSGOUT-NMEA_ID_RMC_SPI (BBR)
    {0xF8, 0x89},	// MSGOUT-NMEA_ID_RMC_UART1 (RAM)
    {0xF9, 0x91},	// MSGOUT-NMEA_ID_RMC_UART1 (BBR)
    {0x15, 0x1A},	// MSGOUT-NMEA_ID_GLL_I2C (RAM)
    {0x16, 0x22},	// MSGOUT-NMEA_ID_GLL_I2C (BBR)
    {0x19, 0x2E},	// MSGOUT-NMEA_ID_GLL_SPI (RAM)
    {0x1A, 0x36},	// MSGOUT-NMEA_ID_GLL_SPI (BBR)
    {0x16, 0x1F},	// MSGOUT-NMEA_ID_GLL_UART1 (RAM)
    {0x17, 0x27},	// MSGOUT-NMEA_ID_GLL_UART1 (BBR)
    {0x0B, 0xE8},	// MSGOUT-NMEA_ID_GSA_I2C (RAM)
    {0x0C, 0xF0},	// MSGOUT-NMEA_ID_GSA_I2C (BBR)
    {0x0F, 0xFC},	// MSGOUT-NMEA_ID_GSA_SPI (RAM)
    {0x10, 0x04},	// MSGOUT-NMEA_ID_GSA_SPI (BBR)
    {0x0C, 0xED},	// MSGOUT-NMEA_ID_GSA_UART1 (RAM)
    {0x0D, 0xF5},	// MSGOUT-NMEA_ID_GSA_UART1 (BBR)
    {0x10, 0x01},	// MSGOUT-NMEA_ID_GSV_I2C (RAM)
    {0x11, 0x09},	// MSGOUT-NMEA_ID_GSV_I2C (BBR)
    {0x14, 0x15},	// MSGOUT-NMEA_ID_GSV_SPI (RAM)
    {0x15, 0x1D},	// MSGOUT-NMEA_ID_GSV_SPI (BBR)
    {0x11, 0x06},	// MSGOUT-NMEA_ID_GSV_UART1 (RAM)
    {0x12, 0x0E},	// MSGOUT-NMEA_ID_GSV_UART1 (BBR)
    {0xFC, 0x9D},	// MSGOUT-NMEA_ID_VTG_I2C (RAM)
    {0xFD, 0xA5},	// MSGOUT-NMEA_ID_VTG_I2C (BBR)
    {0x00, 0xB1},	// MSGOUT-NMEA_ID_VTG_SPI (RAM)
    {0x01, 0xB9},	// MSGOUT-NMEA_ID_VTG_SPI (BBR)
    {0xFD, 0xA2},	// MSGOUT-NMEA_ID_VTG_UART1 (RAM)
    {0xFE, 0xAA},	// MSGOUT-NMEA_ID_VTG_UART1 (BBR)
    {0xEC, 0x55},	// PM-POSUPDATEPERIOD (RAM)
    {0xED, 0x60},	// PM-POSUPDATEPERIOD (BBR)
    {0xBB, 0x95},	// PM-ACQPERIOD (RAM)
    {0xBC, 0xA0},	// PM-ACQPERIOD (BBR)
    {0xA6, 0xD9},	// PM-ONTIME (RAM)
    {0xA7, 0xE2},	// PM-ONTIME (BBR)
    {0x93, 0x0A},	// PM-MINACQTIME (RAM)
    {0x94, 0x12},	// PM-MINACQTIME (BBR)
    {0x9C, 0x17},	// PM-MAXACQTIME (RAM)
    {0x9D, 0x1F},	// PM-MAXACQTIME (BBR)
    {0x85, 0xF8},	// PM-WAITTIMEFIX (RAM)
    {0x86, 0x00},	// PM-WAITTIMEFIX (BBR)
    {0x88, 0x07},	// PM-EXTINTWAKE (RAM)
    {0x89, 0x0F},	// PM-EXTINTWAKE (BBR)
    {0x8D, 0xF0},	// PM-OPERATEMODE (RAM)
    {0x8E, 0xF8}	// PM-OPERATEMODE (BBR)
};



/*
uint8_t payload[] = {~09 00 (Length)~, 01(Version 00 o 01), 01 (Layer RAM), 00 00 (RESERVED),
 	 	 	 	 	 1F 00 31 10 (KEI ID CFG-SIGNAL), 01 (Value), FC (shecksum A), 89 (Checksum B)};
*/

typedef struct msgGPS {
	uint8_t header[2];
	uint8_t msgClass;
	uint8_t msgID;
	uint16_t length;		// Payload length from without checksum bytes.
	uint8_t version;
	uint8_t layer;
	uint16_t reserved;
	uint32_t keyID;
	std::vector<uint8_t> value;	// NACHO: Creo que al hacerlo así se hace automática la gestión de memoria.
	uint8_t checksum[2];
} msgGPS_t;


// Create msgGPS_t structure.
void gps_create_message(msgGPS_t *msg, uint16_t length, uint16_t layer, uint8_t *data, uint8_t *checksum);

// Cast msg to uint8_t num_byte of times.
template <typename T>
std::vector<uint8_t> to_bytes(const T &msg, size_t num_bytes);

// Concatenate bytes and form a single message
std::vector<uint8_t> build_ubx_message(msgGPS_t *msg);

// Configure GPS
void configure_gps();




// NACHO: CREO QUE NO ES NECESARIO. YA LOS TENEMOS DEL UCENTER-2, NO?
/*
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
*/






#endif /* MODULES_CONFIGGPS_CONFIGGPS_H_ */
