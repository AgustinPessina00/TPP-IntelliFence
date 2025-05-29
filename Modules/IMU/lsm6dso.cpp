
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

uint8_t Lsm6dso::setConfigurationREG_CTRL1_XL(Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc) {
    return (static_cast<uint8_t>(odrAcc) & LSM6DSO_ODR_XL_MASK) |
           (static_cast<uint8_t>(fsAcc) & LSM6DSO_FS_XL_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL2_G(Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr){
	return (static_cast<uint8_t>(odrGyr) & LSM6DSO_ODR_G_MASK) |
	       (static_cast<uint8_t>(fsGyr) & LSM6DSO_FS_G_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL3_C(Lsm6dsoIfInc ifInc){
	return (static_cast<uint8_t>(ifInc) & LSM6DSO_IF_INC_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL6_C(Lsm6dsoXlHm xlHm){
	return (static_cast<uint8_t>(XlHm) & LSM6DSO_XL_HM_MODE_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL7_G(Lsm6dsoGHm gHm){
	return (static_cast<uint8_t>(GHm) & LSM6DSO_G_HM_MODE_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL9_XL(Lsm6dsoI3C i3c) {
	return (static_cast<uint8_t>(i3c) & LSM6DSO_I3C_DISABLE);
}

uint8_t Lsm6dso::setConfigurationREG_TAP_CFG0(Lsm6dsoSlopeFilterEn sF){
	return (static_cast<uint8_t>(SF) & LSM6DSO_SLOPE_FDS_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_TAP_CFG2(Lsm6dsoIntEn intEn, Lsm6dsoInactEn inActEn){
	return (static_cast<uint8_t>(IntEn) & LSM6DSO_INT_EN_MASK) |
		   (static_cast<uint8_t>(InActEn) & LSM6DSO_INACT_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_WAKE_UP_THS(Lsm6dsoWakeThs wakeThs){
	return (static_cast<uint8_t>(wakeThs) & LSM6DSO_WK_THS_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_WAKE_UP_DUR(Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur){
	return (static_cast<uint8_t>(wakeDur) & LSM6DSO_WK_DUR_MASK) |
		   (static_cast<uint8_t>(wakeWeight) & LSM6DSO_WK_DUR_THS_MASK) |
		   (static_cast<uint8_t>(sleepDur) & LSM6DSO_WK_DUR_SLP_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_MD1_CFG(Lsm6dsoIntWU intWU){
	return (static_cast<uint8_t>(intWU) & LSM6DSO_INT1_WU_MASK);
}


bool Lsm6dso::writeRegister(uint8_t reg, uint8_t value) {
	uint8_t data = value;
    return HAL_I2C_Mem_Write_DMA(&hi2c2, i2cAddr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1) == HAL_OK;
}

