
/* Includes ------------------------------------------------------------------*/

#include "lsm6dso.h"
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/



Lsm6dso::Lsdm6dso(uint8_t i2cAddr, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr, Lsm6dsoWakeThs wakeThs, Lsdm6dsoWakeDur wakeDur, Lsdm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur){
	this->i2cAddr = i2cAddr;
	this->odrAcc = odrAcc;
	this->fsAcc = fsAcc;
	this->odrGyr = odrGyr;
	this->fsGyr = fsGyr;
	this->wakeThs = wakeThs;
	this->wakeDur = wakeDur;
	this->wakeWeight = wakeWeight;
	this->sleepDur = sleepDur;
}

bool Lsm6dso::writeRegister(uint8_t reg, uint16_t value) {
    uint8_t data[2] = { static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
    return HAL_I2C_Mem_Write_DMA(&hi2c2, i2cAddr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, 2) == HAL_OK;
}

