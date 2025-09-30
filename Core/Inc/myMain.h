
#ifndef INC_MYMAIN_H_
#define INC_MYMAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

void RunCppApplication();

static const uint8_t GPS_ADDRESS = 0x84;	// 0x42 << 1 // GPS 8-bit Address.
//static const uint8_t GPS_ADDRESS = 0xD3;
static const uint8_t IMU_ADDRESS = 0xD4;	// 0x6A << 1 // IMU 8-bit Address.
// TODO: Cambiar addresses de INA
static const uint8_t INA_MCU_ADDRESS = 0xD6;
static const uint8_t INA_GPS_ADDRESS = 0xD8;
static const uint8_t INA_IMU_ADDRESS = 0xDA;

#ifdef __cplusplus
}
#endif

#endif /* INC_MYMAIN_H_ */
