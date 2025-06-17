
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

float_t Lsm6dso::lsm6dso_from_lsb_to_nsec(int16_t lsb)
{
  return ((float_t)lsb * 25000.0f);
}

/**
  * @brief  Read generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to read
  * @param  data  pointer to buffer that store the data read(ptr)
  * @param  len   number of consecutive register to read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t Lsm6dso::lsm6dso_read_reg(const stmdev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len){
  int32_t ret;

  if (ctx == nullptr)
  {
    return -1;
  }

  ret = ctx->read_reg(ctx->handle, reg, data, len);

  return ret;
}


/**
  * @brief  Linear acceleration output register.
  *         The value is expressed as a 16-bit word in two's complement.
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval             interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t Lsm6dso::lsm6dso_acceleration_raw_get (const stmdev_ctx_t *ctx, int16_t *val){
	uint8_t buff[6];
	int32_t ret;

	// Leemos 6 bytes desde el registro base de aceleración (OUTX_L_A)
	ret = lsm6dso_read_reg(ctx, REG_OUTX_L_A, buff, 6);

	/*
	val[0] = (int16_t)buff[1];
	val[0] = (val[0] * 256) + (int16_t)buff[0];
	val[1] = (int16_t)buff[3];
	val[1] = (val[1] * 256) + (int16_t)buff[2];
	val[2] = (int16_t)buff[5];
	val[2] = (val[2] * 256) + (int16_t)buff[4];
	*/

	val[0] = (int16_t)((int16_t)buff[1] << LSM6DSO_WORD_SHIFT | buff[0]);  // X
	val[1] = (int16_t)((int16_t)buff[3] << LSM6DSO_WORD_SHIFT | buff[2]);  // Y
	val[2] = (int16_t)((int16_t)buff[5] << LSM6DSO_WORD_SHIFT | buff[4]);  // Z

	return ret;
}

/**
  * @brief  Read data in engineering unit.
  *
  * @param  ctx     communication interface handler.(ptr)
  * @param  md      the sensor conversion parameters.(ptr)
  * @retval             interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t Lsm6dso::lsm6dso_data_get(const stmdev_ctx_t *ctx, const lsm6dso_md_t *md, lsm6dso_data_t *data) {
    uint8_t buff[14];
    int32_t ret = 0;

    /* read data */
     if (ctx != nullptr)
     {
       ret = lsm6dso_read_reg(ctx, REG_OUT_TEMP_L, buff, 14);
       if (ret != 0) { return ret; }
     }

    uint8_t j = 0;

    // Temperatura
    data->ui.heat.raw = (int16_t)((buff[j + 1] << LSM6DSO_WORD_SHIFT) | buff[j]);
    data->ui.heat.deg_c = lsm6dso_from_lsb_to_celsius(data->ui.heat.raw);
    j += 2;

    // Acelerómetro
    for (uint8_t i = 0; i < 3; ++i) {
        data->ui.xl.raw[i] = (int16_t)((buff[j + 1] << LSM6DSO_WORD_SHIFT) | buff[j]);
        j += 2;

        switch (md->ui.xl.fs) {
            case LSM6DSO_XL_UI_2g:
                data->ui.xl.mg[i] = lsm6dso_from_fs2_to_mg(data->ui.xl.raw[i]);
                break;
            case LSM6DSO_XL_UI_4g:
                data->ui.xl.mg[i] = lsm6dso_from_fs4_to_mg(data->ui.xl.raw[i]);
                break;
            case LSM6DSO_XL_UI_8g:
                data->ui.xl.mg[i] = lsm6dso_from_fs8_to_mg(data->ui.xl.raw[i]);
                break;
            case LSM6DSO_XL_UI_16g:
                data->ui.xl.mg[i] = lsm6dso_from_fs16_to_mg(data->ui.xl.raw[i]);
                break;
            default:
                data->ui.xl.mg[i] = 0.0f;
                break;
        }
    }

    return 0;
}

