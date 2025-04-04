/*
 * configIMU.h
 *
 *  Created on: Mar 24, 2025
 *      Author: Agustín | Ignacio | Ezequiel
 */

#ifndef MODULES_CONFIGIMU_CONFIGIMU_H_
#define MODULES_CONFIGIMU_CONFIGIMU_H_

// static const uint8_t IMU_ADDRESS = 0xD4;	// 0x6A << 1 // IMU 8-bit Address.
#define IMU_ADDRESS = 0xD4;	// 0x6A << 1 // IMU 8-bit Address.

// Registro de control
#define CTRL1_XL		0x10	// Control de acelerómetro (10h = 0x10hexadecimal)
#define CTRL2_G			0x11	// Control de giroscopio (11h)
#define CTRL5_C			0x14	//
#define CTRL6_C         0x15	// 0x0001 1000 Operating Mode Acelerómetro (15h)
#define CTRL7_C			0x15	// 0x0001 1000 Operating Mode Giroscopio (16h)
#define CTRL8_XL		0x17
#define WAKE_UP_THS		0x5B
#define WAKE_UP_DUR		0x5C
#define TAP_CFG			0x58
#define MD1_CFG			0x5E


/*
 * NACHO: VER SI USAMOS FIFO
 * FIFO_CTRL1 (07h), FIFO_CTRL2 (08h), FIFO_STATUS1 (3Ah), FIFO_STATUS2(3Bh), FIFO_CTRL4 (0Ah).
 */

// Write of register
void LSM6DSO_write_reg(uint8_t reg, uint8_t data);

// Initialization of Acelerometer and Gyroscope
void acel_gyro_init();

#endif /* MODULES_CONFIGIMU_CONFIGIMU_H_ */
