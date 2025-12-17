/* Includes ------------------------------------------------------------------*/

#include "ina226.h"

/* Private includes ----------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Trivial constructor - does NOT access hardware, allocate memory, or block
Ina226::Ina226() 
    : i2cAddr(0), i2cBus(nullptr), rShunt(0.0f), currentLSB(0.0f), 
      initialized(false), avgConfig(Ina226Averaging::AVG_1),
      vbusCtConfig(Ina226ConvTime::CT_1_1MS), vshCtConfig(Ina226ConvTime::CT_1_1MS),
      modeConfig(Ina226Mode::SHUNT_BUS_CONTINUOUS) {
    // Constructor does nothing - initialization is explicit via init()
}

bool Ina226::init(uint8_t i2cAddr, float rShunt, float currentLSB, 
                  Ina226Averaging avg, Ina226ConvTime vbusCt, 
                  Ina226ConvTime vshCt, Ina226Mode mode) {
    if (initialized) {
        return true; // Already initialized
    }
    
    // Store parameters
    this->i2cAddr = i2cAddr;
    this->rShunt = rShunt;
    this->currentLSB = currentLSB;
    this->avgConfig = avg;
    this->vbusCtConfig = vbusCt;
    this->vshCtConfig = vshCt;
    this->modeConfig = mode;
    
    // Ensure I2CManager is initialized
    if (!I2CManager::isInitialized()) {
        if (!I2CManager::initializeAll()) {
            return false;
        }
    }
    
    // Get reference to I2C2 bus (same as GPS)
    i2cBus = &I2CManager::getBus2();
    if (!i2cBus) {
        return false;
    }
    
    // Configure the INA226 with provided parameters
    if (configure(avg, vbusCt, vshCt, mode) != I2C_OK) {
        return false;
    }
    
    initialized = true;
    return true;
}

I2CResult Ina226::configure(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode) {
    uint16_t config = setConfiguration(avg, vbusCt, vshCt, mode);
    //float lsb = 0.001f;
    uint16_t cal = calculateCalibration();
    I2CResult result = writeRegister(REG_CFG, config);
    if (result != I2C_OK)
        return result;

    result = writeRegister(REG_CALIB, cal);
    return result;
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

I2CResult Ina226::readShuntVoltage_mV() {
    if (!initialized) {
        return I2C_ERROR;
    }
    int16_t raw;
    I2CResult result = readRegister(REG_VSHUNT, reinterpret_cast<uint16_t&>(raw));
    if (result != I2C_OK)
        return result;
    this->shuntVoltage = raw * 2.5e-3f;
    return I2C_OK;
}

I2CResult Ina226::readBusVoltage_mV() {
    if (!initialized) {
        return I2C_ERROR;
    }
    uint16_t raw;
    I2CResult result = readRegister(REG_VBUS, raw);
    if (result != I2C_OK)
        return result;
    this->busVoltage = raw * 1.25f;
    return I2C_OK;
}

I2CResult Ina226::readCurrent_mA() {
    if (!initialized) {
        return I2C_ERROR;
    }
    int16_t raw;
    I2CResult result = readRegister(REG_CURRENT, reinterpret_cast<uint16_t&>(raw));
    if (result != I2C_OK) 
        return result;
    this->current = raw * this->currentLSB * 1000.0f;
    return I2C_OK;
}

I2CResult Ina226::readPower_mW() {
    if (!initialized) {
        return I2C_ERROR;
    }
    uint16_t raw;
    I2CResult result = readRegister(REG_POWER, raw);
    if (result != I2C_OK)
        return result;
    this->power = raw * 25.0f * this->currentLSB * 1000.0f;
    return I2C_OK;
}

I2CResult Ina226::writeRegister(uint8_t reg, uint16_t value) {
    uint8_t data[2] = { static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
    return i2cBus->memWrite(i2cAddr, reg, 1, data, 2, 100); // Sin shift
}

I2CResult Ina226::readRegister(uint8_t reg, uint16_t &value) {
    uint8_t data[2];
    
    I2CResult result = i2cBus->memRead(i2cAddr, reg, 1, data, 2, 100); // Sin shift
    
    if (result != I2C_OK) {
        return result;
    }
    
    value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    return I2C_OK;
}

void Ina226::testINA() {
    printf("[TEST INA] Iniciando test de sensores de corriente...\n");

    if(this->readCurrent_mA() == I2C_OK) {
    	printf("[TEST INA] Current: %.2f mA\n", this->current);
    }
    else {
    	printf("[TEST INA] read Current FAILED\n");
    }
    if(this->readPower_mW() == I2C_OK) {
        	printf("[TEST INA] Power: %.2f mW\n", this->power);
        }
    else {
    	printf("[TEST INA] read Power FAILED\n");
    }

    if(this->readBusVoltage_mV() == I2C_OK) {
        printf("[TEST INA] Bus Voltage: %.2f mV\n", this->busVoltage);
    }
    else {
    	printf("[TEST INA] read Bus Voltage FAILED\n");
    }

    if(this->readShuntVoltage_mV() == I2C_OK) {
        printf("[TEST INA] Shunt Voltage: %.2f mV\n", this->shuntVoltage);
    }
    else {
    	printf("[TEST INA] read Shunt Voltage FAILED\n");
    }
}
