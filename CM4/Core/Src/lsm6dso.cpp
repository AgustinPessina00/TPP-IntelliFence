/* Includes ------------------------------------------------------------------*/

#include "lsm6dso.h"
#include "../../Core/Inc/I2CManager.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Public methods ----------------------------------------------------------*/

Lsm6dso::Lsm6dso(uint8_t i2cAddr, Lsm6dsoI3C i3c, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr, Lsm6dsoWakeThs wakeThs, Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur)
    : i2cAddr(i2cAddr), i2cBus(nullptr), isInitialized(false)
{
  // Store configuration parameters for later initialization
  this->i3cConfig = i3c;
  this->odrAccConfig = odrAcc;
  this->fsAccConfig = fsAcc;
  this->odrGyrConfig = odrGyr;
  this->fsGyrConfig = fsGyr;
  this->wakeThsConfig = wakeThs;
  this->wakeDurConfig = wakeDur;
  this->wakeWeightConfig = wakeWeight;
  this->sleepDurConfig = sleepDur;
}

bool Lsm6dso::initialize() {
    // Ensure I2CManager is initialized
    if (!I2CManager::isInitialized()) {
        if (!I2CManager::initializeAll()) {
            return false;
        }
    }
    
    // Get reference to I2C2 bus (same as GPS and INA226)
    i2cBus = &I2CManager::getBus2();
    
    // ✅ SET INITIALIZED FLAG BEFORE CONFIGURE
    isInitialized = true;
    
    // Configure the LSM6DSO
    if (!configure(i3cConfig, odrAccConfig, fsAccConfig, odrGyrConfig, fsGyrConfig,
                   wakeThsConfig, wakeDurConfig, wakeWeightConfig, sleepDurConfig)) {
        isInitialized = false; // Reset flag if configuration fails
        return false;
    }
    
    return true;
}

I2CResult Lsm6dso::readAcceleration()
{
    if (!isInitialized) {
        return I2C_ERROR;
    }

    // Read 6 bytes from accelerometer output registers
    uint8_t buffer[6];
    I2CResult result = readRegisters(REG_OUTX_L_A, buffer, 6);
    
    if (result != I2C_OK) {
        return result;
    }
    
    // Convert to 16-bit signed values
    axRaw = (int16_t)((buffer[1] << 8) | buffer[0]);
    ayRaw = (int16_t)((buffer[3] << 8) | buffer[2]);
    azRaw = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    // Convert to mg based on full scale setting
    switch (fsAccConfig) {
        case Lsm6dsoFsAcc::FS_2G:
            ax = lsm6dso_from_fs2_to_mg(axRaw);
            ay = lsm6dso_from_fs2_to_mg(ayRaw);
            az = lsm6dso_from_fs2_to_mg(azRaw);
            break;
        case Lsm6dsoFsAcc::FS_4G:
            ax = lsm6dso_from_fs4_to_mg(axRaw);
            ay = lsm6dso_from_fs4_to_mg(ayRaw);
            az = lsm6dso_from_fs4_to_mg(azRaw);
            break;
        case Lsm6dsoFsAcc::FS_8G:
            ax = lsm6dso_from_fs8_to_mg(axRaw);
            ay = lsm6dso_from_fs8_to_mg(ayRaw);
            az = lsm6dso_from_fs8_to_mg(azRaw);
            break;
        case Lsm6dsoFsAcc::FS_16G:
            ax = lsm6dso_from_fs16_to_mg(axRaw);
            ay = lsm6dso_from_fs16_to_mg(ayRaw);
            az = lsm6dso_from_fs16_to_mg(azRaw);
            break;
        default:
            ax = ay = az = 0.0f;
            break;
    }

    return I2C_OK;
}

I2CResult Lsm6dso::readGyroscope()
{
    if (!isInitialized) {
        return I2C_ERROR;
    }

    // Read 6 bytes from gyroscope output registers
    uint8_t buffer[6];
    I2CResult result = readRegisters(REG_OUTX_L_G, buffer, 6);
    
    if (result != I2C_OK) {
        return result;
    }
    
    // Convert to 16-bit signed values
    gxRaw = (int16_t)((buffer[1] << 8) | buffer[0]);
    gyRaw = (int16_t)((buffer[3] << 8) | buffer[2]);
    gzRaw = (int16_t)((buffer[5] << 8) | buffer[4]);
    
    // Convert to mdps based on full scale setting
    switch (fsGyrConfig) {
        case Lsm6dsoFsGyr::FS_250DPS:
            gx = lsm6dso_from_fs250_to_mdps(gxRaw);
            gy = lsm6dso_from_fs250_to_mdps(gyRaw);
            gz = lsm6dso_from_fs250_to_mdps(gzRaw);
            break;
        case Lsm6dsoFsGyr::FS_500DPS:
            gx = lsm6dso_from_fs500_to_mdps(gxRaw);
            gy = lsm6dso_from_fs500_to_mdps(gyRaw);
            gz = lsm6dso_from_fs500_to_mdps(gzRaw);
            break;
        case Lsm6dsoFsGyr::FS_1KDPS:
            gx = lsm6dso_from_fs1000_to_mdps(gxRaw);
            gy = lsm6dso_from_fs1000_to_mdps(gyRaw);
            gz = lsm6dso_from_fs1000_to_mdps(gzRaw);
            break;
        case Lsm6dsoFsGyr::FS_2KDPS:
            gx = lsm6dso_from_fs2000_to_mdps(gxRaw);
            gy = lsm6dso_from_fs2000_to_mdps(gyRaw);
            gz = lsm6dso_from_fs2000_to_mdps(gzRaw);
            break;
        default:
            gx = gy = gz = 0.0f;
            break;
    }

    return I2C_OK;
}

I2CResult Lsm6dso::readTemperature()
{
    if (!isInitialized) {
        return I2C_ERROR;
    }

    // Read 2 bytes from temperature output registers
    uint8_t buffer[2];
    I2CResult result = readRegisters(REG_OUT_TEMP_L, buffer, 2);
    
    if (result != I2C_OK) {
        return result;
    }
    
    // Convert to 16-bit signed value
    tempRaw = (int16_t)((buffer[1] << 8) | buffer[0]);
    
    // Convert to Celsius
    temperature = lsm6dso_from_lsb_to_celsius(tempRaw);

    return I2C_OK;
}

I2CResult Lsm6dso::getWhoAmI(uint8_t& whoAmI)
{
    if (!isInitialized) {
        return I2C_ERROR;
    }

    return readRegister(REG_WHO_AM_I, whoAmI);
}

// TODO: CAMBIAR TODOS LOS NUMEROS MÁGICOS POR #DEFINE !!!

/* Private methods ----------------------------------------------------------*/

bool Lsm6dso::configure(Lsm6dsoI3C i3c, Lsm6dsoOdrAcc odrAcc, Lsm6dsoFsAcc fsAcc, Lsm6dsoOdrGyr odrGyr, Lsm6dsoFsGyr fsGyr,
	Lsm6dsoWakeThs wakeThs, Lsm6dsoWakeDur wakeDur, Lsm6dsoWakeWeight wakeWeight, Lsm6dsoSleepDur sleepDur) {

	uint8_t config;

    // Configure I3C
    config = setConfigurationREG_CTRL9_XL(i3c);
    if (writeRegister(REG_CTRL9_XL, config) != I2C_OK) return false;

    // Configure accelerometer (ODR + FS)
    config = setConfigurationREG_CTRL1_XL(odrAcc, fsAcc);
    if (writeRegister(REG_CTRL1_XL, config) != I2C_OK) return false;

    // Configure gyroscope (ODR + FS)
    config = setConfigurationREG_CTRL2_G(odrGyr, fsGyr);
    if (writeRegister(REG_CTRL2_G, config) != I2C_OK) return false;

    // Enable register address auto-increment
    config = setConfigurationREG_CTRL3_C(Lsm6dsoIfInc::ENABLED);
    if (writeRegister(REG_CTRL3_C, config) != I2C_OK) return false;

    return true;
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
	return (static_cast<uint8_t>(xlHm) & LSM6DSO_XL_HM_MODE_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL7_G(Lsm6dsoGHm gHm){
	return (static_cast<uint8_t>(gHm) & LSM6DSO_G_HM_MODE_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_CTRL9_XL(Lsm6dsoI3C i3c) {
	return (static_cast<uint8_t>(i3c) & LSM6DSO_I3C_DISABLE_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_TAP_CFG0(Lsm6dsoSlopeFilterEn sF){
	return (static_cast<uint8_t>(sF) & LSM6DSO_SLOPE_FDS_MASK);
}

uint8_t Lsm6dso::setConfigurationREG_TAP_CFG2(Lsm6dsoIntEn intEn, Lsm6dsoInActEn inActEn){
	return (static_cast<uint8_t>(intEn) & LSM6DSO_INT_EN_MASK) |
		   (static_cast<uint8_t>(inActEn) & LSM6DSO_INACT_MASK);
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


I2CResult Lsm6dso::writeRegister(uint8_t reg, uint8_t value) {
    if (!isInitialized || !i2cBus) {
        return I2C_ERROR;
    }
    
    return i2cBus->memWrite(i2cAddr, reg, 1, &value, 1, 100);
}

I2CResult Lsm6dso::readRegister(uint8_t reg, uint8_t& value) {
    if (!isInitialized || !i2cBus) {
        return I2C_ERROR;
    }
    
    return i2cBus->memRead(i2cAddr, reg, 1, &value, 1, 100);
}

I2CResult Lsm6dso::readRegisters(uint8_t reg, uint8_t* data, uint16_t length) {
    if (!isInitialized || !i2cBus || !data) {
        return I2C_ERROR;
    }
    
    return i2cBus->memRead(i2cAddr, reg, 1, data, length, 100);
}


// ---------------------------------------------------------------------------------------------

// ==== OUTPUTS ====

/**
  * @defgroup  LSM6DSO_Sensitivity
  * @brief     These functions convert raw-data into engineering units.
  * @{
  *
  */
float_t Lsm6dso::lsm6dso_from_fs2_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.061f;
}

float_t Lsm6dso::lsm6dso_from_fs4_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.122f;
}

float_t Lsm6dso::lsm6dso_from_fs8_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.244f;
}

float_t Lsm6dso::lsm6dso_from_fs16_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.488f;
}

float_t Lsm6dso::lsm6dso_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 256.0f) + 25.0f);
}

float_t Lsm6dso::lsm6dso_from_fs250_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) * 8.75f;
}

float_t Lsm6dso::lsm6dso_from_fs500_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) * 17.50f;
}

float_t Lsm6dso::lsm6dso_from_fs1000_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) * 35.0f;
}

float_t Lsm6dso::lsm6dso_from_fs2000_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) * 70.0f;
}



/**
  * @brief  Read generic device register
  *
  * @param  reg   register to read
  * @param  data  pointer to buffer that store the data read(ptr)
  * @param  len   number of consecutive register to read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */

// ----- VERSIÓN THREAD-SAFE I2C -----
I2CResult Lsm6dso::lsm6dso_read_reg(uint8_t reg, uint8_t *data, uint16_t len){
    return readRegisters(reg, data, len);
}


/**
  * @brief  Linear acceleration output register.
  *         The value is expressed as a 16-bit word in two's complement.
  *
  * @param  val   pointer to store data read
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */

// ----- VERSIÓN THREAD-SAFE I2C -----
I2CResult Lsm6dso::lsm6dso_acceleration_raw_get(int16_t *val) {
  uint8_t buff[6];
  I2CResult ret;

  // Leemos 6 bytes desde el registro base de aceleración (OUTX_L_A)
  ret = readRegisters(REG_OUTX_L_A, buff, 6);
  if (ret != I2C_OK)
    return ret;

  val[0] = (int16_t)((int16_t)buff[1] << LSM6DSO_WORD_SHIFT | buff[0]);  // X
  val[1] = (int16_t)((int16_t)buff[3] << LSM6DSO_WORD_SHIFT | buff[2]);  // Y
  val[2] = (int16_t)((int16_t)buff[5] << LSM6DSO_WORD_SHIFT | buff[4]);  // Z

  return I2C_OK;
}

// ----- VERSIÓN CON CTX -----
/**
  * @brief  Read data in engineering unit.
  *
  * @param  md      the sensor conversion parameters.(ptr)
  * @param  data    the sensor data.(ptr)
  * @retval             interface status (MANDATORY: return 0 -> no Error)
  *
  */


// ----- VERSIÓN THREAD-SAFE I2C -----
I2CResult Lsm6dso::lsm6dso_data_get(lsm6dso_md_t *md, lsm6dso_data_t *data) {
  uint8_t buff[14];

  // Leer los 14 bytes desde OUT_TEMP_L (2 de temp + 6 de acc + 6 de gyro)
  I2CResult ret = readRegisters(REG_OUT_TEMP_L, buff, 14);
  if (ret != I2C_OK) return ret;

  uint8_t j = 0;

  // Temperatura
  data->ui.heat.raw = (int16_t)((buff[j + 1] << LSM6DSO_WORD_SHIFT) | buff[j]);
  data->ui.heat.deg_c = lsm6dso_from_lsb_to_celsius(data->ui.heat.raw);
  j += 2;

  // Acelerómetro
  for (uint8_t i = 0; i < 3; i++) {
    data->ui.xl.raw[i] = (int16_t)((buff[j + 1] << LSM6DSO_WORD_SHIFT) | buff[j]);
    j += 2;

    switch (md->ui.xl.fs) {
      case Lsm6dsoFsXlUi::LSM6DSO_XL_UI_2g:
        data->ui.xl.mg[i] = lsm6dso_from_fs2_to_mg(data->ui.xl.raw[i]);
        break;
      case Lsm6dsoFsXlUi::LSM6DSO_XL_UI_4g:
        data->ui.xl.mg[i] = lsm6dso_from_fs4_to_mg(data->ui.xl.raw[i]);
        break;
      case Lsm6dsoFsXlUi::LSM6DSO_XL_UI_8g:
        data->ui.xl.mg[i] = lsm6dso_from_fs8_to_mg(data->ui.xl.raw[i]);
        break;
      case Lsm6dsoFsXlUi::LSM6DSO_XL_UI_16g:
        data->ui.xl.mg[i] = lsm6dso_from_fs16_to_mg(data->ui.xl.raw[i]);
        break;
      default:
        data->ui.xl.mg[i] = 0.0f;
        break;
    }
  }

  // Giroscopio (si hay espacio en el buffer)
  for (uint8_t i = 0; i < 3; i++) {
    data->ui.gy.raw[i] = (int16_t)((buff[j + 1] << LSM6DSO_WORD_SHIFT) | buff[j]);
    j += 2;

    // Convert based on current gyroscope full scale setting
    switch (fsGyrConfig) {
      case Lsm6dsoFsGyr::FS_250DPS:
        data->ui.gy.mdps[i] = lsm6dso_from_fs250_to_mdps(data->ui.gy.raw[i]);
        break;
      case Lsm6dsoFsGyr::FS_500DPS:
        data->ui.gy.mdps[i] = lsm6dso_from_fs500_to_mdps(data->ui.gy.raw[i]);
        break;
      case Lsm6dsoFsGyr::FS_1KDPS:
        data->ui.gy.mdps[i] = lsm6dso_from_fs1000_to_mdps(data->ui.gy.raw[i]);
        break;
      case Lsm6dsoFsGyr::FS_2KDPS:
        data->ui.gy.mdps[i] = lsm6dso_from_fs2000_to_mdps(data->ui.gy.raw[i]);
        break;
      default:
        data->ui.gy.mdps[i] = 0.0f;
        break;
    }
  }

  return I2C_OK;
}

void Lsm6dso::testIMU() {
    printf("[TEST IMU] Iniciando test de IMU thread-safe...\n");

    // Test WHO_AM_I
    uint8_t whoAmI;
    if (getWhoAmI(whoAmI) == I2C_OK) {
        printf("[TEST IMU] WHO_AM_I: 0x%02X %s\n", whoAmI, 
               (whoAmI == 0x6C) ? "(LSM6DSO - Correcto)" : "(ID no reconocido)");
    } else {
        printf("[TEST IMU] ERROR - No se pudo leer WHO_AM_I\n");
    }

    // Test accelerometer
    I2CResult result = readAcceleration();
    if (result == I2C_OK) {
        printf("[TEST IMU] Aceleracion: X=%.2f, Y=%.2f, Z=%.2f mg\n", ax, ay, az);
        printf("[TEST IMU] Aceleracion Raw: X=%d, Y=%d, Z=%d\n", axRaw, ayRaw, azRaw);
    } else {
        printf("[TEST IMU] ERROR - Fallo al leer aceleracion\n");
    }

    // Test gyroscope
    result = readGyroscope();
    if (result == I2C_OK) {
        printf("[TEST IMU] Giroscopio: X=%.2f, Y=%.2f, Z=%.2f mdps\n", gx, gy, gz);
        printf("[TEST IMU] Giroscopio Raw: X=%d, Y=%d, Z=%d\n", gxRaw, gyRaw, gzRaw);
    } else {
        printf("[TEST IMU] ERROR - Fallo al leer giroscopio\n");
    }

    // Test temperature
    result = readTemperature();
    if (result == I2C_OK) {
        printf("[TEST IMU] Temperatura: %.2f°C (Raw: %d)\n", temperature, tempRaw);
    } else {
        printf("[TEST IMU] ERROR - Fallo al leer temperatura\n");
    }
}
