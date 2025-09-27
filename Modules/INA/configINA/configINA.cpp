
/* Includes ------------------------------------------------------------------*/

#include "configINA.h"

/* Private includes ----------------------------------------------------------*/


void INA226_write_register(uint8_t reg, uint16_t value, uint16_t address){
	uint8_t info[3];
	info[0] = reg;
	info[1] = value & LSB_MASK;
	info[2] = (value & MSB_MASK) >> 8;

	HAL_I2C_Master_Transmit(&hi2c2, address, info, 3, HAL_MAX_DELAY);
}


void INA226_GPS_init(){
	// Configuración del Configuration Register (Averaging Mode, VBUS & VSHUNT Convertion Time, Modo de Operación)
	INA226_write_register(CONFIGREG, 0x4127, INA_GPS_ADDRESS); // 0 | 100 | 000 | 100 | 100 | 111	-> RST = 0 | Reservado | AVG = 000 (Number of Averages = 1) | VBUSCT = 100 (CT = 1.1ms) | VSHUNT = 100 (CT = 1.1ms) | MODE = 111 (Shunt & Bus Continuous)

	// Configuración del Calibration Register (Resolución del Current Register dependiendo la resistencia de Shunt)
	INA226_write_register(CALREG, 0x1AAB, INA_GPS_ADDRESS); // GPS MAX (estimado): 25mA -> Current_LSB = 1uA/bit -> CAL = 6827 = 0x1AABh.

	// Configuración del MASK/ENABLE Register (Pin Alert: Read/Write)
	INA226_write_register(MASKREG, 0x0000, INA_GPS_ADDRESS); // NOTA: En este registro se puede configurar el funcionamiento del ALERT Pin.
}


void INA226_IMU_init(){
	// Configuración del Configuration Register (Averaging Mode, VBUS & VSHUNT Convertion Time, Modo de Operación)
	INA226_write_register(CONFIGREG, 0x4127, INA_IMU_ADDRESS); // 0 | 100 | 000 | 100 | 100 | 111	-> RST = 0 | Reservado | AVG = 000 (Number of Averages = 1) | VBUSCT = 100 (CT = 1.1ms) | VSHUNT = 100 (CT = 1.1ms) | MODE = 111 (Shunt & Bus Continuous)

	// Configuración del Calibration Register (Resolución del Current Register dependiendo la resistencia de Shunt)
	INA226_write_register(CALREG, 0xC800, INA_IMU_ADDRESS); // IMU MAX (estimado): 26uA -> Current_LSB = 10nA/bit -> CAL = 51200 = 0xC800h.

	// Configuración del MASK/ENABLE Register (Pin Alert: Read/Write)
	INA226_write_register(MASKREG, 0x0000, INA_IMU_ADDRESS); // NOTA: En este registro se puede configurar el funcionamiento del ALERT Pin.
}


void INA226_MCU_init(){
	// Configuración del Configuration Register (Averaging Mode, VBUS & VSHUNT Convertion Time, Modo de Operación)
	INA226_write_register(CONFIGREG, 0x4127, INA_MCU_ADDRESS); // 0 | 100 | 000 | 100 | 100 | 111	-> RST = 0 | Reservado | AVG = 000 (Number of Averages = 1) | VBUSCT = 100 (CT = 1.1ms) | VSHUNT = 100 (CT = 1.1ms) | MODE = 111 (Shunt & Bus Continuous)

	// Configuración del Calibration Register (Resolución del Current Register dependiendo la resistencia de Shunt)
	INA226_write_register(CALREG, 0x6400, INA_MCU_ADDRESS); // MCU MAX (estimado): 500uA (para uso normal) -> Current_LSB = 20nA/bit -> CAL = 25600 = 0x6400h.
															// NOTA: MCU MAX (estimado): 30mA (para transmit mode) -> Current_LSB = 1uA/bit -> CAL = 512 = 0x0200h.

	// Configuración del MASK/ENABLE Register (Pin Alert: Read/Write)
	INA226_write_register(MASKREG, 0x0000, INA_MCU_ADDRESS); // NOTA: En este registro se puede configurar el funcionamiento del ALERT Pin.
}

