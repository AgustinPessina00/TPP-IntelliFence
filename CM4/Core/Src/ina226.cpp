/* Includes ------------------------------------------------------------------*/

#include "ina226.h"

/* Private includes ----------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>

Ina226::Ina226(uint8_t i2cAddr, float rShunt, float currentLSB, 
               Ina226Averaging avg, Ina226ConvTime vbusCt, 
               Ina226ConvTime vshCt, Ina226Mode mode)
    : i2cAddr(i2cAddr), i2cBus(nullptr), rShunt(rShunt), currentLSB(currentLSB), 
      isInitialized(false) {
    // Constructor solo guarda parámetros
}

bool Ina226::initialize() {
    // Asegurar que I2CManager esté inicializado
    if (!I2CManager::isInitialized()) {
        if (!I2CManager::initializeAll()) {
            return false;
        }
    }
    
    // Obtener referencia al bus I2C2 (mismo que GPS)
    i2cBus = &I2CManager::getBus2();
    
    // Configurar el INA226 con valores por defecto
    if (configure(Ina226Averaging::AVG_1, Ina226ConvTime::CT_1_1MS, Ina226ConvTime::CT_1_1MS, Ina226Mode::SHUNT_BUS_CONTINUOUS) != I2C_OK) {
        return false;
    }
    
    isInitialized = true;
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
    int16_t raw;
    I2CResult result = readRegister(REG_VSHUNT, reinterpret_cast<uint16_t&>(raw));
    if (result != I2C_OK)
        return result;
    this->shuntVoltage = raw * 2.5e-3f;
    return I2C_OK;
}

I2CResult Ina226::readBusVoltage_mV() {
    uint16_t raw;
    I2CResult result = readRegister(REG_VBUS, raw);
    if (result != I2C_OK)
        return result;
    this->busVoltage = raw * 1.25f;
    return I2C_OK;
}

I2CResult Ina226::readCurrent_mA() {
    int16_t raw;
    I2CResult result = readRegister(REG_CURRENT, reinterpret_cast<uint16_t&>(raw));
    if (result != I2C_OK) 
        return result;
    this->current = raw * this->currentLSB * 1000.0f;
    return I2C_OK;
}

I2CResult Ina226::readPower_mW() {
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
