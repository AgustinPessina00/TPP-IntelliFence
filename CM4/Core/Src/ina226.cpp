/* Includes ------------------------------------------------------------------*/

#include "ina226.h"

/* Private includes ----------------------------------------------------------*/

Ina226::Ina226(I2C_HandleTypeDef *hi2c, uint8_t i2cAddr, float rShunt, float currentLSB, Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode){
    this->hi2c = hi2c;	
    this->i2cAddr = i2cAddr;
    this->rShunt = rShunt;
    this->currentLSB = currentLSB;	//PESSI: Cargo el currentLSB.
    configure(avg, vbusCt, vshCt, mode);
}

bool Ina226::configure(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode) {
    uint16_t config = setConfiguration(avg, vbusCt, vshCt, mode);
    //float lsb = 0.001f;
    uint16_t cal = calculateCalibration();
    if (!writeRegister(REG_CFG, config)) return false;

    return writeRegister(REG_CALIB, cal);
}


uint16_t Ina226::setConfiguration(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode) {
    return (static_cast<uint16_t>(avg) & INA226_CFG_AVG_MASK) |
           (static_cast<uint16_t>(vbusCt) & INA226_CFG_VBUSCT_MASK) |
           (static_cast<uint16_t>(vshCt) & INA226_CFG_VSHCT_MASK) |
           (static_cast<uint16_t>(mode) & INA226_CFG_MODE_MASK);
}

uint16_t Ina226::calculateCalibration() {
    float cal = 0.00512f / (currentLSB * rShunt);
    return static_cast<uint16_t>(cal);
}

HAL_StatusTypeDef Ina226::readShuntVoltage_mV() {
    int16_t raw;
    if (!readRegister(REG_VSHUNT, reinterpret_cast<uint16_t&>(raw)))
        return HAL_ERROR;
    this->shuntVoltage = raw * 2.5e-3f;
    return HAL_OK;
}

HAL_StatusTypeDef Ina226::readBusVoltage_mV() {
    uint16_t raw;
    if (!readRegister(REG_VBUS, raw))
        return HAL_ERROR;
    this->busVoltage = raw * 1.25f;
    return HAL_OK;
}

HAL_StatusTypeDef Ina226::readCurrent_mA() {
    int16_t raw;
    if (!readRegister(REG_CURRENT, reinterpret_cast<uint16_t&>(raw))) 
        return HAL_ERROR;
    this->current = raw * this->currentLSB * 1000.0f;
    return HAL_OK;
}

HAL_StatusTypeDef Ina226::readPower_mW() {
    uint16_t raw;
    if (!readRegister(REG_POWER, raw))
        return HAL_ERROR;
    this->power = raw * 25.0f * this->currentLSB * 1000.0f;
    return HAL_OK;
}

bool Ina226::writeRegister(uint8_t reg, uint16_t value) {
    uint8_t data[2] = { static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
    return HAL_I2C_Mem_Write_DMA(hi2c, i2cAddr, reg, I2C_MEMADD_SIZE_8BIT, data, 2) == HAL_OK; //PESSI: Revisar el corrimiento del addr.
}

bool Ina226::readRegister(uint8_t reg, uint16_t &value) {
    uint8_t data[2] = {0};
    if (HAL_I2C_Mem_Read_DMA(hi2c, i2cAddr, reg, I2C_MEMADD_SIZE_8BIT, data, 2) != HAL_OK) //PESSI: Revisar el corrimiento del addr.
        return false;
    value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    return true;
}
