/*
 * configIMU.cpp
 *
 *  Created on: Mar 24, 2025
 *      Author: Agustín | Ignacio | Ezequiel
 */

/* Includes ------------------------------------------------------------------*/

#include "configIMU.h"

/* Private includes ----------------------------------------------------------*/




void acel_gyro_init(){

	// Inicializar Acelerómetro (Low-power) -> 0x4A
	LSM6DSO_WriteReg(CTRL1_XL, 0x4A); // 0bxxxx xxxx
	/*
	data[0] = CTRL1_XL;
	data[1] = 0x4A;		// 0100 | 10 | 1 | 0	-> ODR = 52Hz(LP Mode) | FS=±4g | 0
	HAL_I2C_Master_Transmit(&hi2c1, LSM6DSO_ADDR, data, 2, HAL_MAX_DELAY);
	*/

	// Inicializar giroscopio (Power-down)
	data[0] = CTRL2_G;
	data[1] = 0x00; // 0000 | 00 | 0 | 0	-> ODR=Power-down | FS=±250dps | 0
	HAL_I2C_Master_Transmit(&hi2c1, LSM6DSO_ADDR, data, 2, HAL_MAX_DELAY);

	// Inicializar Operating mode Acelerómetro (Low-power)
	data[0] = CTRL6_C;
	data[1] = ; //	->	| XL_HM_MODE = 1 (High-performance disabled) |
	HAL_I2C_Master_Transmit(&hi2c1, LSM6DSO_ADDR, data, 2, HAL_MAX_DELAY);

	// Inicializar Operating mode Giroscopio (DISABLED)
	data[0] = CTRL7_C;
	data[1] = ; //	1 | 	->	| G_HM_MODE = 1 (High-performance disabled) |
	HAL_I2C_Master_Transmit(&hi2c1, LSM6DSO_ADDR, data, 2, HAL_MAX_DELAY);

}

void LSM6DSO_WriteReg(uint8_t reg, uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, LSM6DSO_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
}


// NACHO: NO VA ACÁ EN EL CONFIG, PERO LO DEJO PARA SABER COMO ES LA LECTURA.
/*
PARAM_TYPE read_data(){
	uint8_t accel_data[6];
	HAL_I2C_Mem_Read(&hi2c1, LSM6DSO_ADDR, 0x28, I2C_MEMADD_SIZE_8BIT, accel_data, 6, HAL_MAX_DELAY);
	int16_t ax = (int16_t)(accel_data[1] << 8 | accel_data[0]);
	int16_t ay = (int16_t)(accel_data[3] << 8 | accel_data[2]);
	int16_t az = (int16_t)(accel_data[5] << 8 | accel_data[4]);
	return PARAM_TYPE
}
*/
