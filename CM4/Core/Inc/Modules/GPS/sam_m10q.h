#ifndef MODULES_GPS_SAM_M10Q_H_
#define MODULES_GPS_SAM_M10Q_H_

#include "sam_m10q_KEYID.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_def.h"
#include "stm32wlxx_hal_i2c.h"

// Forward declaration para evitar dependencias circulares
class I2CBus;
class UARTBus;

// Tamaño máximo de mensaje UBX (header + class + id + length + payload + checksum)
#define UBX_MAX_MESSAGE_SIZE 32

// Tamaño del Key ID (4 bytes)
#define UBX_KEYID_SIZE 4

// UBX defs
#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62

// UBX-CFG-VALSET
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A

// UBX-CFG-VALGET
#define VALGET_CLASS 0x06
#define VALGET_ID 0x8B

// UBX-NAV-PVT
#define NAV_CLASS 0x01
#define PVT_ID 0x07

// UBX-ACK
#define ACK_CLASS 0x05
#define ACK_ACK_ID 0x01
#define ACK_NAK_ID 0x00

// Estructura para almacenar datos PVT (Position, Velocity, Time)
// Basada en la especificación UBX-NAV-PVT del protocolo u-blox
typedef struct {
    uint32_t iTOW;          // GPS time of week [ms]
    uint16_t year;          // Year (UTC)
    uint8_t month;          // Month [1..12] (UTC)
    uint8_t day;            // Day of month [1..31] (UTC)
    uint8_t hour;           // Hour [0..23] (UTC)
    uint8_t min;            // Minute [0..59] (UTC)
    uint8_t sec;            // Second [0..60] (UTC)
    uint8_t valid;          // Validity flags
    uint32_t tAcc;          // Time accuracy estimate [ns]
    int32_t nano;           // Fraction of second [-1e9..1e9] [ns]
    uint8_t fixType;        // GNSSfix Type (0=no fix, 3=3D fix)
    uint8_t flags;          // Fix status flags
    uint8_t flags2;         // Additional flags
    uint8_t numSV;          // Number of satellites used
    int32_t lon;            // Longitude [deg * 1e-7]
    int32_t lat;            // Latitude [deg * 1e-7]
    int32_t height;         // Height above ellipsoid [mm]
    int32_t hMSL;           // Height above mean sea level [mm]
    uint32_t hAcc;          // Horizontal accuracy estimate [mm]
    uint32_t vAcc;          // Vertical accuracy estimate [mm]
    int32_t velN;           // NED north velocity [mm/s]
    int32_t velE;           // NED east velocity [mm/s]
    int32_t velD;           // NED down velocity [mm/s]
    int32_t gSpeed;         // Ground Speed [mm/s]
    int32_t headMot;        // Heading of motion [deg * 1e-5]
    uint32_t sAcc;          // Speed accuracy estimate [mm/s]
    uint32_t headAcc;       // Heading accuracy estimate [deg * 1e-5]
    uint16_t pDOP;          // Position DOP [* 0.01]
	uint16_t flags3;        // Additional flags
    uint8_t reserved1[4];   // Reserved
    int32_t headVeh;        // Heading of vehicle [deg * 1e-5]
    int16_t magDec;         // Magnetic declination [deg * 1e-2]
    uint16_t magAcc;        // Magnetic declination accuracy [deg * 1e-2]
} UBX_NAV_PVT_data_t;

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
	// Default constructor - does NOT access hardware
	SamM10q();

	// Must be called explicitly after HAL_Init() and MX_I2C_Init()
	bool init(uint8_t i2cAddr);

    HAL_StatusTypeDef read_gps_position();
	bool update_location_and_time();
	bool set_new_acq_time(gpsRateSpeed gpsRate);

	// TEST
	void testGPS();

	// Escritura y lectura de registros UBX (usa I2C por defecto)
	bool write_register(const uint8_t* payload_data, size_t payload_len, uint8_t layer);

	bool read_configuration(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer); //Leo la configuración de un KeyID con VALGET via I2C.

	// Funciones específicas para UART (configuración inicial)
	bool write_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t layer);

	bool read_register_uart(const uint8_t* payload_data, size_t payload_len, uint8_t* response_buffer, uint16_t buffer_size, uint8_t layer); //Leo la configuración de un KeyID con VALGET via UART.

	void configure_gps_uart();  // Configuración inicial via UART

private:
	void configure_gps();
	void configure_all_registers(const M10QPayload configPayloads[], size_t numPayloads);

	// Armado de mensajes UBX usando arrays estáticos (embedded friendly)
	uint16_t build_ubx_message(uint8_t msgClass, uint8_t msgID, uint8_t layer, const uint8_t* payload_data, size_t payload_len, uint8_t* buffer, uint16_t buffer_size);
	HAL_StatusTypeDef send_message(const uint8_t* message, uint16_t message_len, uint32_t delay_ms);
	HAL_StatusTypeDef send_message_uart(const uint8_t* message, uint16_t message_len, uint32_t delay_ms);
	void ubx_calculate_checksum(const uint8_t* msg, uint16_t msg_len, uint8_t* ck_a, uint8_t* ck_b);

	// ====== NUEVAS FUNCIONES PRIVADAS PARA PVT ======
    bool getPVT(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs = 1000);
    bool requestPVT();
    bool receivePVT(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs);
    bool receivePVTValidateOption(UBX_NAV_PVT_data_t* pvtData, uint32_t maxWaitMs); //Esta función hace varias validaciones para evitar el ruido de tramas NMEA. Solo debemos usarla si no eliminamos los mensajes NMEA.
    bool parseUBXMessage(const uint8_t* buffer, uint16_t bufferLen, UBX_NAV_PVT_data_t* pvtData);
    bool verifyUBXChecksum(const uint8_t* buffer, uint16_t msgLen);

	// ====== FUNCIONES PRIVADAS PARA VERIFICACION VALGET ======
	void flush_uart_buffer(uint32_t timeout_ms = 500);
	bool verify_config_with_valget(const uint8_t* payload_data, size_t payload_len, uint8_t layer);
	bool parse_valget_response(const uint8_t* response_buffer, uint16_t buffer_len, const uint8_t* key_id, const uint8_t* expected_value, uint8_t value_size);
	int8_t check_ack_response(const uint8_t* response_buffer, uint16_t buffer_len, uint8_t expected_class, uint8_t expected_id);

public:
	float latitude = 0;
	float longitude = 0;
	uint32_t fechaUTC = 0; //yymmdd	//TODO: Chequear uint32_t
	uint32_t horaUTC = 0; //hhmmss	//TODO: Chequear uint32_t
    uint8_t flags;

private:
    uint8_t i2cAddr;
	bool initialized;

	//uint16_t length;	Es fijo, lo conocemos del KEYID.
	uint8_t version;
	uint16_t reserved;

	I2CBus* i2cBus;             // Bus I2C thread-safe
	UARTBus* uartBus;           // Bus UART thread-safe para configuración inicial
};

#endif /* MODULES_GPS_SAM_M10Q_H_ */
