
#ifndef MODULES_GPS_SAM_M10Q_H_
#define MODULES_GPS_SAM_M10Q_H_

#include "sam_m10q_KEYID.h"

#define UBX_HEADER1  0xB5
#define UBX_HEADER2  0x62
#define VALSET_CLASS 0x06
#define VALSET_ID 0x8A
#define VALSET_VERSION 0x00 // 0x00 = transactionless, 0x01 = w/ transactions
#define RAM 0x01
#define BBR 0x02
#define RESERVED 0x0000
#define LENGTH_PAYLOAD_OFFSET 8	// Length - sizeof(version) - sizeof(layer) - sizeof(reserved) - sizeof(keyID)

class SamM10q {
public:
	SamM10q(uint8_t i2cAddr);

private:
	void create_msgGPS(msgGPS_t *msg, uint16_t length, uint16_t layer, uint8_t *data, uint8_t *checksum);
	void append_big_endian(vector<uint8_t> *message, const T *value);
	vector<uint8_t> build_ubx_message(msgGPS_t *msg);
	void configure_gps();
	void ubx_calculate_checksum(UBX_Message *msg);
private:
    uint8_t i2cAddr;

};

/*
typedef struct msgGPS {

	uint8_t header[2];
	uint8_t msgClass;
	uint8_t msgID;
	uint16_t length;		// Payload length from without checksum bytes.
	uint8_t version;
	uint8_t layer;
	uint16_t reserved;
	uint32_t keyID;
	uint8_t value[length - LENGTH_PAYLOAD_OFFSET];
	uint8_t checksum[2];
} msgGPS_t;
*/

#endif /* MODULES_GPS_SAM_M10Q_H_ */
