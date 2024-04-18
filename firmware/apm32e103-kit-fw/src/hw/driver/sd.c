#include "sd.h"



#ifdef _USE_HW_SD
#include "gpio.h"
#include "cli.h"
#include "hal/hal_sd.h"




static bool is_init = false;
static bool is_detected = false;
static uint8_t is_try = 0;
static sd_state_t sd_state = SDCARD_IDLE;
static const uint8_t _PIN_GPIO_SDCARD_DETECT = (uint8_t)SD_DETECT;

SD_HandleTypeDef hsd;



#if CLI_USE(HW_SD)
static void cliSd(cli_args_t *args);
#endif





bool sdInit(void)
{
  bool ret = false;


  is_detected = sdIsDetected();

  if (is_detected == true)
  {
    if (HAL_SD_Init(&hsd) == HAL_OK)
    {
      ret = true;
    }
  }

  is_init = ret;


  static bool is_reinit = false;

  if (is_reinit == false)
  {
    logPrintf("[%s] sdInit()\n", ret ? "OK":"NG");   
    if (is_detected == true)
    {
      uint32_t sdmmc_clk = 0;

      logPrintf("     sdcard found\n");
      logPrintf("     clk : %d MHz\n", sdmmc_clk/1000000);
    }
    else
    {
      logPrintf("     sdcard not found\n");   
    }
  }
    

#if CLI_USE(HW_SD)
  if (is_reinit == false)
    cliAdd("sd", cliSd);
#endif

  is_reinit = true;
  
  return ret;
}

bool sdReInit(void)
{
  bool ret = false;

  HAL_SD_DeInit(&hsd);
  if (HAL_SD_Init(&hsd) == HAL_OK)
  {
    ret = true;
  }

  is_init = ret;

  return ret;
}

bool sdDeInit(void)
{
  bool ret = false;

  if (is_init == true)
  {
    is_init = false;
    if (HAL_SD_DeInit(&hsd) == HAL_OK)
    {
      ret = true;
    }
  }

  return ret;
}

bool sdIsInit(void)
{
  return is_init;
}

bool sdIsDetected(void)
{
  if (gpioPinRead(_PIN_GPIO_SDCARD_DETECT) == _DEF_LOW)
  {
    is_detected = true;
  }
  else
  {
    is_detected = false;
  }

  return is_detected;
}

sd_state_t sdUpdate(void)
{
  sd_state_t ret_state = SDCARD_IDLE;
  static uint32_t pre_time;


  switch(sd_state)
  {
    case SDCARD_IDLE:
      if (sdIsDetected() == true)
      {
        if (is_init)
        {
          sd_state = SDCARD_CONNECTED;
        }
        else
        {
          sd_state = SDCARD_CONNECTTING;
          pre_time = millis();
        }
      }
      else
      {
        is_init = false;
        sd_state  = SDCARD_DISCONNECTED;
        ret_state = SDCARD_DISCONNECTED;
      }
      break;

    case SDCARD_CONNECTTING:
      if (millis()-pre_time >= 100)
      {
        if (sdReInit())
        {
          sd_state  = SDCARD_CONNECTED;
          ret_state = SDCARD_CONNECTED;
        }
        else
        {
          sd_state = SDCARD_IDLE;
          is_try++;

          if (is_try >= 3)
          {
            sd_state = SDCARD_ERROR;
          }
        }
      }
      break;

    case SDCARD_CONNECTED:
      if (sdIsDetected() != true)
      {
        is_try = 0;
        sd_state = SDCARD_IDLE;
      }
      break;

    case SDCARD_DISCONNECTED:
      if (sdIsDetected() == true)
      {
        sd_state = SDCARD_IDLE;
      }
      break;

    case SDCARD_ERROR:
      break;
  }

  return ret_state;
}

bool sdGetInfo(sd_info_t *p_info)
{
  bool ret = false;
  sd_info_t *p_sd_info = (sd_info_t *)p_info;

  HAL_SD_CardInfoTypeDef card_info;


  if (is_init == true)
  {
    HAL_SD_GetCardInfo(&hsd, &card_info);

    p_sd_info->card_type          = card_info.CardType;
    p_sd_info->card_version       = card_info.CardVersion;
    p_sd_info->card_class         = card_info.Class;
    p_sd_info->rel_card_Add       = card_info.RelCardAdd;
    p_sd_info->block_numbers      = card_info.BlockNbr;
    p_sd_info->block_size         = card_info.BlockSize;
    p_sd_info->log_block_numbers  = card_info.LogBlockNbr;
    p_sd_info->log_block_size     = card_info.LogBlockSize;
    p_sd_info->card_size          =  (uint32_t)((uint64_t)p_sd_info->block_numbers * (uint64_t)p_sd_info->block_size / (uint64_t)1024 / (uint64_t)1024);
    ret = true;
  }

  return ret;
}

bool sdIsBusy(void)
{
  bool is_busy;

  if (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER )
  {
    is_busy = false;
  }
  else
  {
    is_busy = true;
  }

  return is_busy;
}

bool sdIsReady(uint32_t timeout)
{
  uint32_t pre_time;

  pre_time = millis();

  while(millis() - pre_time < timeout)
  {
    if (sdIsBusy() == false)
    {
      return true;
    }
  }

  return false;
}

bool sdReadBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms)
{
  bool ret = false;

  if (is_init == false) return false;

  if(HAL_SD_ReadBlocks(&hsd, (uint8_t *)p_data, block_addr, num_of_blocks, timeout_ms) == HAL_OK)
  {
    ret = true;
  }
  return ret;
}

bool sdWriteBlocks(uint32_t block_addr, uint8_t *p_data, uint32_t num_of_blocks, uint32_t timeout_ms)
{
  bool ret = false;

  if (is_init == false) return false;


  if(HAL_SD_WriteBlocks(&hsd, (uint8_t *)p_data, block_addr, num_of_blocks, timeout_ms) == HAL_OK)
  {
    ret = true;
  }

  return ret;
}

bool sdEraseBlocks(uint32_t start_addr, uint32_t end_addr)
{
  bool ret = false;

  if (is_init == false) return false;

  if(HAL_SD_Erase(&hsd, start_addr, end_addr) == HAL_OK)
  {
    ret = true;
  }

  return ret;
}


#if CLI_USE(HW_SD)
void cliSd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    sd_info_t sd_info;

    cliPrintf("sd init      : %d\n", is_init);
    cliPrintf("sd connected : %d\n", is_detected);

    if (is_init == true)
    {
      if (sdGetInfo(&sd_info) == true)
      {
        cliPrintf("   card_type            : %d\n", sd_info.card_type);
        cliPrintf("   card_version         : %d\n", sd_info.card_version);
        cliPrintf("   card_class           : %d\n", sd_info.card_class);
        cliPrintf("   rel_card_Add         : %d\n", sd_info.rel_card_Add);
        cliPrintf("   block_numbers        : %d\n", sd_info.block_numbers);
        cliPrintf("   block_size           : %d\n", sd_info.block_size);
        cliPrintf("   log_block_numbers    : %d\n", sd_info.log_block_numbers);
        cliPrintf("   log_block_size       : %d\n", sd_info.log_block_size);
        cliPrintf("   card_size            : %d MB, %d.%d GB\n", sd_info.card_size, sd_info.card_size/1024, ((sd_info.card_size * 10)/1024) % 10);
      }
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read") == true)
  {
    uint32_t number;
    uint32_t buf[512/4];

    number = args->getData(1);

    if (sdReadBlocks(number, (uint8_t *)buf, 1, 100) == true)
    {
      for (int i=0; i<512/4; i++)
      {
        cliPrintf("%d:%04d : 0x%08X\n", number, i*4, buf[i]);
      }
    }
    else
    {
      cliPrintf("sdRead Fail\n");
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "speed-test") == true)
  {
    uint32_t number;
    uint32_t buf[512/4];
    uint32_t cnt;
    uint32_t pre_time;
    uint32_t exe_time;

    number = args->getData(1);

    cnt = 1024*1024 / 512;
    pre_time = millis();
    for (int i=0; i<cnt; i++)
    {
      if (sdReadBlocks(number, (uint8_t *)buf, 1, 100) == false)
      {
        cliPrintf("sdReadBlocks() Fail:%d\n", i);
        break;
      }
    }
    exe_time = millis()-pre_time;
    if (exe_time > 0)
    {
      cliPrintf("%d KB/sec\n", 1024 * 1000 / exe_time);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("sd info\n");

    if (is_init == true)
    {
      cliPrintf("sd read block_number\n");
      cliPrintf("sd speed-test\n");
    }
  }
}
#endif


#endif