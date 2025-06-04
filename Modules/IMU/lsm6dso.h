
#ifndef MODULES_IMU_LSM6DSO_H_
#define MODULES_IMU_LSM6DSO_H_

#include "lsm6dso_registers.h"

// === Máscaras de campos de los registros ===
#define LSM6DSO_ODR_XL_MASK     	(0b1111 	<< ODR_XL3)		// CTRL1_XL (0x10)
#define LSM6DSO_FS_XL_MASK			(0b11 		<< FS1_XL)		// CTRL1_XL (0x10)
#define LSM6DSO_ODR_G_MASK			(0b1111 	<< ODR_G3)		// CTRL2_G (0x11)
#define LSM6DSO_FS_G_MASK			(0b11 		<< FS1_G)		// CTRL2_G (0x11)
#define LSM6DSO_IF_INC_MASK			(0b1 		<< IF_INC)		// CTRL3_C (0x12)
#define LSM6DSO_XL_HM_MODE_MASK		(0b1		<< XL_HM_MODE)	// CTRL6_C (0x15)
#define LSM6DSO_G_HM_MODE_MASK		(0b1		<< G_HM_MODE)	// CTRL7_G (0x16)
#define LSM6DSO_I3C_DISABLE_MASK	(0b1 		<< I3C_DISABLE)	// CTRL9_XL (0x18)
#define LSM6DSO_SLOPE_FDS_MASK		(0b1		<< SLOPE_FDS)	// TAP_CFG0 (0x56)
#define LSM6DSO_INT_EN_MASK			(0b1		<< INT_EN)		// TAP_CFG2 (0x58)
#define LSM6DSO_INACT_MASK			(0b11 		<< INACT_EN1)	// TAP_CFG2 (0x58)
#define LSM6DSO_WK_THS_MASK			(0b111111	<< WK_THS5)		// WAKE_UP_THS (0x5B)
#define LSM6DSO_WK_DUR_MASK			(0b11 		<< WK_DUR1)		// WAKE_UP_DUR (0x5C)
#define LSM6DSO_WK_DUR_THS_MASK		(0b1 		<< WAKE_THS_W)	// WAKE_UP_DUR (0x5C)
#define LSM6DSO_WK_DUR_SLP_MASK		(0b1111 	<< SLEEP_DUR3)	// WAKE_UP_DUR (0x5C)
#define LSM6DSO_INT1_WU_MASK		(0b1		<< INT1_WU)		// MD1_CFG (0x5E)

// === Constantes ===
#define LSM6DSO_WORD_SHIFT		8U     // To shift low byte into position


// CTRL1_XL (0x10)
enum class Lsm6dsoOdrAcc : uint8_t {
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

// CTRL1_XL (0x10)
enum class Lsm6dsoFsAcc : uint8_t {
    FS_2G		= (0b00 << FS1_XL),
    FS_16G		= (0b01 << FS1_XL),
    FS_4G   	= (0b10 << FS1_XL),
    FS_8G   	= (0b11 << FS1_XL)
};

// CTRL2_G (0x11)
enum class Lsm6dsoOdrGyr : uint8_t {
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

// CTRL2_G (0x11)
enum class Lsm6dsoFsGyr : uint8_t {
    FS_250DPS		= (0b00 << FS1_G),
    FS_500DPS		= (0b01 << FS1_G),
    FS_1KDPS	  	= (0b10 << FS1_G),
    FS_2KDPS	   	= (0b11 << FS1_G)
};

// CTRL3_C (0x12)
enum class Lsm6dsoIfInc : uint8_t {
	DISABLED	= (0b0 << IF_INC),
	ENABLED		= (0b1 << IF_INC)
};

// CTRL6_C (0x15)
enum class Lsm6dsoXlHm : uint8_t {
	ENABLED		= (0b0 << XL_HM_MODE),
	DISABLED	= (0b1 << XL_HM_MODE)
};

// CTRL7_G (0x16)
enum class Lsm6dsoGHm : uint8_t {
	ENABLED		= (0b0 << G_HM_MODE),
	DISABLED	= (0b1 << G_HM_MODE)
};

// CTRL9_XL (0x18)
enum class Lsm6dsoI3C : uint8_t {
	ENABLED		= (0b0 << I3C_DISABLE),
	DISABLED	= (0b1 << I3C_DISABLE)
};

// TAP_CFG0 (0x56)
enum class Lsm6dsoSlopeFilterEn : uint8_t {
	SLOPE_FILTER	= (0b0 << SLOPE_FDS),
	HPF				= (0b1 << SLOPE_FDS)
};

// TAP_CFG2 (0x58)
enum class Lsm6dsoIntEn : uint8_t {
	DISABLED	= (0b0 << INT_EN),
	ENABLED		= (0b1 << INT_EN)
};

// TAP_CFG2 (0x58)
enum class Lsm6dsoInActEn : uint8_t {
    XL_G_NOCHANGE	= (0b00 << INACT_EN1),
    G_NOCHANGE		= (0b01 << INACT_EN1),
    G_SLEEP	  		= (0b10 << INACT_EN1),
    G_POWERDOWN	   	= (0b11 << INACT_EN1)
};

// WAKE_UP_THS (0x5B)
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
    THS_16 			= (0b010000  << WK_THS_5),
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
    THS_32 			= (0b100000  << WK_THS_5),
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
    THS_48 			= (0b110000  << WK_THS_5),
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

// WAKE_UP_DUR (0x5C)
enum class Lsm6dsoWakeDur : uint8_t {
	ODR_0		= (0b00 << WK_DUR1),
    ODR_1		= (0b01 << WK_DUR1),
    ODR_2	  	= (0b10 << WK_DUR1),
    ODR_3   	= (0b11 << WK_DUR1)
};

// WAKE_UP_DUR (0x5C)
enum class Lsm6dsoWakeWeight : uint8_t {
	FS_XL_2_6		= (0b0 << WK_THS_W),
    FS_XL_2_8		= (0b1 << WK_THS_W)
};

//CHEQUEAR ESTOS VALORES... NO SÉ DE DONDE LOS SACÓ CHATTY.
//PESSI: Te confirmo que esos valores no son correctos. Según la hoja de datos, SLEEP_DUR[3:0] representa múltiplos de 512 ODR_time, no de 16 ODR
//como ahí dice.
/*
 enum class Lsm6dsoSleepDur : uint8_t {
    DUR_0_512   = (0b0000 << SLEEP_DUR3), // 0 * 512 * ODR_time = deshabilitado
    DUR_1_512   = (0b0001 << SLEEP_DUR3),
    DUR_2_512   = (0b0010 << SLEEP_DUR3),
    DUR_3_512   = (0b0011 << SLEEP_DUR3),
    DUR_4_512   = (0b0100 << SLEEP_DUR3),
    DUR_5_512   = (0b0101 << SLEEP_DUR3),
    DUR_6_512   = (0b0110 << SLEEP_DUR3),
    DUR_7_512   = (0b0111 << SLEEP_DUR3),
    DUR_8_512   = (0b1000 << SLEEP_DUR3),
    DUR_9_512   = (0b1001 << SLEEP_DUR3),
    DUR_10_512  = (0b1010 << SLEEP_DUR3),
    DUR_11_512  = (0b1011 << SLEEP_DUR3),
    DUR_12_512  = (0b1100 << SLEEP_DUR3),
    DUR_13_512  = (0b1101 << SLEEP_DUR3),
    DUR_14_512  = (0b1110 << SLEEP_DUR3),
    DUR_15_512  = (0b1111 << SLEEP_DUR3)
};
 */

// WAKE_UP_DUR (0x5C)
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

// MD1_CFG (0x5E)
enum class Lsm6dsoIntWU : uint8_t {
	DISABLED	= (0b0 << INT1_WU),
	ENABLED		= (0b1 << INT1_WU)
};


// === OUTPUTS ===

typedef int32_t (*stmdev_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*stmdev_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);
typedef void (*stmdev_mdelay_ptr)(uint32_t millisec);

typedef struct
{
  /** Component mandatory fields **/
  stmdev_write_ptr  write_reg;
  stmdev_read_ptr   read_reg;
  /** Component optional fields **/
  stmdev_mdelay_ptr   mdelay;
  /** Customizable optional pointer **/
  void *handle;
} stmdev_ctx_t;

typedef enum
{
  LSM6DSO_XL_UI_OFF       = 0x00, /* in power down */
  LSM6DSO_XL_UI_1Hz6_LP   = 0x1B, /* @1Hz6 (low power) */
  LSM6DSO_XL_UI_1Hz6_ULP  = 0x2B, /* @1Hz6 (ultra low/Gy, OIS imu off) */
  LSM6DSO_XL_UI_12Hz5_HP  = 0x01, /* @12Hz5 (high performance) */
  LSM6DSO_XL_UI_12Hz5_LP  = 0x11, /* @12Hz5 (low power) */
  LSM6DSO_XL_UI_12Hz5_ULP = 0x21, /* @12Hz5 (ultra low/Gy, OIS imu off) */
  LSM6DSO_XL_UI_26Hz_HP   = 0x02, /* @26Hz  (high performance) */
  LSM6DSO_XL_UI_26Hz_LP   = 0x12, /* @26Hz  (low power) */
  LSM6DSO_XL_UI_26Hz_ULP  = 0x22, /* @26Hz  (ultra low/Gy, OIS imu off) */
  LSM6DSO_XL_UI_52Hz_HP   = 0x03, /* @52Hz  (high performance) */
  LSM6DSO_XL_UI_52Hz_LP   = 0x13, /* @52Hz  (low power) */
  LSM6DSO_XL_UI_52Hz_ULP  = 0x23, /* @52Hz  (ultra low/Gy, OIS imu off) */
  LSM6DSO_XL_UI_104Hz_HP  = 0x04, /* @104Hz (high performance) */
  LSM6DSO_XL_UI_104Hz_NM  = 0x14, /* @104Hz (normal mode) */
  LSM6DSO_XL_UI_104Hz_ULP = 0x24, /* @104Hz (ultra low/Gy, OIS imu off) */
  LSM6DSO_XL_UI_208Hz_HP  = 0x05, /* @208Hz (high performance) */
  LSM6DSO_XL_UI_208Hz_NM  = 0x15, /* @208Hz (normal mode) */
  LSM6DSO_XL_UI_208Hz_ULP = 0x25, /* @208Hz (ultra low/Gy, OIS imu off) */
  LSM6DSO_XL_UI_416Hz_HP  = 0x06, /* @416Hz (high performance) */
  LSM6DSO_XL_UI_833Hz_HP  = 0x07, /* @833Hz (high performance) */
  LSM6DSO_XL_UI_1667Hz_HP = 0x08, /* @1kHz66 (high performance) */
  LSM6DSO_XL_UI_3333Hz_HP = 0x09, /* @3kHz33 (high performance) */
  LSM6DSO_XL_UI_6667Hz_HP = 0x0A, /* @6kHz66 (high performance) */
} lsm6dso_odr_xl_ui_t;

typedef enum
{
  LSM6DSO_XL_UI_2g   = 0,
  LSM6DSO_XL_UI_4g   = 2,
  LSM6DSO_XL_UI_8g   = 3,
  LSM6DSO_XL_UI_16g  = 1, /* OIS full scale is also forced to be 16g */
} lsm6dso_fs_xl_ui_t;

typedef enum
{
  LSM6DSO_GY_UI_OFF       = 0x00, /* gy in power down */
  LSM6DSO_GY_UI_12Hz5_LP  = 0x11, /* gy @12Hz5 (low power) */
  LSM6DSO_GY_UI_12Hz5_HP  = 0x01, /* gy @12Hz5 (high performance) */
  LSM6DSO_GY_UI_26Hz_LP   = 0x12, /* gy @26Hz  (low power) */
  LSM6DSO_GY_UI_26Hz_HP   = 0x02, /* gy @26Hz  (high performance) */
  LSM6DSO_GY_UI_52Hz_LP   = 0x13, /* gy @52Hz  (low power) */
  LSM6DSO_GY_UI_52Hz_HP   = 0x03, /* gy @52Hz  (high performance) */
  LSM6DSO_GY_UI_104Hz_NM  = 0x14, /* gy @104Hz (low power) */
  LSM6DSO_GY_UI_104Hz_HP  = 0x04, /* gy @104Hz (high performance) */
  LSM6DSO_GY_UI_208Hz_NM  = 0x15, /* gy @208Hz (low power) */
  LSM6DSO_GY_UI_208Hz_HP  = 0x05, /* gy @208Hz (high performance) */
  LSM6DSO_GY_UI_416Hz_HP  = 0x06, /* gy @416Hz (high performance) */
  LSM6DSO_GY_UI_833Hz_HP  = 0x07, /* gy @833Hz (high performance) */
  LSM6DSO_GY_UI_1667Hz_HP = 0x08, /* gy @1kHz66 (high performance) */
  LSM6DSO_GY_UI_3333Hz_HP = 0x09, /* gy @3kHz33 (high performance) */
  LSM6DSO_GY_UI_6667Hz_HP = 0x0A, /* gy @6kHz66 (high performance) */
} lsm6dso_odr_g_ui_t;

typedef enum
{
  LSM6DSO_GY_UI_250dps   = 0,
  LSM6DSO_GY_UI_125dps   = 1,
  LSM6DSO_GY_UI_500dps   = 2,
  LSM6DSO_GY_UI_1000dps  = 4,
  LSM6DSO_GY_UI_2000dps  = 6,
} lsm6dso_fs_g_ui_t;

typedef enum
{
  LSM6DSO_OIS_ONLY_AUX    = 0x00, /* Auxiliary SPI full control */
  LSM6DSO_OIS_MIXED       = 0x01, /* Enabling by UI / read-config by AUX */
} lsm6dso_ctrl_md_t;

typedef enum
{
  LSM6DSO_XL_OIS_OFF       = 0x00, /* in power down */
  LSM6DSO_XL_OIS_6667Hz_HP = 0x01, /* @6kHz OIS imu active/NO ULP on UI */
} lsm6dso_odr_xl_ois_noaux_t;

typedef enum
{
  LSM6DSO_XL_OIS_2g   = 0,
  LSM6DSO_XL_OIS_4g   = 2,
  LSM6DSO_XL_OIS_8g   = 3,
  LSM6DSO_XL_OIS_16g  = 1, /* UI full scale is also forced to be 16g */
} lsm6dso_fs_xl_ois_noaux_t;

typedef enum
{
  LSM6DSO_GY_OIS_OFF       = 0x00, /* in power down */
  LSM6DSO_GY_OIS_6667Hz_HP = 0x01, /* @6kHz No Ultra Low Power*/
} lsm6dso_odr_g_ois_noaux_t;

typedef enum
{
  LSM6DSO_GY_OIS_250dps   = 0,
  LSM6DSO_GY_OIS_125dps   = 1,
  LSM6DSO_GY_OIS_500dps   = 2,
  LSM6DSO_GY_OIS_1000dps  = 4,
  LSM6DSO_GY_OIS_2000dps  = 6,
} lsm6dso_fs_g_ois_noaux_t;

typedef enum
{
  LSM6DSO_FSM_DISABLE = 0x00,
  LSM6DSO_FSM_XL      = 0x01,
  LSM6DSO_FSM_GY      = 0x02,
  LSM6DSO_FSM_XL_GY   = 0x03,
} lsm6dso_sens_fsm_t;

typedef enum
{
  LSM6DSO_FSM_12Hz5 = 0x00,
  LSM6DSO_FSM_26Hz  = 0x01,
  LSM6DSO_FSM_52Hz  = 0x02,
  LSM6DSO_FSM_104Hz = 0x03,
} lsm6dso_odr_fsm_t;

typedef struct
{
  struct
  {
    struct
    {
      lsm6dso_odr_xl_ui_t odr;
      lsm6dso_fs_xl_ui_t fs;
    } xl;
    struct
    {
      lsm6dso_odr_g_ui_t odr;
      lsm6dso_fs_g_ui_t fs;
    } gy;
  } ui;
  struct
  {
    lsm6dso_ctrl_md_t ctrl_md;
    struct
    {
      lsm6dso_odr_xl_ois_noaux_t odr;
      lsm6dso_fs_xl_ois_noaux_t fs;
    } xl;
    struct
    {
      lsm6dso_odr_g_ois_noaux_t odr;
      lsm6dso_fs_g_ois_noaux_t fs;
    } gy;
  } ois;
  struct
  {
    lsm6dso_sens_fsm_t sens;
    lsm6dso_odr_fsm_t odr;
  } fsm;
} lsm6dso_md_t;

typedef struct
{
  struct
  {
    struct
    {
      float_t mg[3];
      int16_t raw[3];
    } xl;
    struct
    {
      float_t mdps[3];
      int16_t raw[3];
    } gy;
    struct
    {
      float_t deg_c;
      int16_t raw;
    } heat;
  } ui;
  struct
  {
    struct
    {
      float_t mg[3];
      int16_t raw[3];
    } xl;
    struct
    {
      float_t mdps[3];
      int16_t raw[3];
    } gy;
  } ois;
} lsm6dso_data_t;


// == LSM6DSO ==

class Lsm6dso {
public:
	Lsm6dso(uint8_t i2cAddr, Lsm6dsoI3C i3c, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr, Lsm6dsoWakeThs wakeThs,
			Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur);

private:
	bool configure(Lsm6dsoI3C i3c, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr, Lsm6dsoWakeThs wakeThs,
			Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur);

	uint8_t setConfigurationREG_CTRL1_XL(Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc);
    uint8_t setConfigurationREG_CTRL2_G(Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr);
    uint8_t setConfigurationREG_CTRL3_C(Lsm6dsoIfInc ifInc);
    uint8_t setConfigurationREG_CTRL6_C(Lsm6dsoXlHm xlHm);
    uint8_t setConfigurationREG_CTRL7_G(Lsm6dsoGHm gHm);
	uint8_t setConfigurationREG_CTRL9_XL(Lsm6dsoI3C i3c);
    uint8_t setConfigurationREG_TAP_CFG0(Lsm6dsoSlopeFilterEn sF);
    uint8_t setConfigurationREG_TAP_CFG2(Lsm6dsoIntEn intEn, Lsm6dsoInactEn inActEn);
	uint8_t setConfigurationREG_WAKE_UP_THS(Lsm6dsoWakeThs wakeThs);
    uint8_t setConfigurationREG_WAKE_UP_DUR(Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur);
    uint8_t setConfigurationREG_MD1_CFG(Lsm6dsoIntWU intWU);

    bool writeRegister(uint8_t reg, uint8_t value);


    float_t lsm6dso_from_fs2_to_mg(int16_t lsb);
    float_t lsm6dso_from_fs4_to_mg(int16_t lsb);
    float_t lsm6dso_from_fs8_to_mg(int16_t lsb);
    float_t lsm6dso_from_fs16_to_mg(int16_t lsb);
    float_t lsm6dso_from_lsb_to_celsius(int16_t lsb);
    float_t lsm6dso_from_lsb_to_nsec(int16_t lsb);

    int32_t lsm6dso_read_reg(const stmdev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len);
    int32_t lsm6dso_acceleration_raw_get (const stmdev_ctx_t *ctx, int16_t *val);
    int32_t lsm6dso_data_get(const stmdev_ctx_t *ctx, const lsm6dso_md_t *md, lsm6dso_data_t *data);


private:
    uint8_t i2cAddr;
};


#endif /* MODULES_IMU_LSM6DSO_H_ */
