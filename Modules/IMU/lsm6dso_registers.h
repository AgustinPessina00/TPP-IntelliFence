
#ifndef LSM6DSO_REGISTERS_H
#define LSM6DSO_REGISTERS_H

// ==== Register Address Map (LSM6DSO) ====
#define REG_FUNC_CFG_ACCESS        0x01
#define REG_PIN_CTRL               0x02
#define REG_FIFO_CTRL1             0x07
#define REG_FIFO_CTRL2             0x08
#define REG_FIFO_CTRL3             0x09
#define REG_FIFO_CTRL4             0x0A
#define REG_COUNTER_BDR_REG1       0x0B
#define REG_COUNTER_BDR_REG2       0x0C
#define REG_INT1_CTRL              0x0D
#define REG_INT2_CTRL              0x0E
#define REG_WHO_AM_I               0x0F
#define REG_CTRL1_XL               0x10
#define REG_CTRL2_G                0x11
#define REG_CTRL3_C                0x12
#define REG_CTRL4_C                0x13
#define REG_CTRL5_C                0x14
#define REG_CTRL6_C                0x15
#define REG_CTRL7_G                0x16
#define REG_CTRL8_XL               0x17
#define REG_CTRL9_XL               0x18
#define REG_CTRL10_C               0x19
#define REG_ALL_INT_SRC            0x1A
#define REG_WAKE_UP_SRC            0x1B
#define REG_TAP_SRC                0x1C
#define REG_D6D_SRC                0x1D
#define REG_STATUS_REG             0x1E

// ==== Output Registers ====
#define REG_OUT_TEMP_L             0x20
#define REG_OUT_TEMP_H             0x21
#define REG_OUTX_L_G               0x22
#define REG_OUTX_H_G               0x23
#define REG_OUTY_L_G               0x24
#define REG_OUTY_H_G               0x25
#define REG_OUTZ_L_G               0x26
#define REG_OUTZ_H_G               0x27
#define REG_OUTX_L_A               0x28
#define REG_OUTX_H_A               0x29
#define REG_OUTY_L_A               0x2A
#define REG_OUTY_H_A               0x2B
#define REG_OUTZ_L_A               0x2C
#define REG_OUTZ_H_A               0x2D

// ==== Function Status ====
#define REG_EMB_FUNC_STATUS_MAIN   0x35
#define REG_FSM_STATUS_A_MAIN      0x36
#define REG_FSM_STATUS_B_MAIN      0x37

// ==== FIFO Status ====
#define REG_STATUS_MASTER_MAIN     0x39
#define REG_FIFO_STATUS1           0x3A
#define REG_FIFO_STATUS2           0x3B

// ==== Timestamp Registers ====
#define REG_TIMESTAMP0             0x40
#define REG_TIMESTAMP1             0x41
#define REG_TIMESTAMP2             0x42
#define REG_TIMESTAMP3             0x43

// ==== TAP and Motion Configuration ====
#define REG_TAP_CFG0               0x56
#define REG_TAP_CFG1               0x57
#define REG_TAP_CFG2               0x58
#define REG_TAP_THS_6D             0x59
#define REG_INT_DUR2               0x5A
#define REG_WAKE_UP_THS            0x5B
#define REG_WAKE_UP_DUR            0x5C
#define REG_FREE_FALL              0x5D
#define REG_MD1_CFG                0x5E
#define REG_MD2_CFG                0x5F

// ==== Clock and Interface Config ====
#define REG_I3C_BUS_AVB            0x62
#define REG_INTERNAL_FREQ_FINE     0x63

// ==== OIS (Aux SPI) Config ====
#define REG_INT_OIS                0x6F
#define REG_CTRL1_OIS              0x70
#define REG_CTRL2_OIS              0x71
#define REG_CTRL3_OIS              0x72

// ==== User Offset Correction ====
#define REG_X_OFS_USR              0x73
#define REG_Y_OFS_USR              0x74
#define REG_Z_OFS_USR              0x75

// ==== FIFO Data Output ====
#define REG_FIFO_DATA_OUT_TAG      0x78
#define REG_FIFO_DATA_OUT_X_L      0x79
#define REG_FIFO_DATA_OUT_X_H      0x7A
#define REG_FIFO_DATA_OUT_Y_L      0x7B
#define REG_FIFO_DATA_OUT_Y_H      0x7C
#define REG_FIFO_DATA_OUT_Z_L      0x7D
#define REG_FIFO_DATA_OUT_Z_H      0x7E


// ==== WRITE REGISTERS ====

// ==== CTRL1_XL (0x10) ====
#define ODR_XL3        	7
#define ODR_XL2        	6
#define ODR_XL1        	5
#define ODR_XL0        	4
#define FS1_XL         	3
#define FS0_XL         	2
#define LPF2_XL_EN     	1
#define CTRL1_XL_RSV	0

// ==== CTRL2_G (0x11) ====
#define ODR_G3         	7
#define ODR_G2         	6
#define ODR_G1         	5
#define ODR_G0         	4
#define FS1_G          	3
#define FS0_G          	2
#define FS_G_125       	1
#define CTRL2_G_RSV		0

// ==== CTRL3_C (0x12) ====
#define BOOT         	7
#define BDU         	6
#define H_LACTIVE		5
#define PP_OD        	4
#define SIM          	3
#define IF_INC       	2
#define CTRL3_RSV    	1
#define SW_RESET     	0

// ==== CTRL6_C (0x15) ====
#define TRIG_EN     	7
#define LVL1_EN     	6
#define LVL2_EN      	5
#define XL_HM_MODE		4
#define USR_OFF_W		3
#define FTYPE2        	2
#define FTYPE1        	1
#define FTYPE0			0

// ==== CTRL7_G (0x16) ====
#define G_HM_MODE     	7
#define HP_EN_G       	6
#define HPM1_G        	5
#define HPM0_G        	4
#define CTRL7_RSV	  	3
#define OIS_ON_EN     	2
#define USR_OFF_ON_OUT	1
#define OIS_ON        	0

// ==== CTRL9_XL (0x18) ====
#define DEN_X        	7
#define DEN_Y        	6
#define DEN_Z        	5
#define DEN_XL_G     	4
#define DEN_XL_EN    	3
#define DEN_LH       	2
#define I3C_DISABLE		1
#define CTRL9_RSV    	0

// ==== TAP_CFG0 (0x56) ====
#define TAP_CFG0_RSV		7
#define INT_CLR_ON_READ		6
#define SLEEP_STS_ON_INT	5
#define SLOPE_FDS       	4
#define TAP_X_EN        	3
#define TAP_Y_EN        	2
#define TAP_Z_EN        	1
#define LIR             	0

// ==== TAP_CFG2 (0x58) ====
#define INT_EN   		7
#define INACT_EN1       6
#define INACT_EN0       5
#define TAP_THS_Y_4     4
#define TAP_THS_Y_3     3
#define TAP_THS_Y_2     2
#define TAP_THS_Y_1     1
#define TAP_THS_Y_0		0

// ==== WAKE_UP_THS (0x5B) ====
#define SINGLE_DOUB_TAP		7
#define USR_OFF_ON_WU      	6
#define WK_THS5     	  	5
#define WK_THS4				4
#define WK_THS3       		3
#define WK_THS2       		2
#define WK_THS1       		1
#define WK_THS0       		0

// ==== WAKE_UP_DUR (0x5C) ====
#define FF_DUR5       	7
#define WAKE_DUR1     	6
#define WAKE_DUR0     	5
#define WAKE_THS_W		4
#define SLEEP_DUR3    	3
#define SLEEP_DUR2    	2
#define SLEEP_DUR1    	1
#define SLEEP_DUR0    	0

// ==== MD1_CFG (0x5E) ====
#define INT1_SLEEP_CHG		7
#define INT1_SINGLE_TAP		6
#define INT1_WU           	5
#define INT1_FF           	4
#define IN1_DOUBLE_TAP		3
#define INT1_6D				2
#define INT1_EMB_FUNC		1
#define INT1_SHUB			0


// ==== READ REGISTERS ====

// ==== OUTX_L_A (0x28) ====
#define XLA_D7		7
#define XLA_D6		6
#define XLA_D5		5
#define XLA_D4		4
#define XLA_D3		3
#define XLA_D2		2
#define XLA_D1		1
#define XLA_D0		0

// ==== OUTX_H_A (0x29) ====
#define XHA_D15		7
#define XHA_D14		6
#define XHA_D13		5
#define XHA_D12		4
#define XHA_D11		3
#define XHA_D10		2
#define XHA_D9		1
#define XHA_D8		0

// ==== OUTY_L_A (0x2A) ====
#define YLA_D7		7
#define YLA_D6		6
#define YLA_D5		5
#define YLA_D4		4
#define YLA_D3		3
#define YLA_D2		2
#define YLA_D1		1
#define YLA_D0		0

// ==== OUTY_H_A (0x2B) ====
#define YHA_D15		7
#define YHA_D14		6
#define YHA_D13		5
#define YHA_D12		4
#define YHA_D11		3
#define YHA_D10		2
#define YHA_D9		1
#define YHA_D8		0

// ==== OUTZ_L_A (0x2C) ====
#define ZLA_D7		7
#define ZLA_D6		6
#define ZLA_D5		5
#define ZLA_D4		4
#define ZLA_D3		3
#define ZLA_D2		2
#define ZLA_D1		1
#define ZLA_D0		0

// ==== OUTZ_H_A (0x2D) ====
#define ZHA_D15		7
#define ZHA_D14		6
#define ZHA_D13		5
#define ZHA_D12		4
#define ZHA_D11		3
#define ZHA_D10		2
#define ZHA_D9		1
#define ZHA_D8		0


#endif // LSM6DSO_REGISTERS_H
