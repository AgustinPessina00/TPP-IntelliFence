#ifndef MODULES_GPS_SAM_M10Q_H_
#define MODULES_GPS_SAM_M10Q_H_

#include "sam_m10q_KEYID.h"
#inclde "TinyGPSPlus/TinyGPS++.h"

#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A
#define VALSET_VERSION 0x00 // 0x00 = transactionless, 0x01 = w/ transactions
#define RAM 0x01
#define BBR 0x02
#define RESERVED 0x0000

#define NMEA_BUFFER_SIZE	64

class SamM10q {
public:
	SamM10q(uint8_t i2cAddr, TinyGPSPlus *trackerGPS);

	void read_nmea_stream(I2C_HandleTypeDef *hi2c);
	void update_location_and_time();

private:
	void configure_gps();
	std::vector<uint8_t> build_full_message_from_index(size_t i);
	std::vector<uint8_t> build_ubx_message(uint8_t layer, const std::vector<uint8_t> payload, const std::vector<uint8_t> checksum);
	HAL_StatusTypeDef send_message(I2C_HandleTypeDef *hi2c, const std::vector<uint8_t> &message, uitn32_t delay_ms);
	void ubx_calculate_checksum(UBX_Message *msg)

public:
	double latitude = 0.0;
	double longitude = 0.0;
	uint32_t fechaUTC = 0; //yymmdd
	uint32_t horaUTC = 0; //hhmmss

private:
    uint8_t i2cAddr;
	TinyGPSPlus *trackerGPS;
	uint8_t header[2];
	uint8_t msgClass;
	uint8_t msgID;
//	uint16_t length;	Ahora que tenemos el KEYID en un vector podemos calcularlo haciendo m10q_data.size().
	uint8_t version;
	uint16_t reserved;
};

#endif /* MODULES_GPS_SAM_M10Q_H_ */
