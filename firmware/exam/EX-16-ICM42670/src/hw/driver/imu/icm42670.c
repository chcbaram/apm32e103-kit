#include "imu/icm42670.h"


#ifdef _USE_HW_ICM42670
#include "i2c.h"
#include "cli.h"
#include "imu/icm42670_regs.h"



#define WHO_AM_I      ICM42670_WHO_AM_I
#define REG_WHO_AM_I  ICM42670_REG_WHO_AM_I


#if CLI_USE(HW_LED)
static void cliCmd(cli_args_t *args);
#endif
bool regReadByte(uint16_t addr, uint8_t *p_data);
bool regWriteByte(uint16_t addr, uint8_t data);
bool regRead(uint16_t addr, uint8_t *p_data, uint16_t length);
bool regWrite(uint16_t addr, uint8_t *p_data, uint16_t length);
static bool icm42670InitRegs(void);

static bool    is_init     = false;
static uint8_t i2c_ch      = HW_I2C_CH_IMU;
static uint8_t i2c_addr    = ICM42670_I2C_ADDR_GND;
static bool    is_found    = false;






bool icm42670Init(void)
{
  bool ret = true;


  if (!i2cIsBegin(i2c_ch))
  {
    ret = i2cBegin(i2c_ch, 400);
  }

  if (ret)
  {
    uint8_t data = 0;
   
    regReadByte(ICM42670_REG_WHO_AM_I, &data);
    if (data == WHO_AM_I)
    {
      icm42670InitRegs();
      is_found = true;
    }    
  }
  if (is_found)
    logPrintf("[OK] ICM42670 Found\n");
  else
    logPrintf("[NG] ICM42670 Not Found\n");

#if CLI_USE(HW_LED)
  cliAdd("icm42670", cliCmd);
#endif

  is_init = is_found;
  ret = is_init;
  
  return ret;
}

bool icm42670InitRegs(void)
{
  
  uint8_t data;


  data = 1 << ICM42670_FIFO_FLUSH_SHIFT;
  regWriteByte(ICM42670_REG_SIGNAL_PATH_RESET, data);
  delay(1);

  data = 1 << ICM42670_SOFT_RESET_DEVICE_CONFIG_SHIFT;
  regWriteByte(ICM42670_REG_SIGNAL_PATH_RESET, data);
  delay(10);


  data  = ICM42670_GYRO_ENABLE_LN_MODE  << ICM42670_GYRO_MODE_SHIFT;
  data |= ICM42670_ACCEL_ENABLE_LN_MODE << ICM42670_ACCEL_MODE_SHIFT;
  regWriteByte(ICM42670_REG_PWR_MGMT0, data);
  delay(50);


  data  = (ICM42670_INT_MODE_LATCHED         << ICM42670_INT1_MODE_SHIFT);
  data |= (ICM42670_INT_DRIVE_PUSH_PULL      << ICM42670_INT1_DRIVE_CIRCUIT_SHIFT);
  data |= (ICM42670_INT_POLARITY_ACTIVE_HIGH << ICM42670_INT1_POLARITY_SHIFT);
  data |= (ICM42670_INT_MODE_LATCHED         << ICM42670_INT2_MODE_SHIFT);
  data |= (ICM42670_INT_DRIVE_PUSH_PULL      << ICM42670_INT2_DRIVE_CIRCUIT_SHIFT);
  data |= (ICM42670_INT_POLARITY_ACTIVE_HIGH << ICM42670_INT2_POLARITY_SHIFT);
  regWriteByte(ICM42670_REG_INT_CONFIG, data);


  data = (1 << ICM42670_DRDY_INT1_EN_SHIFT);
  regWriteByte(ICM42670_REG_INT_SOURCE0, data);


  // GYRO
  //
  data  = ICM42670_GYRO_RANGE_2000DPS << ICM42670_GYRO_UI_FS_SEL_SHIFT;
  data |= ICM42670_GYRO_ODR_400HZ     << ICM42670_GYRO_ODR_SHIFT;
  regWriteByte(ICM42670_REG_GYRO_CONFIG0, data);

  data = ICM42670_GYRO_LFP_53HZ << ICM42670_GYRO_UI_FILT_BW_SHIFT;
  regWriteByte(ICM42670_REG_GYRO_CONFIG1, data);

  // ACCEL
  //
  data  = ICM42670_ACCEL_RANGE_2G  << ICM42670_ACCEL_UI_FS_SEL_SHIFT;
  data |= ICM42670_ACCEL_ODR_400HZ << ICM42670_ACCEL_ODR_SHIFT;
  regWriteByte(ICM42670_REG_ACCEL_CONFIG0, data);

  data  = ICM42670_ACCEL_AVG_2X   << ICM42670_ACCEL_UI_AVG_SHIFT;
  data |= ICM42670_ACCEL_LFP_53HZ << ICM42670_ACCEL_UI_FILT_BW_SHIFT;
  regWriteByte(ICM42670_REG_ACCEL_CONFIG1, data);

  return true;
}

bool icm42670GetInfo(icm42670_info_t *p_info)
{
  bool ret = true;
  uint8_t buf[14];
  
  if (is_found == false)
    return false;
  
  p_info->timestamp = micros();
  ret = regRead(ICM42670_REG_TEMP_DATA1, buf, sizeof(buf));

  p_info->acc_x = (buf[2]<<8) | (buf[3]<<0);
  p_info->acc_y = (buf[4]<<8) | (buf[5]<<0);
  p_info->acc_z = (buf[6]<<8) | (buf[7]<<0);

  p_info->gyro_x = (buf[8]<<8) | (buf[9]<<0);
  p_info->gyro_y = (buf[10]<<8) | (buf[11]<<0);
  p_info->gyro_z = (buf[12]<<8) | (buf[13]<<0);

  p_info->temp = (buf[0]<<8) | (buf[1]<<0);

  p_info->acc_scale = 16384;
  p_info->gyro_scale = 164;
  
  return ret;
}

bool regReadByte(uint16_t addr, uint8_t *p_data)
{
  bool ret;  

  ret = regRead(addr, p_data, 1);
  return ret;
}

bool regWriteByte(uint16_t addr, uint8_t data)
{
  bool ret;  

  ret = regWrite(addr, &data, 1);
  return ret;
}

bool regRead(uint16_t addr, uint8_t *p_data, uint16_t length)
{
  bool ret = true;  

  ret = i2cIsDeviceReady(i2c_ch, i2c_addr);
  if (ret)
  {
    ret = i2cReadBytes(i2c_ch, i2c_addr, addr, p_data, length, length * 5);
  }

  return ret;
}

bool regWrite(uint16_t addr, uint8_t *p_data, uint16_t length)
{
  bool ret = true;  

  ret = i2cIsDeviceReady(i2c_ch, i2c_addr);
  if (ret)
  {
    ret = i2cWriteBytes(i2c_ch, i2c_addr, addr, p_data, length, length * 5);
  }

  return ret;
}

#ifdef _USE_HW_CLI

void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    uint8_t data = 0;

    regReadByte(REG_WHO_AM_I, &data);
    
    cliPrintf("is init  : %d\n", is_init);
    cliPrintf("WHO_AM_I : 0x%02X\n", data);
    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "reg") == true)
  {
    uint16_t addr;
    uint8_t data;
    uint8_t len;

    addr = args->getData(1);
    len  = args->getData(2);


    for (int i=0; i<len; i++)
    {
      regReadByte(addr + i, &data);
      cliPrintf("0x%02X : 0x%02X\n", addr + i, data);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "get") && args->isStr(1, "info"))
  {
    icm42670_info_t info;


    while(cliKeepLoop())
    {
      if (icm42670GetInfo(&info))
      {
        cliPrintf("ax=%-5d ay=%-5d az=%-5d, ", info.acc_x, info.acc_y, info.acc_z);
        cliPrintf("gx=%-5d gy=%-5d gz=%-5d\n", info.gyro_x, info.gyro_y, info.gyro_z);
      }
      else
      {
        cliPrintf("icm42670GetInfo() Err\n");
        break;
      }
      delay(10);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("icm42670 info\n");
    cliPrintf("icm42670 reg addr len\n");
    cliPrintf("icm42670 get info\n");
  }
}


#endif


#endif
