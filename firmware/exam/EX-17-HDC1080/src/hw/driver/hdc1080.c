#include "hdc1080.h"


#ifdef _USE_HW_HDC1080
#include "i2c.h"
#include "cli.h"


#define REG_ADDR_TEMPERATRUE 0x00
#define REG_ADDR_HUMIDITY    0x01
#define REG_ADDR_CONFIG      0x02
#define REG_ADDR_SERIAL_ID1  0xFB
#define REG_ADDR_SERIAL_ID2  0xFC
#define REG_ADDR_SERIAL_ID3  0xFD
#define REG_ADDR_MENUF_ID    0xFE
#define REG_ADDR_DEVICE_ID   0xFF


#if CLI_USE(HW_HDC1080)
static void cliCmd(cli_args_t *args);
#endif
static bool regRead(uint16_t addr, uint16_t *p_data);
static bool regWrite(uint16_t addr, uint16_t data);
static bool hdc1080InitRegs(void);

static bool is_init = false;
static uint8_t i2c_ch = _DEF_I2C1;
static uint8_t i2c_addr = 0x40;





bool hdc1080Init(void)
{
  bool ret = true;


  if (!i2cIsBegin(i2c_ch))
  {
    ret = i2cBegin(i2c_ch, 100);
  }

  if (ret)
  {
    ret = i2cIsDeviceReady(i2c_ch, i2c_addr);
  }

  if (ret)
  {
    ret = hdc1080InitRegs();
  }

  is_init = ret;

  logPrintf("[%s] hdc1080Init()\n", ret ? "OK":"NG");
  if (ret)
  {
    uint16_t reg = 0;
    regRead(REG_ADDR_DEVICE_ID, &reg);    
    logPrintf("     ID : 0x%0X\n", reg);
  }
#if CLI_USE(HW_HDC1080)
  cliAdd("hdc1080", cliCmd);
#endif
  return ret;
}

bool hdc1080InitRegs(void)
{ 
  bool ret = true;


  ret = regWrite(REG_ADDR_CONFIG, 0x1000);
  return ret;
}

bool hdc1080Trigger(void)
{ 
  bool ret;

  ret = i2cWriteBytes(i2c_ch, i2c_addr, 0x00, NULL, 0, 5);
  return ret;
}

bool regRead(uint16_t addr, uint16_t *p_data)
{
  bool ret = true;    

  ret = i2cIsDeviceReady(i2c_ch, i2c_addr);
  if (ret)
  {
    uint8_t buf[2];

    ret = i2cReadBytes(i2c_ch, i2c_addr, addr, buf, 2, 2 * 5);
    if (ret)
      *p_data = (buf[0]<<8) | (buf[1]<<0);
  }

  return ret;
}

bool regReadMeasure(uint16_t *p_temp, uint16_t *p_humidity)
{
  bool ret = true;    

  ret = i2cIsDeviceReady(i2c_ch, i2c_addr);
  if (ret)
  {
    uint8_t buf[4];

    ret = i2cReadData(i2c_ch, i2c_addr, buf, 4, 4 * 5);
    if (ret)
    {
      *p_temp     = (buf[0] << 8) | (buf[1] << 0);
      *p_humidity = (buf[2] << 8) | (buf[3] << 0);
    }
  }

  return ret;
}



bool regWrite(uint16_t addr, uint16_t data)
{
  bool ret = true;    

  ret = i2cIsDeviceReady(i2c_ch, i2c_addr);
  if (ret)
  {
    uint8_t buf[2];

    buf[0] = data >> 8;
    buf[1] = data >> 0;

    ret = i2cWriteBytes(i2c_ch, i2c_addr, addr, buf, 2, 2 * 5);
  }

  return ret;
}

bool hdc1080GetInfo(hdc1080_info_t *p_info)
{
  bool ret = true;  
  bool is_measured = false;
  uint16_t reg_temp;
  uint16_t reg_humidity;
  static uint8_t state = 0;
  static uint32_t pre_time = 0;


  switch(state)
  {
    case 0:
      hdc1080Trigger();
      state = 1;
      pre_time = millis();
      break;    
  
    case 1:
      if (millis()-pre_time >= 20)
      {
        state = 2;
      }
      break;

    case 2:
      ret &= regReadMeasure(&reg_temp, &reg_humidity);
      if (ret)
      {
        int32_t temp;
        int32_t humidity;

        temp      = ((reg_temp * 165) * 100) / 65536;
        temp     -= (40 * 100);
        humidity  = ((reg_humidity * 100) * 100) / 65536;

        p_info->temp          = temp / 100;
        p_info->temp_x100     = temp;
        p_info->humidity      = humidity / 100;
        p_info->humidity_x100 = humidity;

        is_measured = true;
      }
      state = 0;
      break;
  }

  return is_measured;
}


#if CLI_USE(HW_HDC1080)
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("is_init : %d\n", is_init);
    
    uint16_t reg[6];

    regRead(REG_ADDR_CONFIG, &reg[0]);
    regRead(REG_ADDR_SERIAL_ID1, &reg[1]);
    regRead(REG_ADDR_SERIAL_ID2, &reg[2]);
    regRead(REG_ADDR_SERIAL_ID3, &reg[3]);
    regRead(REG_ADDR_MENUF_ID, &reg[4]);
    regRead(REG_ADDR_DEVICE_ID, &reg[5]);

    cliPrintf("CONFIG    : 0x%04X\n", reg[0]);
    cliPrintf("SERIAL_ID1: 0x%04X\n", reg[1]);
    cliPrintf("SERIAL_ID2: 0x%04X\n", reg[2]);
    cliPrintf("SERIAL_ID3: 0x%04X\n", reg[3]);
    cliPrintf("MENUF_ID  : 0x%04X\n", reg[4]);
    cliPrintf("DEVICE_ID : 0x%04X\n", reg[5]);

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "get") && args->isStr(1, "info"))
  {
    hdc1080_info_t info;


    while(cliKeepLoop())
    {
      if (hdc1080GetInfo(&info))
      {
        cliPrintf("temp     : %d.%d  ", info.temp_x100/100, info.temp_x100%100);
        cliPrintf("humidity : %d.%d%%\n", info.humidity_x100/100, info.humidity_x100%100);
      }
      delay(10);
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("hdc1080 info\n");
    cliPrintf("hdc1080 get info\n");
  }
}
#endif

#endif