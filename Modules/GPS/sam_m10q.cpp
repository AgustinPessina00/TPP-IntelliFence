/* Includes ------------------------------------------------------------------*/

#include "sam_m10q.h"
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/

SamM10q::SamM10q(uint8_t i2cAddr) {
	this->i2cAddr = i2cAddr;
}

void SamM10q::create_msgGPS(msgGPS_t *msg, uint16_t length, uint16_t layer, uint8_t *data, uint8_t *checksum) {

}

void SamM10q::append_big_endian(vector<uint8_t> *message, const T *value) {

}

vector<uint8_t> SamM10q::build_ubx_message(msgGPS_t *msg) {

}

void SamM10q::configure_gps() {

}

void SamM10q::ubx_calculate_checksum(UBX_Message *msg) {

}


