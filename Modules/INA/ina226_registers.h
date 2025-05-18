#ifndef INA226_REGISTERS_H
#define INA226_REGISTERS_H

// ==== Dirección de registros ====
#define REG_CFG           0x00
#define REG_VSHUNT        0x01
#define REG_VBUS          0x02
#define REG_POWER         0x03
#define REG_CURRENT       0x04
#define REG_CALIB         0x05
#define REG_MASK_EN       0x06
#define REG_ALERT_LIMIT   0x07
#define REG_MANUF_ID      0xFE
#define REG_DIE_ID        0xFF

// ==== CFG Register (0x00) ====
#define CFG_RST              15
#define CFG_RSV_2            14
#define CFG_RSV_1            13
#define CFG_RSV_0            12
#define CFG_AVG2             11
#define CFG_AVG1             10
#define CFG_AVG0             9
#define CFG_VBUSCT2          8
#define CFG_VBUSCT1          7
#define CFG_VBUSCT0          6
#define CFG_VSHCT2           5
#define CFG_VSHCT1           4
#define CFG_VSHCT0           3
#define CFG_MODE2            2
#define CFG_MODE1            1
#define CFG_MODE0            0

// ==== Shunt Voltage Register (0x01) ====
#define VSHUNT_SIGN       15
#define VSHUNT_SD14       14
#define VSHUNT_SD13       13
#define VSHUNT_SD12       12
#define VSHUNT_SD11       11
#define VSHUNT_SD10       10
#define VSHUNT_SD9        9
#define VSHUNT_SD8        8
#define VSHUNT_SD7        7
#define VSHUNT_SD6        6
#define VSHUNT_SD5        5
#define VSHUNT_SD4        4
#define VSHUNT_SD3        3
#define VSHUNT_SD2        2
#define VSHUNT_SD1        1
#define VSHUNT_SD0        0

// ==== Bus Voltage Register (0x02) ====
#define VBUS_RSVD         15
#define VBUS_BD14         14
#define VBUS_BD13         13
#define VBUS_BD12         12
#define VBUS_BD11         11
#define VBUS_BD10         10
#define VBUS_BD9          9
#define VBUS_BD8          8
#define VBUS_BD7          7
#define VBUS_BD6          6
#define VBUS_BD5          5
#define VBUS_BD4          4
#define VBUS_BD3          3
#define VBUS_BD2          2
#define VBUS_BD1          1
#define VBUS_BD0          0

// ==== Power Register (0x03) ====
#define PWR_PD15          15
#define PWR_PD14          14
#define PWR_PD13          13
#define PWR_PD12          12
#define PWR_PD11          11
#define PWR_PD10          10
#define PWR_PD9           9
#define PWR_PD8           8
#define PWR_PD7           7
#define PWR_PD6           6
#define PWR_PD5           5
#define PWR_PD4           4
#define PWR_PD3           3
#define PWR_PD2           2
#define PWR_PD1           1
#define PWR_PD0           0

// ==== Current Register (0x04) ====
#define CUR_CSGN          15
#define CUR_CD14          14
#define CUR_CD13          13
#define CUR_CD12          12
#define CUR_CD11          11
#define CUR_CD10          10
#define CUR_CD9           9
#define CUR_CD8           8
#define CUR_CD7           7
#define CUR_CD6           6
#define CUR_CD5           5
#define CUR_CD4           4
#define CUR_CD3           3
#define CUR_CD2           2
#define CUR_CD1           1
#define CUR_CD0           0

// ==== Calibration Register (0x05) ====
#define CAL_RSVD          15
#define CAL_FS14          14
#define CAL_FS13          13
#define CAL_FS12          12
#define CAL_FS11          11
#define CAL_FS10          10
#define CAL_FS9           9
#define CAL_FS8           8
#define CAL_FS7           7
#define CAL_FS6           6
#define CAL_FS5           5
#define CAL_FS4           4
#define CAL_FS3           3
#define CAL_FS2           2
#define CAL_FS1           1
#define CAL_FS0           0

// ==== Mask/Enable Register (0x06) ====
#define MSK_SOL           15
#define MSK_SUL           14
#define MSK_BOL           13
#define MSK_BUL           12
#define MSK_POL           11
#define MSK_CNVR          10
#define MSK_RSVD9         9
#define MSK_RSVD8         8
#define MSK_RSVD7         7
#define MSK_RSVD6         6
#define MSK_RSVD5         5
#define MSK_AFF           4
#define MSK_CVRF          3
#define MSK_OVF           2
#define MSK_APOL          1
#define MSK_LEN           0

// ==== Alert Limit Register (0x07) ====
#define ALRT_AUL15        15
#define ALRT_AUL14        14
#define ALRT_AUL13        13
#define ALRT_AUL12        12
#define ALRT_AUL11        11
#define ALRT_AUL10        10
#define ALRT_AUL9         9
#define ALRT_AUL8         8
#define ALRT_AUL7         7
#define ALRT_AUL6         6
#define ALRT_AUL5         5
#define ALRT_AUL4         4
#define ALRT_AUL3         3
#define ALRT_AUL2         2
#define ALRT_AUL1         1
#define ALRT_AUL0         0



#endif // INA226_REGISTERS_H
