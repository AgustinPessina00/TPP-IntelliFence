/*
 * configINA.h
 *
 *  Created on: Apr 10, 2025
 *      Author: nacho
 */

#ifndef MODULES_CONFIGINA_CONFIGINA_H_
#define MODULES_CONFIGINA_CONFIGINA_H_


#define INA_GPS_ADDRESS = 0x80;	// 0x40 << 1 // INA-GPS 8-bit Address.
#define INA_IMU_ADDRESS = 0x82;	// 0x41 << 1 // INA-IMU 8-bit Address.
#define INA_MCU_ADDRESS = 0x8A;	// 0x45 << 1 // IMU-MCU 8-bit Address.

#define CONFIGREG	0x00	// Configuration Register (00h)
#define VSHUNTREG	0x01	// Shunt Voltage Register (01h)
#define VBUSREG		0x02	// Bus Voltage Register (02h)
#define POWERREG	0x03	// Power Register (03h)
#define IREG		0x04	// Current Register (04h)
#define CALREG		0x05	// Calibration Register (05h)
#define MASKREG		0x06	// Mask/Enable Register (06h)
#define ALTERREG	0x07	// Altert Limit Register (07h)
#define MANIDREG	0xFE	// Manufacturer ID Register (FEh)
#define DIEIDREG	0xFF	// Die ID Register (FFh)



#endif /* MODULES_CONFIGINA_CONFIGINA_H_ */
