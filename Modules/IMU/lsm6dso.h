
#ifndef MODULES_IMU_LSM6DSO_H_
#define MODULES_IMU_LSM6DSO_H_

#include "lsm6dso_registers.h"

// === Máscaras de campos de los registros ===
#define LSM6DSO_ODR_XL_MASK     (0b1111 << ODR_XL3)
#define LSM6DSO_FS_XL_MASK		(0b11 	<< FS1_XL)
#define LSM6DSO_ODR_G_MASK		(0b1111 << ODR_G3)
#define LSM6DSO_FS_G_MASK		(0b11 	<< FS1_G)
#define LSM6DSO_INACT_MASK		(0b11 	<< INACT_EN1)
#define LSM6DSO_WK_THS_MASK		(0b111111 << WK_THS5)
#define LSM6DSO_WK_DUR_MASK		(0b11 	<< WK_DUR1)
#define LSM6DSO_WK_DUR_MASK		(0b1 	<< WAKE_THS_W)
#define LSM6DSO_SLEEP_DUR_MASK	(0b1111 << SLEEP_DUR3)


enum class Lsm6dsoOdrAcc : uint16_t {
	POWER_DOWN	= (0b0000 << ODR_XL3),
    ODR_1_6		= (0b1011 << ODR_XL3),
	ODR_12_5   	= (0b0001 << ODR_XL3),
	ODR_26   	= (0b0010 << ODR_XL3),
	ODR_52  	= (0b0011 << ODR_XL3),
	ODR_104  	= (0b0100 << ODR_XL3),
	ODR_208  	= (0b0101 << ODR_XL3),
	ODR_416 	= (0b0110 << ODR_XL3),
	ODR_833 	= (0b0111 << ODR_XL3),
	ODR_1K66 	= (0b1000 << ODR_XL3),
	ODR_3K33 	= (0b1001 << ODR_XL3),
	ODR_6K66 	= (0b1010 << ODR_XL3)
};

enum class Lsdm6dsoFsAcc : uint16_t {
    FS_2G		= (0b00 << FS1_XL),
    FS_16G		= (0b01 << FS1_XL),
    FS_4G   	= (0b10 << FS1_XL),
    FS_8G   	= (0b11 << FS1_XL)
};

enum class Lsm6dsoOdrGyr : uint16_t {
	POWER_DOWN	= (0b0000 << ODR_G3),
	ODR_12_5   	= (0b0001 << ODR_G3),
	ODR_26   	= (0b0010 << ODR_G3),
	ODR_52  	= (0b0011 << ODR_G3),
	ODR_104  	= (0b0100 << ODR_G3),
	ODR_208  	= (0b0101 << ODR_G3),
	ODR_416 	= (0b0110 << ODR_G3),
	ODR_833 	= (0b0111 << ODR_G3),
	ODR_1K66 	= (0b1000 << ODR_G3),
	ODR_3K33 	= (0b1001 << ODR_G3),
	ODR_6K66 	= (0b1010 << ODR_G3)
};

enum class Lsdm6dsoFsGyr : uint16_t {
    FS_250DPS		= (0b00 << FS1_G),
    FS_500DPS		= (0b01 << FS1_G),
    FS_1KDPS	  	= (0b10 << FS1_G),
    FS_2KDPS	   	= (0b11 << FS1_G)
};

enum class Lsm6dsoWakeThs : uint8_t {
    THS_DISABLE		= (0b000000  << WK_THS_5),
    THS_1 			= (0b000001  << WK_THS_5),
    THS_2 			= (0b000010  << WK_THS_5),
	THS_3 			= (0b000011  << WK_THS_5),
	THS_4 			= (0b000100  << WK_THS_5),
	THS_5 			= (0b000101  << WK_THS_5),
	THS_6 			= (0b000110  << WK_THS_5),
	THS_7 			= (0b000111  << WK_THS_5),
    THS_8 			= (0b001000  << WK_THS_5),
	THS_9 			= (0b001001  << WK_THS_5),
	THS_10 			= (0b001010  << WK_THS_5),
	THS_11 			= (0b001011  << WK_THS_5),
	THS_12 			= (0b001100  << WK_THS_5),
	THS_13 			= (0b001101  << WK_THS_5),
	THS_14 			= (0b001110  << WK_THS_5),
	THS_15 			= (0b001111  << WK_THS_5),
    THS_16 			= (0b010000 << WK_THS_5),
    THS_17 			= (0b010001  << WK_THS_5),
    THS_18 			= (0b010010  << WK_THS_5),
	THS_19 			= (0b010011  << WK_THS_5),
	THS_20 			= (0b010100  << WK_THS_5),
	THS_21 			= (0b010101  << WK_THS_5),
	THS_22 			= (0b010110  << WK_THS_5),
	THS_23 			= (0b010111  << WK_THS_5),
    THS_24 			= (0b011000  << WK_THS_5),
	THS_25 			= (0b011001  << WK_THS_5),
	THS_26 			= (0b011010  << WK_THS_5),
	THS_27 			= (0b011011  << WK_THS_5),
	THS_28 			= (0b011100  << WK_THS_5),
	THS_29 			= (0b011101  << WK_THS_5),
	THS_30 			= (0b011110  << WK_THS_5),
	THS_31 			= (0b011111  << WK_THS_5),
    THS_32 			= (0b100000 << WK_THS_5),
    THS_33 			= (0b100001  << WK_THS_5),
    THS_34 			= (0b100010  << WK_THS_5),
	THS_35 			= (0b100011  << WK_THS_5),
	THS_36 			= (0b100100  << WK_THS_5),
	THS_37 			= (0b100101  << WK_THS_5),
	THS_38 			= (0b100110  << WK_THS_5),
	THS_39 			= (0b100111  << WK_THS_5),
    THS_40 			= (0b101000  << WK_THS_5),
	THS_41 			= (0b101001  << WK_THS_5),
	THS_42 			= (0b101010  << WK_THS_5),
	THS_43 			= (0b101011  << WK_THS_5),
	THS_44 			= (0b101100  << WK_THS_5),
	THS_45 			= (0b101101  << WK_THS_5),
	THS_46			= (0b101110  << WK_THS_5),
	THS_47 			= (0b101111  << WK_THS_5),
    THS_48 			= (0b110000 << WK_THS_5),
    THS_49 			= (0b110001  << WK_THS_5),
    THS_50 			= (0b110010  << WK_THS_5),
	THS_51 			= (0b110011  << WK_THS_5),
	THS_52 			= (0b110100  << WK_THS_5),
	THS_53 			= (0b110101  << WK_THS_5),
	THS_54			= (0b110110  << WK_THS_5),
	THS_55 			= (0b110111  << WK_THS_5),
    THS_56 			= (0b111000  << WK_THS_5),
	THS_57 			= (0b111001  << WK_THS_5),
	THS_58 			= (0b111010  << WK_THS_5),
	THS_59 			= (0b111011  << WK_THS_5),
	THS_60 			= (0b111100  << WK_THS_5),
	THS_61 			= (0b111101  << WK_THS_5),
	THS_62 			= (0b111110  << WK_THS_5),
	THS_63 			= (0b111111  << WK_THS_5)
};

enum class Lsdm6dsoWakeDur : uint16_t {
	ODR_0		= (0b00 << WK_DUR1),
    ODR_1		= (0b01 << WK_DUR1),
    ODR_2	  	= (0b10 << WK_DUR1),
    ODR_3   	= (0b11 << WK_DUR1)
};

enum class Lsdm6dsoWakeWeight : uint16_t {
	FS_2_6		= (0b0 << WK_THS_W),
    FS_2_8		= (0b1 << WK_THS_W)
};

//CHEQUEAR ESTOS VALORES... NO SÉ DE DONDE LOS SACÓ CHATTY.
enum class Lsm6dsoSleepDur : uint8_t {
    DUR_16_ODR   = (0b0000 << SLEEP_DUR3),
    DUR_32_ODR   = (0b0001 << SLEEP_DUR3),
    DUR_48_ODR   = (0b0010 << SLEEP_DUR3),
    DUR_64_ODR   = (0b0011 << SLEEP_DUR3),
    DUR_80_ODR   = (0b0100 << SLEEP_DUR3),
    DUR_96_ODR   = (0b0101 << SLEEP_DUR3),
    DUR_112_ODR  = (0b0110 << SLEEP_DUR3),
    DUR_128_ODR  = (0b0111 << SLEEP_DUR3),
    DUR_144_ODR  = (0b1000 << SLEEP_DUR3),
    DUR_160_ODR  = (0b1001 << SLEEP_DUR3),
    DUR_176_ODR  = (0b1010 << SLEEP_DUR3),
    DUR_192_ODR  = (0b1011 << SLEEP_DUR3),
    DUR_208_ODR  = (0b1100 << SLEEP_DUR3),
    DUR_224_ODR  = (0b1101 << SLEEP_DUR3),
    DUR_240_ODR  = (0b1110 << SLEEP_DUR3),
    DUR_256_ODR  = (0b1111 << SLEEP_DUR3)
};


class Lsm6dso {
public:
	Lsdm6dso(uint8_t i2cAddr, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr, Lsm6dsoWakeThs wakeThs, Lsdm6dsoWakeDur wakeDur, Lsdm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur);

private:
    uint8_t setConfiguration(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode);
    bool writeRegister(uint8_t reg, uint16_t value);

private:
    uint8_t i2cAddr;
    int odrAcc;
    int fsAcc;
    int odrGyr;
	int fsGyr;
	int wakeThs;
	int wakeDur;
	int wakeWeight;
	int sleepDur;

};


#endif /* MODULES_IMU_LSM6DSO_H_ */
