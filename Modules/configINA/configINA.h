
#ifndef MODULES_CONFIGINA_CONFIGINA_H_
#define MODULES_CONFIGINA_CONFIGINA_H_


#define CONFIGREG	0x00	// Configuration Register (00h)
#define VSHUNTREG	0x01	// Shunt Voltage Register (01h)
#define VBUSREG		0x02	// Bus Voltage Register (02h)
#define POWERREG	0x03	// Power Register (03h)
#define IREG		0x04	// Current Register (04h)
#define CALREG		0x05	// Calibration Register (05h)
#define MASKREG		0x06	// Mask/Enable Register (06h)
#define ALTERREG	0x07	// Alert Limit Register (07h)
#define MANIDREG	0xFE	// Manufacturer ID Register (FEh)
#define DIEIDREG	0xFF	// Die ID Register (FFh)


#define INA_GPS_ADDRESS 0x80	// 0x40 << 1 // INA-GPS 8-bit Address.
#define INA_IMU_ADDRESS 0x82	// 0x41 << 1 // INA-MCU 8-bit Address.
#define	INA_MCU_ADDRESS 0x8A	// 0x45 << 1 // INA-MCU 8-bit Address.

#define LSB_MASK 0x00FF
#define MSB_MASK 0xFF00

// Write register into the INA226 via I2C
void INA226_write_register(uint8_t reg, uint8_t data);

// Initialization of INA226 for GPS
void INA226_GPS_init();

// Initialization of INA226 for IMU
void INA226_IMU_init();

// Initialization of INA226 for MCU
void INA226_MCU_init();

#endif /* MODULES_CONFIGINA_CONFIGINA_H_ */
