
/* Includes ------------------------------------------------------------------*/

#include "lsm6dso.h"
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/



Lsm6dso::Lsm6dso(uint8_t i2cAddr, Lsm6dsoI3C i3c, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr,
		Lsm6dsoWakeThs wakeThs, Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur){
	this->i2cAddr = i2cAddr;
	configure(odrAcc, fsAcc, odrGyr, fsGyr, wakeThs, wakeDur, wakeWeight, sleepDur);
}

bool Lsm6dso::configure(Lsm6dsoI3C i3c, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr,
		Lsm6dsoWakeThs wakeThs, Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur) {

    uint8_t config = setConfigurationREG_CTRL9_XL(i3c);
    if (!writeRegister(REG_CTRL9_XL, config)) return false;

    uint8_t config = setConfigurationREG_CTRL1_XL(odrAcc, fsAcc);
    if (!writeRegister(REG_CTRL1_XL, config)) return false;

    return 0;//writeRegister(REG_CALIB, cal);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL9_XL(Lsm6dsoI3C i3c) {
	return (static_cast<uint8_t>(i3c) & LSM6DSO_I3C_DISABLE);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL1_XL(Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc) {
    return (static_cast<uint8_t>(odrAcc) & LSM6DSO_ODR_XL_MASK) |
           (static_cast<uint8_t>(fsAcc) & LSM6DSO_FS_XL_MASK);
}

bool Lsm6dso::writeRegister(uint8_t reg, uint8_t value) {
	uint8_t data = value;
    return HAL_I2C_Mem_Write_DMA(&hi2c2, i2cAddr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1) == HAL_OK;
}

