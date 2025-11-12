#ifndef MODULES_GPS_SAM_M10Q_H_
#define MODULES_GPS_SAM_M10Q_H_

#include "sam_m10q_KEYID.h"
#include "TinyGPSPlus/TinyGPS++.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_i2c.h"

// Forward declaration para evitar dependencias circulares
class I2CBus;
class UARTBus;

// Tamaño máximo de mensaje UBX (header + class + id + length + payload + checksum)
#define UBX_MAX_MESSAGE_SIZE 32

// UBX defs
#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62

// UBX-CFG-VALSET
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A

// UBX-CFG-VALGET
#define VALGET_CLASS 0x06
#define VALGET_ID 0x8B

#define VALSET_VERSION 0x00 // 0x00 = transactionless, 0x01 = w/ transactions
#define RAM 0x01
#define BBR 0x02
#define FLASH_ 0x04
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
	SamM10q(uint8_t i2cAddr);

	// Llamar explícitamente luego de HAL_Init() y MX_I2C_Init()
    void initSamM10q();

	HAL_StatusTypeDef read_nmea_stream();
	HAL_StatusTypeDef read_gps_position();
	void update_location_and_time();
	bool set_new_acq_time(gpsRateSpeed gpsRate);

	// TEST
	void testGPS();

	// Escritura y lectura de registros UBX (usa I2C por defecto)
	bool write_register(const uint8_t* payload_data, size_t payload_len, uint8_t layer);
	bool read_register(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer);

	// Funciones específicas para UART (configuración inicial)
	bool write_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t layer);
	bool read_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer);
	void configure_gps_uart();  // Configuración inicial via UART

private:
	void configure_gps();

	// Armado de mensajes UBX usando arrays estáticos (embedded friendly)
	uint16_t build_ubx_message(uint8_t msgClass, uint8_t msgID, uint8_t layer, const uint8_t* payload_data, size_t payload_len, uint8_t* buffer, uint16_t buffer_size);
	HAL_StatusTypeDef send_message(const uint8_t* message, uint16_t message_len, uint32_t delay_ms);
	HAL_StatusTypeDef send_message_uart(const uint8_t* message, uint16_t message_len, uint32_t delay_ms);
	void ubx_calculate_checksum(const uint8_t* msg, uint16_t msg_len, uint8_t* ck_a, uint8_t* ck_b);

public:
	double latitude = 0;
	double longitude = 0;
	uint32_t fechaUTC = 0; //yymmdd	//TODO: Chequear uint32_t
	uint32_t horaUTC = 0; //hhmmss	//TODO: Chequear uint32_t

private:
    uint8_t i2cAddr;
	TinyGPSPlus trackerGPS;

	//uint16_t length;	Es fijo, lo conocemos del KEYID.
	uint8_t version;
	uint16_t reserved;

	I2CBus* i2cBus;             // Bus I2C thread-safe
	UARTBus* uartBus;           // Bus UART thread-safe para configuración inicial
};

#endif /* MODULES_GPS_SAM_M10Q_H_ */
