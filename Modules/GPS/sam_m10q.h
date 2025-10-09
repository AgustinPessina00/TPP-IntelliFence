#ifndef MODULES_GPS_SAM_M10Q_H_
#define MODULES_GPS_SAM_M10Q_H_

#include <vector>				// Ahora lo agrego acá, ya que lo quité de sam_m10q_KEYID.h
#include "sam_m10q_KEYID.h"
#include "TinyGPSPlus/TinyGPS++.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_i2c.h"

// UBX/VALSET defs
#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A
#define VALSET_VERSION 0x00 // 0x00 = transactionless, 0x01 = w/ transactions
#define RAM 0x01
#define BBR 0x02
#define RESERVED 0x0000

#define NMEA_BUFFER_SIZE	16

enum class gpsRateSpeed {
  STOP,
  SLOW,
  MEDIUM,
  FAST
};

class SamM10q {
public:
	SamM10q(I2C_HandleTypeDef *hi2c, uint8_t i2cAddr);

	// Llamar explícitamente luego de HAL_Init() y MX_I2C_Init()
    void initSamM10q();

	HAL_StatusTypeDef read_nmea_stream();
	HAL_StatusTypeDef read_gps_position();
	void update_location_and_time();
	bool set_new_acq_time(gpsRateSpeed gpsRate);

	// TEST
	void testGPS();

private:
	void configure_gps();

	// Armado de mensajes UBX sin copiar vectores globales
    std::vector<uint8_t> build_full_message_from_index(size_t i, uint8_t layer);
    std::vector<uint8_t> build_ubx_message(uint8_t layer, const uint8_t* payload, size_t payload_len, const uint8_t* ck, size_t ck_len);

	HAL_StatusTypeDef send_message(const std::vector<uint8_t> &message, uint32_t delay_ms);	//TODO: Chequear uint32_t
	void ubx_calculate_checksum(std::vector<uint8_t> msg);

public:
	double latitude = -34.570386;
	double longitude = -58.444212;
	uint32_t fechaUTC = 0; //yymmdd	//TODO: Chequear uint32_t
	uint32_t horaUTC = 0; //hhmmss	//TODO: Chequear uint32_t

private:
    uint8_t i2cAddr;
	TinyGPSPlus trackerGPS;

	uint8_t header[2];
	uint8_t msgClass;
	uint8_t msgID;
	//uint16_t length;	Es fijo, lo conocemos del KEYID.
	uint8_t version;
	uint16_t reserved;

	I2C_HandleTypeDef *hi2c;
};

#endif /* MODULES_GPS_SAM_M10Q_H_ */
