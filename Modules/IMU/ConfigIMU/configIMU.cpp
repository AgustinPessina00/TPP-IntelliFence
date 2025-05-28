
/* Includes ------------------------------------------------------------------*/

#include "configIMU.h"

/* Private includes ----------------------------------------------------------*/


void LSM6DSO_write_register(uint8_t reg, uint8_t value) {
	uint8_t info[2];
	info[0] = reg;
	info[1] = value;
	HAL_I2C_Master_Transmit(&hi2c2, LSM6DSO_ADDRESS, info, 2, HAL_MAX_DELAY);
}


void LSM6DSO_init(){

	// Deshabilita Interface I3C
	LSM6DSO_write_register(CTRL9_XL, 0x02); // 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0	-> | I3C_disable = 1 (Deshabilita interface I3C)|

	// Inicializar Acelerómetro (Low-power)
	LSM6DSO_write_register(CTRL1_XL, 0x28); // 0010 | 10 | 0 | 0	-> ODR = 26Hz(LP Mode) | FS = ±4g | 0 (reserved)

	// Inicializar giroscopio (Power-down)
	LSM6DSO_write_register(CTRL2_G, 0x00); // 0000 | 00 | 0 | 0	-> ODR = Power-down | FS = ±250dps | 0 (reserved)

	// Inicializar Operating mode Acelerómetro (Low-power)
	LSM6DSO_write_register(CTRL6_C, 0x10); //  | 1 |	->	| XL_HM_MODE = 1 (High-performance disabled) |

	// Inicializar Operating mode Giroscopio (DISABLED)
	LSM6DSO_write_register(CTRL7_G, 0x80); // 1 | 	-> G_HM_MODE = 1 (High-performance disabled) |

	// Configurar el THS del Wake-Up
	LSM6DSO_write_register(WAKE_UP_THS, 0x02); // 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0	-> | WK_THS[5:0] = 2 (1LSB x 2 = 125mg) |

	// Configurar duración para acivar Wake-Up
	LSM6DSO_write_register(WAKE_UP_DUR, 0x00); // 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0	-> | WAKE_DUR[1:0] = 0 (1LSB = 1ODR_time = 1/26Hz = 38.5ms) | WAKE_THS_W = 0 (1LSB = FS_XL/2^6 = 4g*1000/2^6 = 62.5mg) |

	// Configurar Filtros de la Interrupción
	LSM6DSO_write_register(TAP_CFG0, 0x00); // 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0	-> | SLOPE_FDS = 0 (Aplica filtro SLOPE)|

	// Habilitar Interrupciones
	LSM6DSO_write_register(TAP_CFG2, 0x80); // 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0	-> INT_EN = 1 (Habilita interrupciones básicas)|

	// Rutear INT1 con la función Wake-Up
	LSM6DSO_write_register(MD1_CFG, 0x20); // 0 | 0 | 1 | 0 | 0 | 0 | 0 | 0	-> | INT1_WU = 1 |

	// Configurar Nivel de Activación de INT1
	LSM6DSO_write_register(CTRL3_C, 0x00); // 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0	-> | H_LACTIVE = 0 (INT1 se activa en ALTO) | PP_OD = 0 (Modo Push-Pull)


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
