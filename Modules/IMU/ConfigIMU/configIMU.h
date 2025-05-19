
#ifndef MODULES_CONFIGIMU_CONFIGIMU_H_
#define MODULES_CONFIGIMU_CONFIGIMU_H_


// Registro de control
#define CTRL1_XL		0x10	// Control de acelerómetro (10h = 0x10hexadecimal)
#define CTRL2_G			0x11	// Control de giroscopio (11h)
#define CTRL3_C			0x12	// Configurar Activación de INT1 (12h)
#define CTRL6_C         0x15	// Operating Mode Acelerómetro (15h)
#define CTRL7_C			0x16	// Operating Mode Giroscopio (16h)
#define CTRL9_XL		0x18	// Deshabilitar Interface I3C (18h)
#define TAP_CFG0		0x56	// Aplicar Filtro HPF o SLOPE (56h)
#define TAP_CFG2		0x58	// Habilitar Interrupciones Básicas (58h)
#define WAKE_UP_THS		0x5B	// Configuración de Wake-up (5Bh)
#define WAKE_UP_DUR		0x5C	// Duración del Wake-Up (5Ch)
#define MD1_CFG			0x5E	// Rutear Funciones del pin INT1 (5Eh)


#define LSM6DSO_ADDRES 0xD4		// 0x6A << 1 // IMU 8-bit Address.


// Write register into the LSM6DSO via I2C
void LSM6DSO_write_register(uint8_t reg, uint8_t value);

// Initialization of LSM6DSO Accelerometer
void LSM6DSO_init();

#endif /* MODULES_CONFIGIMU_CONFIGIMU_H_ */
