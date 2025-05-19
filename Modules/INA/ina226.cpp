/* Includes ------------------------------------------------------------------*/

#include "ina226.h"
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/

extern I2C_HandleTypeDef hi2c2; //PESSI: Modifico el i2c que usamos.

Ina226::Ina226(uint8_t i2cAddr, float rShunt, float currentLSB, Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode){
    	this->i2cAddr = i2cAddr;
    	this->rShunt = rShunt;
    	this->currentLSB = currentLSB;	//PESSI: Cargo el currentLSB.
    	configure(avg, vbusCt, vshCt, mode);
}

bool Ina226::configure(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode) {
    uint16_t config = setConfiguration(avg, vbusCt, vshCt, mode);
    //float lsb = 0.001f;
    uint16_t cal = calculateCalibration(currentLSB);
    if (!writeRegister(REG_CFG, config)) return false;

    return writeRegister(REG_CALIB, cal);
}

bool Ina226::readShuntVoltage_mV(float &voltage) {
    int16_t raw;
    if (!readRegister(REG_SHUNT, reinterpret_cast<uint16_t&>(raw))) return false;
    voltage = raw * 2.5e-3f;
    return true;
}

bool Ina226::readBusVoltage_mV(float &voltage) {
    uint16_t raw;
    if (!readRegister(REG_BUS, raw)) return false;
    voltage = raw * 1.25f;
    return true;
}

bool Ina226::readCurrent_mA(float &current) {
    int16_t raw;
    if (!readRegister(REG_CURRENT, reinterpret_cast<uint16_t&>(raw))) return false;
    current = raw * currentLSB * 1000.0f;
    return true;
}

bool Ina226::readPower_mW(float &power) {
    uint16_t raw;
    if (!readRegister(REG_PWR, raw)) return false;
    power = raw * 25.0f * currentLSB * 1000.0f;
    return true;
}

uint16_t Ina226::setConfiguration(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode) {
    return (static_cast<uint16_t>(avg) & INA226_CFG_AVG_MASK) |
           (static_cast<uint16_t>(vbusCt) & INA226_CFG_VBUSCT_MASK) |
           (static_cast<uint16_t>(vshCt) & INA226_CFG_VSHCT_MASK) |
           (static_cast<uint16_t>(mode) & INA226_CFG_MODE_MASK);
}

bool Ina226::writeRegister(uint8_t reg, uint16_t value) {
    uint8_t data[2] = { static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
    return HAL_I2C_Mem_Write_DMA(&hi2c2, i2cAddr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, 2) == HAL_OK;
}

bool Ina226::readRegister(uint8_t reg, uint16_t &value) {
    uint8_t data[2] = {0};
    if (HAL_I2C_Mem_Read_DMA(&hi2c2, i2cAddr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, 2) != HAL_OK)
        return false;
    value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    return true;
}

uint16_t Ina226::calculateCalibration() {
    float cal = 0.00512f / (currentLSB * rShunt);
    return static_cast<uint16_t>(cal);
}

