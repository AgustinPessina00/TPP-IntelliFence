
#ifndef INA226_CLASS_H
#define INA226_CLASS_H

#include "ina226_registers.h"

// === Máscaras de campos del registro de configuración ===
#define INA226_CFG_AVG_MASK     (0b111 << CFG_AVG0)
#define INA226_CFG_VBUSCT_MASK  (0b111 << CFG_VBUSCT0)
#define INA226_CFG_VSHCT_MASK   (0b111 << CFG_VSHCT0)
#define INA226_CFG_MODE_MASK    (0b111 << CFG_MODE0)

enum class Ina226Averaging : uint16_t {
    AVG_1    = (0b000 << CFG_AVG0),
    AVG_4    = (0b001 << CFG_AVG0),
    AVG_16   = (0b010 << CFG_AVG0),
    AVG_64   = (0b011 << CFG_AVG0),
    AVG_128  = (0b100 << CFG_AVG0),
    AVG_256  = (0b101 << CFG_AVG0),
    AVG_512  = (0b110 << CFG_AVG0),
    AVG_1024 = (0b111 << CFG_AVG0)
};

enum class Ina226ConvTime : uint16_t {
    CT_140US   = (0b000 << CFG_VBUSCT0),
    CT_204US   = (0b001 << CFG_VBUSCT0),
    CT_332US   = (0b010 << CFG_VBUSCT0),
    CT_588US   = (0b011 << CFG_VBUSCT0),
    CT_1_1MS   = (0b100 << CFG_VBUSCT0),
    CT_2_116MS = (0b101 << CFG_VBUSCT0),
    CT_4_156MS = (0b110 << CFG_VBUSCT0),
    CT_8_244MS = (0b111 << CFG_VBUSCT0)
};

enum class Ina226Mode : uint16_t {
    POWER_DOWN            = (0b000 << CFG_MODE0),
    SHUNT_TRIGGERED       = (0b001 << CFG_MODE0),
    BUS_TRIGGERED         = (0b010 << CFG_MODE0),
    SHUNT_BUS_TRIGGERED   = (0b011 << CFG_MODE0),
    SHUNT_CONTINUOUS      = (0b101 << CFG_MODE0),
    BUS_CONTINUOUS        = (0b110 << CFG_MODE0),
    SHUNT_BUS_CONTINUOUS  = (0b111 << CFG_MODE0)
};

//const float currentLsb = 0.001f;

class Ina226 {
public:
	Ina226(uint8_t i2cAddr, float rShunt, float currentLSB, Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode);
    bool readShuntVoltage_mV(float &voltage);
    bool readBusVoltage_mV(float &voltage);
    bool readCurrent_mA(float &current);
    bool readPower_mW(float &power);

private:
    bool configure(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode);
    uint16_t setConfiguration(Ina226Averaging avg, Ina226ConvTime vbusCt, Ina226ConvTime vshCt, Ina226Mode mode);
    bool writeRegister(uint8_t reg, uint16_t value);
    bool readRegister(uint8_t reg, uint16_t &value);
    uint16_t calculateCalibration(float lsb);

private:
    uint8_t i2cAddr;
    float rShunt;
    float currentLSB; //PESSI: Agrego currentLSB para calibrar.
};



#endif // INA226_CLASS_H
