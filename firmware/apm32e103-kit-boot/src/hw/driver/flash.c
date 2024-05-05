#include "flash.h"
#include "qspi.h"
#include "spi_flash.h"
#include "cli.h"


#define FLASH_ADDR                0x8000000
#define FLASH_MAX_BANK            1
#define FLASH_MAX_SECTOR          256
#define FLASH_WRITE_SIZE          4
#define FLASH_SECTOR_SIZE         2048




#ifdef _USE_HW_CLI
static void cliFlash(cli_args_t *args);
#endif
static bool flashInSector(uint16_t sector_num, uint32_t addr, uint32_t length);





bool flashInit(void)
{

  logPrintf("[OK] flashInit()\n");

  FMC_Unlock();
  FMC_ClearStatusFlag(FMC_FLAG_OC | FMC_FLAG_PE | FMC_FLAG_WPE);

#ifdef _USE_HW_CLI
  cliAdd("flash", cliFlash);
#endif
  return true;
}

bool flashInSector(uint16_t sector_num, uint32_t addr, uint32_t length)
{
  bool ret = false;

  uint32_t sector_start;
  uint32_t sector_end;
  uint32_t flash_start;
  uint32_t flash_end;


  sector_start = FLASH_ADDR + (sector_num * FLASH_SECTOR_SIZE);
  sector_end   = sector_start + FLASH_SECTOR_SIZE - 1;
  flash_start  = addr;
  flash_end    = addr + length - 1;


  if (sector_start >= flash_start && sector_start <= flash_end)
  {
    ret = true;
  }

  if (sector_end >= flash_start && sector_end <= flash_end)
  {
    ret = true;
  }

  if (flash_start >= sector_start && flash_start <= sector_end)
  {
    ret = true;
  }

  if (flash_end >= sector_start && flash_end <= sector_end)
  {
    ret = true;
  }

  return ret;
}

bool flashErase(uint32_t addr, uint32_t length)
{
  bool ret = false;

  int32_t start_sector = -1;
  int32_t end_sector = -1;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiErase(addr - qspiGetAddr(), length);
    return ret;
  }
#endif
#ifdef _USE_HW_SPI_FLASH
  if (addr >= spiFlashGetAddr() && addr < (spiFlashGetAddr() + spiFlashGetLength()))
  {
    ret = spiFlashErase(addr - spiFlashGetAddr(), length);
    return ret;
  }
#endif

  start_sector = -1;
  end_sector = -1;

  for (int i=0; i<FLASH_MAX_SECTOR; i++)
  {
    if (flashInSector(i, addr, length) == true)
    {
      if (start_sector < 0)
      {
        start_sector = i;
      }
      end_sector = i;
    }
  }


  // FMC_Unlock();
  // FMC_ClearStatusFlag(FMC_FLAG_OC | FMC_FLAG_PE | FMC_FLAG_WPE);

  if (start_sector >= 0)
  {
    FMC_STATUS_T status = FMC_STATUS_COMPLETE;
    uint16_t num_sectors;
    uint32_t sector_addr;

    num_sectors = (end_sector - start_sector) + 1;

    for (int i=0; i<num_sectors; i++)
    {
      sector_addr = FLASH_ADDR + (start_sector + i) * FLASH_SECTOR_SIZE;
      status = FMC_ErasePage(sector_addr);
      if (status != FMC_STATUS_COMPLETE)
      {
        break;
      }
    }
    if (status == FMC_STATUS_COMPLETE)
    {
      ret = true;
    }
  }

  // FMC_WaitForLastOperation(100);
  // FMC_Lock();

  return ret;
}

bool flashWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool         ret = true;
  uint32_t     index;
  uint32_t     write_length;
  uint32_t     write_addr;
  uint32_t     write_data;
  uint8_t      buf[FLASH_WRITE_SIZE];
  uint32_t     offset;
  FMC_STATUS_T status;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiWrite(addr - qspiGetAddr(), p_data, length);
    return ret;
  }
#endif
#ifdef _USE_HW_SPI_FLASH
  if (addr >= spiFlashGetAddr() && addr < (spiFlashGetAddr() + spiFlashGetLength()))
  {
    ret = spiFlashWrite(addr - spiFlashGetAddr(), p_data, length);
    return ret;
  }
#endif

  // FMC_Unlock();
  // FMC_ClearStatusFlag(FMC_FLAG_OC | FMC_FLAG_PE | FMC_FLAG_WPE);

  index = 0;
  offset = addr%FLASH_WRITE_SIZE;

  if (offset != 0 || length < FLASH_WRITE_SIZE)
  {
    write_addr = addr - offset;
    memcpy(&buf[0], (void *)write_addr, FLASH_WRITE_SIZE);
    memcpy(&buf[offset], &p_data[0], constrain(FLASH_WRITE_SIZE-offset, 0, length));
    
    memcpy(&write_data, buf, FLASH_WRITE_SIZE);
    status = FMC_ProgramWord(write_addr, write_data);
    if (status != FMC_STATUS_COMPLETE)
    {
      FMC_Lock();
      return false;
    }

    if (offset == 0 && length < FLASH_WRITE_SIZE)
    {
      index += length;
    }
    else
    {
      index += (FLASH_WRITE_SIZE - offset);
    }
  }


  while(index < length)
  {
    write_addr = addr + index;
    write_length = constrain(length - index, 0, FLASH_WRITE_SIZE);

    memcpy(&write_data, &p_data[index], FLASH_WRITE_SIZE);
    status = FMC_ProgramWord(write_addr, write_data);
    if (status != FMC_STATUS_COMPLETE)
    {
      ret = false;
      break;
    }

    index += write_length;

    if ((length - index) > 0 && (length - index) < FLASH_WRITE_SIZE)
    {
      offset = length - index;
      write_addr = addr + index;
      memcpy(&buf[0], (void *)write_addr, FLASH_WRITE_SIZE);
      memcpy(&buf[0], &p_data[index], offset);

      memcpy(&write_data, buf, FLASH_WRITE_SIZE);
      status = FMC_ProgramWord(write_addr, write_data);
      if (status != FMC_STATUS_COMPLETE)
      {
        return false;
      }
      break;
    }
  }

  // FMC_WaitForLastOperation(100);
  // FMC_Lock();

  return ret;
}

bool flashRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint8_t *p_byte = (uint8_t *)addr;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiRead(addr - qspiGetAddr(), p_data, length);
    return ret;
  }
#endif
#ifdef _USE_HW_SPI_FLASH
  if (addr >= spiFlashGetAddr() && addr < (spiFlashGetAddr() + spiFlashGetLength()))
  {
    ret = spiFlashRead(addr - spiFlashGetAddr(), p_data, length);
    return ret;
  }
#endif

  for (int i=0; i<length; i++)
  {
    p_data[i] = p_byte[i];
  }

  return ret;
}





#ifdef _USE_HW_CLI
void cliFlash(cli_args_t *args)
{
  bool ret = false;
  uint32_t i;
  uint32_t addr;
  uint32_t length;
  uint32_t pre_time;
  bool flash_ret;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("flash addr  : 0x%X\n", FLASH_ADDR);
    #ifdef _USE_HW_QSPI
    cliPrintf("qspi  addr  : 0x%X\n", qspiGetAddr());
    #endif
    cliPrintf("spi   addr  : 0x%X\n", spiFlashGetAddr());
    
    ret = true;
  }

  if(args->argc == 3 && args->isStr(0, "read"))
  {
    uint8_t data;

    addr   = (uint32_t)args->getData(1);
    length = (uint32_t)args->getData(2);

    for (i=0; i<length; i++)
    {
      flash_ret = flashRead(addr+i, &data, 1);

      if (flash_ret == true)
      {
        cliPrintf( "addr : 0x%X\t 0x%02X\n", addr+i, data);
      }
      else
      {
        cliPrintf( "addr : 0x%X\t Fail\n", addr+i);
      }
    }

    ret = true;
  }
    
  if(args->argc == 3 && args->isStr(0, "erase"))
  {
    addr   = (uint32_t)args->getData(1);
    length = (uint32_t)args->getData(2);

    pre_time = millis();
    flash_ret = flashErase(addr, length);

    cliPrintf( "addr : 0x%X\t len : %d %d ms\n", addr, length, (millis()-pre_time));
    if (flash_ret)
    {
      cliPrintf("OK\n");
    }
    else
    {
      cliPrintf("FAIL\n");
    }

    ret = true;
  }
    
  if(args->argc == 3 && args->isStr(0, "write"))
  {
    uint32_t data;

    addr = (uint32_t)args->getData(1);
    data = (uint32_t)args->getData(2);

    pre_time = millis();
    flash_ret = flashWrite(addr, (uint8_t *)&data, 4);

    cliPrintf( "addr : 0x%X\t 0x%X %dms\n", addr, data, millis()-pre_time);
    if (flash_ret)
    {
      cliPrintf("OK\n");
    }
    else
    {
      cliPrintf("FAIL\n");
    }

    ret = true;
  }

  if(args->argc == 3 && args->isStr(0, "check"))
  {
    uint32_t data = 0;
    uint32_t block = 4;


    addr    = (uint32_t)args->getData(1);
    length  = (uint32_t)args->getData(2);
    length -= (length % block);

    do
    {
      cliPrintf("flashErase()..");
      if (flashErase(addr, length) == false)
      {
        cliPrintf("Fail\n");
        break;
      }
      cliPrintf("OK\n");

      cliPrintf("flashWrite()..");
      for (uint32_t i=0; i<length; i+=block)
      {
        data = i;
        if (flashWrite(addr + i, (uint8_t *)&data, block) == false)
        {
          cliPrintf("Fail %d\n", i);
          break;
        }
      }
      cliPrintf("OK\n");

      cliPrintf("flashRead() ..");
      for (uint32_t i=0; i<length; i+=block)
      {
        data = 0;
        if (flashRead(addr + i, (uint8_t *)&data, block) == false)
        {
          cliPrintf("Fail %d\n", i);
          break;
        }
        if (data != i)
        {
          cliPrintf("Check Fail %d\n", i);
          break;
        }
      }  
      cliPrintf("OK\n");


      cliPrintf("flashErase()..");
      if (flashErase(addr, length) == false)
      {
        cliPrintf("Fail\n");
        break;
      }
      cliPrintf("OK\n");  
    } while (0);
    
    ret = true;
  }


  if (ret == false)
  {
    cliPrintf( "flash info\n");
    cliPrintf( "flash read  [addr] [length]\n");
    cliPrintf( "flash erase [addr] [length]\n");
    cliPrintf( "flash write [addr] [data]\n");
    cliPrintf( "flash check [addr] [length]\n");
  }
}
#endif