
#include "hal_sd.h"


SDIO_Config_T     sdioConfigStructure;
SDIO_DataConfig_T sdioDataConfigStructure;
SDIO_CMDConfig_T  sdioCmdConfigStructure;

/* public variables */
/* SD write and read buffer */

#if defined(__CC_ARM) /*!< Arm Compiler */
__align(4) uint8_t SDIO_DATA_BUFFER[512];
__align(4) uint32_t *tempbuff;

#elif defined(__ICCARM__) || defined(__GNUC__)
uint8_t   SDIO_DATA_BUFFER[512];
uint32_t *tempbuff;

#endif


static uint8_t  s_sdCardType = SDIO_STD_CAPACITY_SD_CARD_V1_1; /* SD card type variable */
static uint32_t s_csdTab[4];                                   /* SD CSD register table */
static uint32_t s_cidTab[4];                                   /* SD CID register table */
static uint32_t s_rca = 0;                                     /* SD s_rca register */


static uint8_t      s_deviceMode    = SD_POLLING_MODE; /* SDIO device work mode */
static uint8_t      s_stopCondition = 0;               /* Flag for send SD stop command */
volatile SD_ERROR_T g_transferError = SD_OK;           /* Flag for data transmission(DMA) error */
volatile uint8_t    g_transferEnd   = 0;               /* Flag for data transmission(DMA) ended */
static SD_CardInfo_T       SDCardInfo;                        /* SD card info structure */



/* private function prototypes */
SD_ERROR_T CmdError(void);
SD_ERROR_T CmdResp1Error(uint8_t cmd);
SD_ERROR_T CmdResp2Error(void);
SD_ERROR_T CmdResp3Error(void);
SD_ERROR_T CmdResp6Error(uint8_t cmd, uint16_t *prca);
SD_ERROR_T CmdResp7Error(void);

SD_ERROR_T SD_EnWideBus(uint8_t enx);
SD_ERROR_T SD_FindSCR(uint16_t rca, uint32_t *pscr);
SD_ERROR_T IsCardProgramming(uint8_t *pstatus);
SD_ERROR_T SD_PowerON(void);
SD_ERROR_T SD_PowerOFF(void);
SD_ERROR_T SD_InitializeCards(void);
SD_ERROR_T SD_GetCardInfo(SD_CardInfo_T *cardInfo);
SD_ERROR_T SD_EnableWideBusOperation(uint32_t wmode);
SD_ERROR_T SD_SetDeviceMode(SD_DEVICE_MODE_T mode);
SD_ERROR_T SD_CardSelect(uint32_t addr);
SD_ERROR_T SD_SendStatus(uint32_t *pcardstatus);

SD_ERROR_T SD_ReadBlock(uint8_t *buf, long long addr, uint16_t blockSize);
SD_ERROR_T SD_ReadMultiBlocks(uint8_t *buf, long long addr, uint16_t blockSize, uint32_t nblks, uint32_t timeout);
SD_ERROR_T SD_WriteBlock(uint8_t *buf, long long addr, uint16_t blockSize);
SD_ERROR_T SD_WriteMultiBlocks(uint8_t *buf, long long addr, uint16_t blockSize, uint32_t nblks, uint32_t timeout);

uint8_t Convert_FromBytes2PowerofTwo(uint16_t NumberOfBytes);
void    SDIO_ClockConfig(uint8_t clkDiv);
void    SD_DMA_Config(uint32_t *memoryBaseAddr, uint32_t bufSize, DMA_DIR_T dir);




/*!
 * @brief       Show SD card informations.
 *
 * @param       None
 *
 * @retval      None
 *
 */
#if 0
void SD_ShowCardInfo(void)
{
  switch (SDCardInfo.CardType)
  {
    case SDIO_STD_CAPACITY_SD_CARD_V1_1:
      printf("Card Type:SDSC V1.1\r\n");
      break;

    case SDIO_STD_CAPACITY_SD_CARD_V2_0:
      printf("Card Type:SDSC V2.0\r\n");
      break;

    case SDIO_HIGH_CAPACITY_SD_CARD:
      printf("Card Type:SDHC V2.0\r\n");
      break;

    case SDIO_MULTIMEDIA_CARD:
      printf("Card Type:MMC Card\r\n");
      break;
  }
  /* Manufacturer ID */
  printf("Card ManufacturerID:%d\r\n", SDCardInfo.SD_cid.ManufacturerID);

  /* Ralative card address */
  printf("Card s_rca:%d\r\n", SDCardInfo.RCA);

  /* Card capacity */
  printf("Card Capacity:%d MB\r\n", (uint32_t)(SDCardInfo.CardCapacity >> 20));

  /* Card block size */
  printf("Card BlockSize:%d\r\n\r\n", SDCardInfo.CardBlockSize);
}
#endif

/*!
 * @brief       Reset SDIO register.
 *
 * @param       None
 *
 * @retval      None
 *
 */
static void SDIO_Register_Reset(void)
{
  SDIO->PWRCTRL  = 0x00000000;
  SDIO->CLKCTRL  = 0x00000000;
  SDIO->ARG      = 0x00000000;
  SDIO->CMD      = 0x00000000;
  SDIO->DATATIME = 0x00000000;
  SDIO->DATALEN  = 0x00000000;
  SDIO->DCTRL    = 0x00000000;
  SDIO->ICF      = 0x00C007FF;
  SDIO->MASK     = 0x00000000;
}

/*!
 * @brief       SD Init.
 *
 * @param       None
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_Init(void)
{
  GPIO_Config_T GPIO_InitStructure;

  SD_ERROR_T errorStatus = SD_OK;

  /* Set SDIO clock div  */
  uint8_t clkDiv = 0;

  /* Enable the GPIO and DMA2 Clock */
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOC | RCM_APB2_PERIPH_GPIOD);
  RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA2);

  /* Enable the SDIO Clock */
  RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_SDIO);

  /* Configure the GPIO pin */
  GPIO_InitStructure.pin   = SDIO_D0_PIN | SDIO_D1_PIN | SDIO_D2_PIN | SDIO_D3_PIN | SDIO_CK_PIN;
  GPIO_InitStructure.mode  = GPIO_MODE_AF_PP;
  GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
  GPIO_Config(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.pin = SDIO_CMD_PIN;
  GPIO_Config(GPIOD, &GPIO_InitStructure);

  /* Reset SDIO register to default value */
  SDIO_Register_Reset();

  NVIC_EnableIRQRequest(SDIO_IRQn, 0, 0);

  /* power on SD card */
  errorStatus = SD_PowerON();

  if (errorStatus == SD_OK)
  {
    /* Initialize SD card */
    errorStatus = SD_InitializeCards();
  }

  if (errorStatus == SD_OK)
  {
    /* Get SD card info */
    errorStatus = SD_GetCardInfo(&SDCardInfo);
  }

  if (errorStatus == SD_OK)
  {
    /* Select and enable SD card */
    errorStatus = SD_CardSelect((uint32_t)(SDCardInfo.RCA << 16));
  }

  if (errorStatus == SD_OK)
  {
    /* Set SDIO bus wide as 4B */
    errorStatus = SD_EnableWideBusOperation(SDIO_BUSWIDE_4B);
  }
  /* Set SDIO clock */
  if ((errorStatus == SD_OK) || (SDIO_MULTIMEDIA_CARD == s_sdCardType))
  {
    /* Card type is V1.1 or V2.0 */
    if (SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1 || SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0)
    {
      /* Set SDIO clock to 48 / 4 = 12MHz */
      clkDiv = SDIO_TRANSFER_CLK_DIV + 2;
    }
    /* Card type is SDHC or else */
    else
    {
      /* Set SDIO clock to 48 / 2 = 24MHz */
      clkDiv = SDIO_TRANSFER_CLK_DIV;
    }

    /*
      Set SDIO clock
      SDIO_CK = SDIOCLK / [clkdiv+2] , SDIOCLK fixed 48MHz
    */
    SDIO_ClockConfig(clkDiv);

    /* Set device to DMA mode */
    errorStatus = SD_SetDeviceMode(SD_DMA_MODE);
  }

  return errorStatus;
}

/*!
 * @brief       Init clock of SDIO
 *
 * @param       clkDiv: Clock frequency division coefficient
 *
 * @retval      SD error code
 *
 */
void SDIO_ClockConfig(uint8_t clkDiv)
{
  uint32_t tmpreg = SDIO->CLKCTRL;

  tmpreg        &= 0XFFFFFF00;
  tmpreg        |= clkDiv;
  SDIO->CLKCTRL  = tmpreg;
}

/*!
 * @brief       Power on SD card. Query all card device from SDIO interface.
 *              And check voltage value and set clock.
 *
 * @param       None
 *
 * @retval      SD error code
 *
 * @note        If DMA mode is used, the transfer rate can be reached to 48MHz. But may be have same error happend.
 *
 */
SD_ERROR_T SD_PowerON(void)
{
  uint8_t    i            = 0;
  SD_ERROR_T errorStatus  = SD_OK;
  uint32_t   response     = 0;
  uint32_t   count        = 0;
  uint32_t   validVoltage = 0;
  uint32_t   sdType       = SD_STD_CAPACITY;

  /* The clock frequency can not be set than 400KHz in initalization */
  /* HCLK = 120MHz, SDIOCLK = 120MHz, SDIO_CK = HCLK/(118 + 2) = 400 KHz */
  sdioConfigStructure.clockDiv  = SDIO_INIT_CLK_DIV;
  sdioConfigStructure.clockEdge = SDIO_CLOCK_EDGE_RISING;
  /* Directly use HCLK to divide the frequency to get SDIO CLOCK */
  sdioConfigStructure.clockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  /* Do not off power in idle */
  sdioConfigStructure.clockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  sdioConfigStructure.busWide             = SDIO_BUSWIDE_1B;
  sdioConfigStructure.hardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  SDIO_Config(&sdioConfigStructure);

  /* Check power state of SD card */
  SDIO_ConfigPowerState(SDIO_POWER_STATE_ON);
  /* Enable SDIO clock */
  SDIO_EnableClock();
  /* Wait SD card power on */
  delay(10);

  /* Send 74 clocks to wait for the completion of the internal operation of the SD card */
  for (i = 0; i < 74; i++)
  {
    /* Send CMD0 to goto idle stage */
    sdioCmdConfigStructure.argument = 0x0;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_GO_IDLE_STATE;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_NO;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait the response */
    errorStatus = CmdError();

    if (errorStatus == SD_OK)
    {
      break;
    }
  }
  if (errorStatus)
  {
    return errorStatus;
  }

  /* Send CMD8 to check SD card interface characteristic */
  sdioCmdConfigStructure.argument = SD_CHECK_PATTERN;
  sdioCmdConfigStructure.cmdIndex = SDIO_SEND_IF_COND;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait the R7 response */
  errorStatus = CmdResp7Error();

  /* If the R7 response normal */
  if (errorStatus == SD_OK)
  {
    s_sdCardType = SDIO_STD_CAPACITY_SD_CARD_V2_0;

    sdType = SD_HIGH_CAPACITY;
  }

  /* Send CMD55 command before ACMD command */
  sdioCmdConfigStructure.argument = 0x00;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_APP_CMD;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait the R1 response */
  errorStatus = CmdResp1Error(SD_CMD_APP_CMD);

  /* SD V2.0 or SD V1.1 */
  if (errorStatus == SD_OK)
  {
    /* Send CMD55 + ACMD41 */
    while ((!validVoltage) && (count < SD_MAX_VOLT_TRIAL))
    {
      sdioCmdConfigStructure.argument = 0x00;
      sdioCmdConfigStructure.cmdIndex = SD_CMD_APP_CMD;
      sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
      sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
      sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
      SDIO_TxCommand(&sdioCmdConfigStructure);

      /* Wait the R1 response */
      errorStatus = CmdResp1Error(SD_CMD_APP_CMD);

      if (errorStatus != SD_OK)
      {
        return errorStatus;
      }

      /*
         Send ACMD41 command.
         The command argument consists of the supported voltage range and HCS position.
         The HCS position can be use to judge SD card type(SDSC or SDHC).
      */
      sdioCmdConfigStructure.argument = SD_VOLTAGE_WINDOW_SD | sdType;
      sdioCmdConfigStructure.cmdIndex = SD_CMD_SD_APP_OP_COND;
      sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
      sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
      sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
      SDIO_TxCommand(&sdioCmdConfigStructure);

      /* Wait the R3 response */
      errorStatus = CmdResp3Error();

      if (errorStatus != SD_OK)
      {
        return errorStatus;
      }

      /* Get the response */
      response = SDIO_ReadResponse(SDIO_RES1);

      /* Judge the power on finish or not */
      validVoltage = (((response >> 31) == 1) ? 1 : 0);

      count++;
    }

    if (count >= SD_MAX_VOLT_TRIAL)
    {
      /* Exceed voltage range */
      errorStatus = SD_INVALID_VOLTRANGE;

      return errorStatus;
    }

    if (response &= SD_HIGH_CAPACITY)
    {
      s_sdCardType = SDIO_HIGH_CAPACITY_SD_CARD;
    }
  }
  /* MMC Card */
  else
  {
    while ((!validVoltage) && (count < SD_MAX_VOLT_TRIAL))
    {
      /* Send CMD1 */
      sdioCmdConfigStructure.argument = SD_VOLTAGE_WINDOW_MMC;
      sdioCmdConfigStructure.cmdIndex = SD_CMD_SEND_OP_COND;
      sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
      sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
      sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
      SDIO_TxCommand(&sdioCmdConfigStructure);

      /* Wait the R3 response */
      errorStatus = CmdResp3Error();

      if (errorStatus != SD_OK)
      {
        return errorStatus;
      }
      /* Get the response */
      response = SDIO_ReadResponse(SDIO_RES1);

      validVoltage = (((response >> 31) == 1) ? 1 : 0);
      count++;
    }
    if (count >= SD_MAX_VOLT_TRIAL)
    {
      errorStatus = SD_INVALID_VOLTRANGE;
      return errorStatus;
    }

    s_sdCardType = SDIO_MULTIMEDIA_CARD;
  }

  return (errorStatus);
}

/*!
 * @brief       Power off SD card.
 *
 * @param       None
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_PowerOFF(void)
{
  SDIO_ConfigPowerState(SDIO_POWER_STATE_OFF);

  return SD_OK;
}

/*!
 * @brief       Initialize SD card and let it into ready status
 *
 * @param       None
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_InitializeCards(void)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint16_t   rca         = 0x01;

  /* Check the power state of SD card */
  if (SDIO_ReadPowerState() == SDIO_POWER_STATE_OFF)
  {
    errorStatus = SD_REQUEST_NOT_APPLICABLE;
    return (errorStatus);
  }
  /* If the card is not secure digital io card */
  if (SDIO_SECURE_DIGITAL_IO_CARD != s_sdCardType)
  {
    /* Send CMD2 */
    sdioCmdConfigStructure.argument = 0x0;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_ALL_SEND_CID;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_LONG;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait the R2 response */
    errorStatus = CmdResp2Error();

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }

    /* Get CID */
    s_cidTab[0] = SDIO->RES1;
    s_cidTab[1] = SDIO->RES2;
    s_cidTab[2] = SDIO->RES3;
    s_cidTab[3] = SDIO->RES4;
  }

  if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == s_sdCardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == s_sdCardType)
      || (SDIO_SECURE_DIGITAL_IO_COMBO_CARD == s_sdCardType) || (SDIO_HIGH_CAPACITY_SD_CARD == s_sdCardType))
  {
    /* Send CMD3 */
    sdioCmdConfigStructure.argument = 0x00;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_REL_ADDR;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait the R6 response */
    errorStatus = CmdResp6Error(SD_CMD_SET_REL_ADDR, &rca);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
  }
  if (SDIO_MULTIMEDIA_CARD == s_sdCardType)
  {
    /* Send CMD3 */
    sdioCmdConfigStructure.argument = (uint32_t)(rca << 16);
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_REL_ADDR;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait the R2 response */
    errorStatus = CmdResp2Error();

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
  }

  /* If the card is not secure digital io card */
  if (SDIO_SECURE_DIGITAL_IO_CARD != s_sdCardType)
  {
    s_rca = rca;

    /* Send CMD9 + rca */
    sdioCmdConfigStructure.argument = (uint32_t)(rca << 16);
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SEND_CSD;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_LONG;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait the R2 response */
    errorStatus = CmdResp2Error();

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }

    /* Get CSD */
    s_csdTab[0] = SDIO->RES1;
    s_csdTab[1] = SDIO->RES2;
    s_csdTab[2] = SDIO->RES3;
    s_csdTab[3] = SDIO->RES4;
  }

  return SD_OK;
}

/*!
 * @brief       Get SD card informations
 *
 * @param       cardInfo: The card information save structure
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_GetCardInfo(SD_CardInfo_T *cardInfo)
{
  SD_ERROR_T errorStatus = SD_OK;

  uint8_t tmp = 0;

  /* Card type */
  cardInfo->CardType = (uint8_t)s_sdCardType;
  /* Card RCA value */
  cardInfo->RCA = (uint16_t)s_rca;
  /* Card CSD structure */
  tmp                        = (uint8_t)((s_csdTab[0] & 0xFF000000) >> 24);
  cardInfo->SD_csd.CSDStruct = (tmp & 0xC0) >> 6;

  /* Card system special version and reserved */
  cardInfo->SD_csd.SysSpecVersion = (tmp & 0x3C) >> 2;
  cardInfo->SD_csd.Reserved1      = tmp & 0x03;

  /* Card read speed 1 */
  tmp                   = (uint8_t)((s_csdTab[0] & 0x00FF0000) >> 16);
  cardInfo->SD_csd.TAAC = tmp;
  /* Card read speed 2 */
  tmp                   = (uint8_t)((s_csdTab[0] & 0x0000FF00) >> 8);
  cardInfo->SD_csd.NSAC = tmp;
  /* Card transfer speed */
  tmp                            = (uint8_t)(s_csdTab[0] & 0x000000FF);
  cardInfo->SD_csd.MaxBusClkFrec = tmp;

  /* Card command class */
  tmp                               = (uint8_t)((s_csdTab[1] & 0xFF000000) >> 24);
  cardInfo->SD_csd.CardComdClasses  = tmp << 4;
  tmp                               = (uint8_t)((s_csdTab[1] & 0x00FF0000) >> 16);
  cardInfo->SD_csd.CardComdClasses |= (tmp & 0xF0) >> 4;

  /* Maximum read data lenght */
  cardInfo->SD_csd.RdBlockLen = tmp & 0x0F;
  tmp                         = (uint8_t)((s_csdTab[1] & 0x0000FF00) >> 8);

  /* permission block read */
  cardInfo->SD_csd.PartBlockRead = (tmp & 0x80) >> 7;

  /* write block miss align */
  cardInfo->SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
  /* read block miss align */
  cardInfo->SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;

  /* Card reserved */
  cardInfo->SD_csd.DSRImpl   = (tmp & 0x10) >> 4;
  cardInfo->SD_csd.Reserved2 = 0;

  if ((s_sdCardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (s_sdCardType == SDIO_STD_CAPACITY_SD_CARD_V2_0)
      || (SDIO_MULTIMEDIA_CARD == s_sdCardType))
  {
    /* Device size */
    cardInfo->SD_csd.DeviceSize  = (tmp & 0x03) << 10;
    tmp                          = (uint8_t)(s_csdTab[1] & 0x000000FF);
    cardInfo->SD_csd.DeviceSize |= (tmp) << 2;
    tmp                          = (uint8_t)((s_csdTab[2] & 0xFF000000) >> 24);
    cardInfo->SD_csd.DeviceSize |= (tmp & 0xC0) >> 6;

    /* Max read current VDD */
    cardInfo->SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
    cardInfo->SD_csd.MaxRdCurrentVDDMax = (tmp & 0x07);

    /* Max write current VDD */
    tmp                                 = (uint8_t)((s_csdTab[2] & 0x00FF0000) >> 16);
    cardInfo->SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
    cardInfo->SD_csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
    cardInfo->SD_csd.DeviceSizeMul      = (tmp & 0x03) << 1;

    tmp                             = (uint8_t)((s_csdTab[2] & 0x0000FF00) >> 8);
    cardInfo->SD_csd.DeviceSizeMul |= (tmp & 0x80) >> 7;
    /* Card capacity */
    cardInfo->CardCapacity  = (cardInfo->SD_csd.DeviceSize + 1);
    cardInfo->CardCapacity *= (1 << (cardInfo->SD_csd.DeviceSizeMul + 2));
    /* Card block size */
    cardInfo->CardBlockSize  = 1 << (cardInfo->SD_csd.RdBlockLen);
    cardInfo->CardCapacity  *= cardInfo->CardBlockSize;
  }
  else if (s_sdCardType == SDIO_HIGH_CAPACITY_SD_CARD)
  {
    tmp                          = (uint8_t)(s_csdTab[1] & 0x000000FF);
    cardInfo->SD_csd.DeviceSize  = (tmp & 0x3F) << 16;
    tmp                          = (uint8_t)((s_csdTab[2] & 0xFF000000) >> 24);
    cardInfo->SD_csd.DeviceSize |= (tmp << 8);
    tmp                          = (uint8_t)((s_csdTab[2] & 0x00FF0000) >> 16);
    cardInfo->SD_csd.DeviceSize |= (tmp);
    tmp                          = (uint8_t)((s_csdTab[2] & 0x0000FF00) >> 8);
    cardInfo->CardCapacity       = (long long)(cardInfo->SD_csd.DeviceSize + 1) * 512 * 1024;
    cardInfo->CardBlockSize      = 512;
  }
  cardInfo->SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
  cardInfo->SD_csd.EraseGrMul  = (tmp & 0x3F) << 1;

  tmp                               = (uint8_t)(s_csdTab[2] & 0x000000FF);
  cardInfo->SD_csd.EraseGrMul      |= (tmp & 0x80) >> 7;
  cardInfo->SD_csd.WrProtectGrSize  = (tmp & 0x7F);

  tmp                                = (uint8_t)((s_csdTab[3] & 0xFF000000) >> 24);
  cardInfo->SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
  cardInfo->SD_csd.ManDeflECC        = (tmp & 0x60) >> 5;
  cardInfo->SD_csd.WrSpeedFact       = (tmp & 0x1C) >> 2;
  cardInfo->SD_csd.MaxWrBlockLen     = (tmp & 0x03) << 2;

  tmp                                   = (uint8_t)((s_csdTab[3] & 0x00FF0000) >> 16);
  cardInfo->SD_csd.MaxWrBlockLen       |= (tmp & 0xC0) >> 6;
  cardInfo->SD_csd.WriteBlockPaPartial  = (tmp & 0x20) >> 5;
  cardInfo->SD_csd.Reserved3            = 0;
  cardInfo->SD_csd.ContentProtectAppli  = (tmp & 0x01);

  tmp                               = (uint8_t)((s_csdTab[3] & 0x0000FF00) >> 8);
  cardInfo->SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
  cardInfo->SD_csd.CopyFlag         = (tmp & 0x40) >> 6;
  cardInfo->SD_csd.PermWrProtect    = (tmp & 0x20) >> 5;
  cardInfo->SD_csd.TempWrProtect    = (tmp & 0x10) >> 4;
  cardInfo->SD_csd.FileFormat       = (tmp & 0x0C) >> 2;
  cardInfo->SD_csd.ECC              = (tmp & 0x03);

  tmp                        = (uint8_t)(s_csdTab[3] & 0x000000FF);
  cardInfo->SD_csd.CSD_CRC   = (tmp & 0xFE) >> 1;
  cardInfo->SD_csd.Reserved4 = 1;

  tmp                             = (uint8_t)((s_cidTab[0] & 0xFF000000) >> 24);
  cardInfo->SD_cid.ManufacturerID = tmp;

  tmp                          = (uint8_t)((s_cidTab[0] & 0x00FF0000) >> 16);
  cardInfo->SD_cid.OEM_AppliID = tmp << 8;

  tmp                           = (uint8_t)((s_cidTab[0] & 0x000000FF00) >> 8);
  cardInfo->SD_cid.OEM_AppliID |= tmp;

  tmp                        = (uint8_t)(s_cidTab[0] & 0x000000FF);
  cardInfo->SD_cid.ProdName1 = tmp << 24;

  tmp                         = (uint8_t)((s_cidTab[1] & 0xFF000000) >> 24);
  cardInfo->SD_cid.ProdName1 |= tmp << 16;

  tmp                         = (uint8_t)((s_cidTab[1] & 0x00FF0000) >> 16);
  cardInfo->SD_cid.ProdName1 |= tmp << 8;

  tmp                         = (uint8_t)((s_cidTab[1] & 0x0000FF00) >> 8);
  cardInfo->SD_cid.ProdName1 |= tmp;

  tmp                        = (uint8_t)(s_cidTab[1] & 0x000000FF);
  cardInfo->SD_cid.ProdName2 = tmp;

  tmp                      = (uint8_t)((s_cidTab[2] & 0xFF000000) >> 24);
  cardInfo->SD_cid.ProdRev = tmp;

  tmp                     = (uint8_t)((s_cidTab[2] & 0x00FF0000) >> 16);
  cardInfo->SD_cid.ProdSN = tmp << 24;

  tmp                      = (uint8_t)((s_cidTab[2] & 0x0000FF00) >> 8);
  cardInfo->SD_cid.ProdSN |= tmp << 16;

  tmp                      = (uint8_t)(s_cidTab[2] & 0x000000FF);
  cardInfo->SD_cid.ProdSN |= tmp << 8;

  tmp                      = (uint8_t)((s_cidTab[3] & 0xFF000000) >> 24);
  cardInfo->SD_cid.ProdSN |= tmp;

  tmp                            = (uint8_t)((s_cidTab[3] & 0x00FF0000) >> 16);
  cardInfo->SD_cid.Reserved1    |= (tmp & 0xF0) >> 4;
  cardInfo->SD_cid.ManufactDate  = (tmp & 0x0F) << 8;

  tmp                            = (uint8_t)((s_cidTab[3] & 0x0000FF00) >> 8);
  cardInfo->SD_cid.ManufactDate |= tmp;

  tmp                        = (uint8_t)(s_cidTab[3] & 0x000000FF);
  cardInfo->SD_cid.CID_CRC   = (tmp & 0xFE) >> 1;
  cardInfo->SD_cid.Reserved2 = 1;

  return errorStatus;
}

/*!
 * @brief       Set SDIO bus wide
 *
 * @param       wideMode: bus wide
 *
 * @retval      SD error code
 *
 * @note        Four bit bus wide is unsupported to MMC card.
 *
 */
SD_ERROR_T SD_EnableWideBusOperation(uint32_t wideMode)
{
  SD_ERROR_T errorStatus = SD_OK;

  if (SDIO_MULTIMEDIA_CARD == s_sdCardType)
  {
    errorStatus = SD_UNSUPPORTED_FEATURE;
    return (errorStatus);
  }
  else if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == s_sdCardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == s_sdCardType)
           || (SDIO_HIGH_CAPACITY_SD_CARD == s_sdCardType))
  {
    /* SD card V2.0 is unsupported */
    if (SDIO_BUSWIDE_8B == wideMode)
    {
      errorStatus = SD_UNSUPPORTED_FEATURE;
      return (errorStatus);
    }
    else if (SDIO_BUSWIDE_4B == wideMode)
    {
      errorStatus = SD_EnWideBus(0x01);

      if (SD_OK == errorStatus)
      {
        sdioConfigStructure.clockDiv            = SDIO_TRANSFER_CLK_DIV;
        sdioConfigStructure.clockEdge           = SDIO_CLOCK_EDGE_RISING;
        sdioConfigStructure.clockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
        sdioConfigStructure.clockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
        sdioConfigStructure.busWide             = SDIO_BUSWIDE_4B;
        sdioConfigStructure.hardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
        SDIO_Config(&sdioConfigStructure);
      }
    }
    else
    {
      errorStatus = SD_EnWideBus(0);

      if (SD_OK == errorStatus)
      {
        sdioConfigStructure.clockDiv            = SDIO_TRANSFER_CLK_DIV;
        sdioConfigStructure.clockEdge           = SDIO_CLOCK_EDGE_RISING;
        sdioConfigStructure.clockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
        sdioConfigStructure.clockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
        sdioConfigStructure.busWide             = SDIO_BUSWIDE_1B;
        sdioConfigStructure.hardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
        SDIO_Config(&sdioConfigStructure);
      }
    }
  }

  return errorStatus;
}

/*!
 * @brief       Set device mode
 *
 * @param       mode: device mode
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_SetDeviceMode(SD_DEVICE_MODE_T mode)
{
  SD_ERROR_T errorStatus = SD_OK;

  if ((mode == SD_DMA_MODE) || (mode == SD_POLLING_MODE))
  {
    s_deviceMode = mode;
  }
  else
  {
    errorStatus = SD_INVALID_PARAMETER;
  }

  return errorStatus;
}

/*!
 * @brief       Select
 *
 * @param       mode: device mode
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_CardSelect(uint32_t addr)
{
  /* Send CMD7 */
  sdioCmdConfigStructure.argument = addr;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_SEL_DESEL_CARD;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R7 response and reture error status */
  return CmdResp1Error(SD_CMD_SEL_DESEL_CARD);
}

/*!
 * @brief       Read single block
 *
 * @param       buf: save the read data.(Must 4 bytes alignment)
 *
 * @param       addr: read address
 *
 * @param       blockSize: block size
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_ReadBlock(uint8_t *buf, long long addr, uint16_t blockSize)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint8_t    power;
  uint32_t   count = 0;
  tempbuff         = (uint32_t *)buf;
  uint32_t timeout = SDIO_DATATIMEOUT;

  if (NULL == buf)
  {
    return SD_INVALID_PARAMETER;
  }

  /* Disable DMA */
  SDIO->DCTRL = 0x0;

  if (s_sdCardType == SDIO_HIGH_CAPACITY_SD_CARD)
  {
    blockSize   = 512;
    addr      >>= 9;
  }

  /* Clear DPSM */
  sdioDataConfigStructure.dataBlockSize = SDIO_DATA_BLOCKSIZE_1B;
  sdioDataConfigStructure.dataLength    = 0;
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOCARD;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  SDIO_ConfigData(&sdioDataConfigStructure);


  if (SDIO->RES1 & SD_CARD_LOCKED)
  {
    return SD_LOCK_UNLOCK_FAILED;
  }

  if ((blockSize > 0) && (blockSize <= 2048) && ((blockSize & (blockSize - 1)) == 0))
  {
    /* Change data format */
    power = Convert_FromBytes2PowerofTwo(blockSize);

    /* Send CMD16 */
    sdioCmdConfigStructure.argument = blockSize;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_BLOCKLEN;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
  }
  else
  {
    return SD_INVALID_PARAMETER;
  }

  /* Config data format */
  sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)(power << 4);
  sdioDataConfigStructure.dataLength    = blockSize;
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOSDIO;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  SDIO_ConfigData(&sdioDataConfigStructure);

  /* Send CMD17 and start read data from addr*/
  sdioCmdConfigStructure.argument = addr;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_READ_SINGLE_BLOCK;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }

  if (s_deviceMode == SD_POLLING_MODE)
  {
    while (!(SDIO->STS & ((1 << 5) | (1 << 1) | (1 << 3) | (1 << 10) | (1 << 9))))
    {
      if (SDIO_ReadStatusFlag(SDIO_FLAG_RXFHF) != RESET)
      {
        /* Read data */
        for (count = 0; count < 8; count++)
        {
          *(tempbuff + count) = SDIO_ReadData();
        }
        tempbuff += 8;
        timeout   = 0X7FFFFF;
      }
      else
      {
        if (timeout == 0)
        {
          return SD_DATA_TIMEOUT;
        }
        timeout--;
      }
    }
    if (SDIO_ReadStatusFlag(SDIO_FLAG_DATATO) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_DATATO);

      return SD_DATA_TIMEOUT;
    }
    else if (SDIO_ReadStatusFlag(SDIO_FLAG_DBDR) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_DBDR);

      return SD_DATA_CRC_FAIL;
    }
    else if (SDIO_ReadStatusFlag(SDIO_FLAG_RXOVRER) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_RXOVRER);

      return SD_RX_OVERRUN;
    }
    else if (SDIO_ReadStatusFlag(SDIO_FLAG_SBE) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_SBE);

      return SD_START_BIT_ERR;
    }
    /* If FIFODATA has data */
    while (SDIO_ReadStatusFlag(SDIO_FLAG_RXDA) != RESET)
    {
      *tempbuff = SDIO_ReadData();
      tempbuff++;
    }
    /* Clear all flags */
    SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);
  }
  else if (s_deviceMode == SD_DMA_MODE)
  {
    g_transferError = SD_OK;
    /* Read single blcok no need to send stop command */
    s_stopCondition = 0;

    g_transferEnd = 0;

    /* Set the required interrupts */
    SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 5) | (1 << 9);

    SDIO_EnableDMA();
    SD_DMA_Config((uint32_t *)buf, blockSize, DMA_DIR_PERIPHERAL_SRC);

    while ((DMA_ReadStatusFlag(DMA2_FLAG_TC4) == RESET) && (g_transferEnd == 0) && (g_transferError == SD_OK) && timeout)
    {
      timeout--;
    }
    if (timeout == 0)
    {
      return SD_DATA_TIMEOUT;
    }
    if (g_transferError != SD_OK)
    {
      errorStatus = g_transferError;
    }
  }

  return errorStatus;
}

/*!
 * @brief       Read multi block
 *
 * @param       buf: save the read data.(Must 4 bytes alignment)
 *
 * @param       addr: read address
 *
 * @param       blockSize: block size
 *
 * @param       nblks:need read number of block
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_ReadMultiBlocks(uint8_t *buf, long long addr, uint16_t blockSize, uint32_t nblks, uint32_t timeout)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint8_t    power;
  uint32_t   count = 0;
  tempbuff         = (uint32_t *)buf;

  /* Disable DMA */
  SDIO->DCTRL = 0x0;

  if (s_sdCardType == SDIO_HIGH_CAPACITY_SD_CARD)
  {
    blockSize   = 512;
    addr      >>= 9;
  }
  /* Clear DPSM */
  sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)0;
  sdioDataConfigStructure.dataLength    = 0;
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOCARD;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  SDIO_ConfigData(&sdioDataConfigStructure);

  if (SDIO->RES1 & SD_CARD_LOCKED)
  {
    return SD_LOCK_UNLOCK_FAILED;
  }
  if ((blockSize > 0) && (blockSize <= 2048) && ((blockSize & (blockSize - 1)) == 0))
  {
    /* Change data format according block size */
    power = Convert_FromBytes2PowerofTwo(blockSize);

    /* Send CMD16 */
    sdioCmdConfigStructure.argument = blockSize;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_BLOCKLEN;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
  }
  else
  {
    return SD_INVALID_PARAMETER;
  }

  /* Read multi block */
  if (nblks >= 1) // THH
  {
    if (nblks * blockSize > SD_MAX_DATA_LENGTH)
    {
      return SD_INVALID_PARAMETER;
    }
    /** Config data format */
    sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)(power << 4);
    sdioDataConfigStructure.dataLength    = nblks * blockSize;
    sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
    sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
    sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOSDIO;
    sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
    SDIO_ConfigData(&sdioDataConfigStructure);

    /* Send CMD18 and start read data from addr*/
    sdioCmdConfigStructure.argument = addr;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_READ_MULT_BLOCK;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_READ_MULT_BLOCK);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }

    if (s_deviceMode == SD_POLLING_MODE)
    {
      while (!(SDIO->STS & ((1 << 5) | (1 << 1) | (1 << 3) | (1 << 8) | (1 << 9))))
      {
        if (SDIO_ReadStatusFlag(SDIO_FLAG_RXFHF) != RESET)
        {
          /* Read data */
          for (count = 0; count < 8; count++)
          {
            *(tempbuff + count) = SDIO_ReadData();
          }
          tempbuff += 8;
          timeout   = 0X7FFFFF;
        }
        else
        {
          if (timeout == 0)
          {
            return SD_DATA_TIMEOUT;
          }
          timeout--;
        }
      }
      if (SDIO_ReadStatusFlag(SDIO_FLAG_DATATO) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_DATATO);

        return SD_DATA_TIMEOUT;
      }
      else if (SDIO_ReadStatusFlag(SDIO_FLAG_DBDR) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_DBDR);

        return SD_DATA_CRC_FAIL;
      }
      else if (SDIO_ReadStatusFlag(SDIO_FLAG_RXOVRER) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_RXOVRER);

        return SD_RX_OVERRUN;
      }
      else if (SDIO_ReadStatusFlag(SDIO_FLAG_SBE) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_SBE);

        return SD_START_BIT_ERR;
      }
      /* If FIFODATA has data */
      while (SDIO_ReadStatusFlag(SDIO_FLAG_RXDA) != RESET)
      {
        *tempbuff = SDIO_ReadData();
        tempbuff++;
      }

      if (SDIO_ReadStatusFlag(SDIO_FLAG_DATAEND) != RESET)
      {
        if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == s_sdCardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == s_sdCardType)
            || (SDIO_HIGH_CAPACITY_SD_CARD == s_sdCardType))
        {
          /* Send CMD12 to stop transmission */
          sdioCmdConfigStructure.argument = 0;
          sdioCmdConfigStructure.cmdIndex = SD_CMD_STOP_TRANSMISSION;
          sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
          sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
          sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
          SDIO_TxCommand(&sdioCmdConfigStructure);

          /* Wait R1 response */
          errorStatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);

          if (errorStatus != SD_OK)
          {
            return errorStatus;
          }
        }
      }
      /* Clear all flags */
      SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);
    }
    else if (s_deviceMode == SD_DMA_MODE)
    {
      g_transferError = SD_OK;
      /* Read multi blcok need to send stop command */
      s_stopCondition = 1;
      g_transferEnd   = 0;

      /* Set the required interrupts */
      SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 5) | (1 << 9);

      SDIO_EnableDMA();
      SD_DMA_Config((uint32_t *)buf, nblks * blockSize, DMA_DIR_PERIPHERAL_SRC);

      uint32_t pre_time;

      pre_time = millis();
      while ((DMA_ReadStatusFlag(DMA2_FLAG_TC4) == RESET))
      {
        if (millis()-pre_time >= timeout)
        {
          return SD_DATA_TIMEOUT;
        }  
      }
      while ((g_transferEnd == 0) && (g_transferError == SD_OK))
      {
        if (millis()-pre_time >= timeout)
        {
          return SD_DATA_TIMEOUT;
        }  
      }

      if (g_transferError != SD_OK)
      {
        errorStatus = g_transferError;
      }
    }
  }

  return errorStatus;
}

/*!
 * @brief       Write single block
 *
 * @param       buf: save the read data.(Must 4 bytes alignment)
 *
 * @param       addr: write address
 *
 * @param       blockSize: block size
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_WriteBlock(uint8_t *buf, long long addr, uint16_t blockSize)
{
  SD_ERROR_T errorStatus      = SD_OK;
  uint8_t    power            = 0;
  uint8_t    cardState        = 0;
  uint32_t   timeout          = 0;
  uint32_t   bytesTransferred = 0;
  uint32_t   cardStatus       = 0;
  uint32_t   count            = 0;
  uint32_t   restWords        = 0;
  /* Total length(bytes) */
  uint32_t tlen = blockSize;
  tempbuff      = (uint32_t *)buf;

  if (buf == NULL)
  {
    return SD_INVALID_PARAMETER;
  }

  /* Disable DMA */
  SDIO->DCTRL = 0x0;

  /* Clear DPSM */
  sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)0;
  sdioDataConfigStructure.dataLength    = 0;
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOCARD;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  SDIO_ConfigData(&sdioDataConfigStructure);


  if (SDIO->RES1 & SD_CARD_LOCKED)
  {
    return SD_LOCK_UNLOCK_FAILED;
  }

  if (s_sdCardType == SDIO_HIGH_CAPACITY_SD_CARD)
  {
    blockSize   = 512;
    addr      >>= 9;
  }

  if ((blockSize > 0) && (blockSize <= 2048) && ((blockSize & (blockSize - 1)) == 0))
  {
    /* Change data format */
    power = Convert_FromBytes2PowerofTwo(blockSize);

    /* Send CMD16 */
    sdioCmdConfigStructure.argument = blockSize;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_BLOCKLEN;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
  }
  else
  {
    return SD_INVALID_PARAMETER;
  }

  /* Send CMD13 to check card status */
  sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_SEND_STATUS;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_SEND_STATUS);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }

  /* Get the response */
  cardStatus = SDIO_ReadResponse(SDIO_RES1);

  timeout = SD_DATATIMEOUT;

  while (((cardStatus & 0x00000100) == 0) && (timeout > 0))
  {
    timeout--;
    /* Send CMD13 to check card status */
    sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SEND_STATUS;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_SEND_STATUS);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }

    /* Get the response */
    cardStatus = SDIO_ReadResponse(SDIO_RES1);
  }
  if (timeout == 0)
  {
    return SD_ERROR;
  }
  /* Send CMD24 to write block */
  sdioCmdConfigStructure.argument = addr;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }
  /* Write single blcok no need to send stop command */
  s_stopCondition = 0;

  /* Config data format */
  sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)(power << 4);
  sdioDataConfigStructure.dataLength    = blockSize;
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOCARD;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  SDIO_ConfigData(&sdioDataConfigStructure);

  timeout = SDIO_DATATIMEOUT;

  if (s_deviceMode == SD_POLLING_MODE)
  {
    while (!(SDIO->STS & ((1 << 10) | (1 << 4) | (1 << 1) | (1 << 3) | (1 << 9))))
    {
      if (SDIO_ReadStatusFlag(SDIO_FLAG_TXFHF) != RESET)
      {
        if ((tlen - bytesTransferred) < SD_HALFFIFOBYTES)
        {
          restWords = ((tlen - bytesTransferred) % 4 == 0) ? ((tlen - bytesTransferred) / 4) : ((tlen - bytesTransferred) / 4 + 1);

          for (count = 0; count < restWords; count++, tempbuff++, bytesTransferred += 4)
          {
            SDIO->FIFODATA = *tempbuff;
          }
        }
        else
        {
          for (count = 0; count < 8; count++)
          {
            SDIO_WriteData(*(tempbuff + count));
          }
          tempbuff         += 8;
          bytesTransferred += 32;
        }
        timeout = 0X3FFFFFFF;
      }
      else
      {
        if (timeout == 0)
        {
          return SD_DATA_TIMEOUT;
        }
        timeout--;
      }
    }
    if (SDIO_ReadStatusFlag(SDIO_FLAG_DATATO) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_DATATO);

      return SD_DATA_TIMEOUT;
    }
    else if (SDIO_ReadStatusFlag(SDIO_FLAG_DBDR) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_DBDR);

      return SD_DATA_CRC_FAIL;
    }
    else if (SDIO_ReadStatusFlag(SDIO_FLAG_TXUDRER) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_TXUDRER);

      return SD_TX_UNDERRUN;
    }
    else if (SDIO_ReadStatusFlag(SDIO_FLAG_SBE) != RESET)
    {
      SDIO_ClearStatusFlag(SDIO_FLAG_SBE);

      return SD_START_BIT_ERR;
    }
    /* Clear all flags */
    SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);
  }
  else if (s_deviceMode == SD_DMA_MODE)
  {
    g_transferError = SD_OK;
    /* Write single blcok no need to send stop command */
    s_stopCondition = 0;
    g_transferEnd   = 0;

    /* Set the required interrupts */
    SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 4) | (1 << 9);

    SDIO_EnableDMA();
    SD_DMA_Config((uint32_t *)buf, blockSize, DMA_DIR_PERIPHERAL_DST);

    while ((DMA_ReadStatusFlag(DMA2_FLAG_TC4) == RESET) && timeout)
    {
      timeout--;
    }
    if (timeout == 0)
    {
      /* Reinitialize */
      SD_Init();
      return SD_DATA_TIMEOUT;
    }
    timeout = SDIO_DATATIMEOUT;

    while ((g_transferEnd == 0) && (g_transferError == SD_OK) && timeout)
    {
      timeout--;
    }
    if (timeout == 0)
    {
      return SD_DATA_TIMEOUT;
    }
    if (g_transferError != SD_OK)
    {
      return g_transferError;
    }
  }
  /* Clear all flags */
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  errorStatus = IsCardProgramming(&cardState);

  while ((errorStatus == SD_OK) && ((cardState == SD_CARD_PROGRAMMING) || (cardState == SD_CARD_RECEIVING)))
  {
    errorStatus = IsCardProgramming(&cardState);
  }

  return errorStatus;
}

/*!
 * @brief       Write multi block
 *
 * @param       buf: save the read data.(Must 4 bytes alignment)
 *
 * @param       addr: write address
 *
 * @param       blockSize: block size
 *
 * @param       nblks:need read number of block
 *
 * @retval      SD error code
 *
 */
SD_ERROR_T SD_WriteMultiBlocks(uint8_t *buf, long long addr, uint16_t blockSize, uint32_t nblks, uint32_t Timeout)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint8_t    power = 0, cardState = 0;
  uint32_t   bytesTransferred = 0;
  uint32_t   count            = 0;
  uint32_t   restwords        = 0;

  /* Total length(bytes) */
  uint32_t tlen = nblks * blockSize;
  tempbuff      = (uint32_t *)buf;

  if (buf == NULL)
  {
    return SD_INVALID_PARAMETER;
  }
  /* Disable DMA */
  SDIO->DCTRL = 0x0;

  /* Clear DPSM */
  sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)0;
  sdioDataConfigStructure.dataLength    = 0;
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOCARD;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  SDIO_ConfigData(&sdioDataConfigStructure);

  if (SDIO->RES1 & SD_CARD_LOCKED)
  {
    return SD_LOCK_UNLOCK_FAILED;
  }
  if (s_sdCardType == SDIO_HIGH_CAPACITY_SD_CARD)
  {
    blockSize   = 512;
    addr      >>= 9;
  }
  if ((blockSize > 0) && (blockSize <= 2048) && ((blockSize & (blockSize - 1)) == 0))
  {
    /* Change data format */
    power = Convert_FromBytes2PowerofTwo(blockSize);

    /* Send CMD16 */
    sdioCmdConfigStructure.argument = blockSize;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_BLOCKLEN;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
  }
  else
  {
    return SD_INVALID_PARAMETER;
  }

  /* Write multi block */
  if (nblks >= 1)
  {
    if (nblks * blockSize > SD_MAX_DATA_LENGTH)
    {
      return SD_INVALID_PARAMETER;
    }
    if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == s_sdCardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == s_sdCardType)
        || (SDIO_HIGH_CAPACITY_SD_CARD == s_sdCardType))
    {
      /* Send CMD55 */
      sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
      sdioCmdConfigStructure.cmdIndex = SD_CMD_APP_CMD;
      sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
      sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
      sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
      SDIO_TxCommand(&sdioCmdConfigStructure);

      /* Wait R1 response */
      errorStatus = CmdResp1Error(SD_CMD_APP_CMD);

      if (errorStatus != SD_OK)
      {
        return errorStatus;
      }

      /* Send CMD23 to set quantiry of block */
      sdioCmdConfigStructure.argument = nblks;
      sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_BLOCK_COUNT;
      sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
      sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
      sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
      SDIO_TxCommand(&sdioCmdConfigStructure);

      /* Wait R1 response */
      errorStatus = CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);

      if (errorStatus != SD_OK)
      {
        return errorStatus;
      }
    }
    /* Send CMD25 and start write data from addr */
    sdioCmdConfigStructure.argument = addr;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_WRITE_MULT_BLOCK;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
    /* Config data format */
    sdioDataConfigStructure.dataBlockSize = (SDIO_DATA_BLOCKSIZE_T)(power << 4);
    sdioDataConfigStructure.dataLength    = nblks * blockSize;
    sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
    sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
    sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOCARD;
    sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
    SDIO_ConfigData(&sdioDataConfigStructure);

    if (s_deviceMode == SD_POLLING_MODE)
    {

      uint32_t pre_time;

      pre_time = millis();
      while (!(SDIO->STS & ((1 << 4) | (1 << 1) | (1 << 8) | (1 << 3) | (1 << 9))))
      {
        if (SDIO_ReadStatusFlag(SDIO_FLAG_TXFHF) != RESET)
        {
          if ((tlen - bytesTransferred) < SD_HALFFIFOBYTES)
          {
            restwords = ((tlen - bytesTransferred) % 4 == 0) ? ((tlen - bytesTransferred) / 4) : ((tlen - bytesTransferred) / 4 + 1);
            for (count = 0; count < restwords; count++, tempbuff++, bytesTransferred += 4)
            {
              SDIO_WriteData(*tempbuff);
            }
          }
          else
          {
            for (count = 0; count < SD_HALFFIFO; count++)
            {
              SDIO_WriteData(*(tempbuff + count));
            }
            tempbuff         += SD_HALFFIFO;
            bytesTransferred += SD_HALFFIFOBYTES;
          }
        }
        else
        {
          if (millis()-pre_time >= Timeout)
          {
            return SD_DATA_TIMEOUT;
          }
        }
      }
      if (SDIO_ReadStatusFlag(SDIO_FLAG_DATATO) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_DATATO);

        return SD_DATA_TIMEOUT;
      }
      else if (SDIO_ReadStatusFlag(SDIO_FLAG_DBDR) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_DBDR);

        return SD_DATA_CRC_FAIL;
      }
      else if (SDIO_ReadStatusFlag(SDIO_FLAG_TXUDRER) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_TXUDRER);

        return SD_TX_UNDERRUN;
      }
      else if (SDIO_ReadStatusFlag(SDIO_FLAG_SBE) != RESET)
      {
        SDIO_ClearStatusFlag(SDIO_FLAG_SBE);

        return SD_START_BIT_ERR;
      }
      /* Finish data transmission */
      if (SDIO_ReadStatusFlag(SDIO_FLAG_DATAEND) != RESET)
      {
        if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == s_sdCardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == s_sdCardType)
            || (SDIO_HIGH_CAPACITY_SD_CARD == s_sdCardType))
        {
          /* Send CMD12 to stop transmission */
          sdioCmdConfigStructure.argument = 0;
          sdioCmdConfigStructure.cmdIndex = SD_CMD_STOP_TRANSMISSION;
          sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
          sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
          sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
          SDIO_TxCommand(&sdioCmdConfigStructure);

          /* Wait R1 response */
          errorStatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);

          if (errorStatus != SD_OK)
          {
            return errorStatus;
          }
        }
      }
      /* Clear all flags */
      SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);
    }
    else if (s_deviceMode == SD_DMA_MODE)
    {
      g_transferError = SD_OK;
      /* Write multi blcok need to send stop command */
      s_stopCondition = 1;
      g_transferEnd   = 0;
      /* Set the required interrupts */
      SDIO->MASK |= (1 << 1) | (1 << 3) | (1 << 8) | (1 << 4) | (1 << 9);

      SDIO_EnableDMA();
      SD_DMA_Config((uint32_t *)buf, nblks * blockSize, DMA_DIR_PERIPHERAL_DST);

      uint32_t pre_time;

      pre_time = millis();
      while ((DMA_ReadStatusFlag(DMA2_FLAG_TC4) == RESET))
      {
        if (millis()-pre_time >= Timeout)
        {
          /* Reinitialize */
          SD_Init();
          return SD_DATA_TIMEOUT;
        }
      }

      while ((g_transferEnd == 0) && (g_transferError == SD_OK))
      {
        if (millis()-pre_time >= Timeout)
        {
          return SD_DATA_TIMEOUT;
        }
      }
      if (g_transferError != SD_OK)
      {
        return g_transferError;
      }
    }
  }
  /* Clear all flags */
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  errorStatus = IsCardProgramming(&cardState);

  uint32_t pre_time;

  pre_time = millis();
  while ((errorStatus == SD_OK) && ((cardState == SD_CARD_PROGRAMMING) || (cardState == SD_CARD_RECEIVING)))
  {
    if (millis()-pre_time >= Timeout)
    {
      return SD_DATA_TIMEOUT;
    }

    errorStatus = IsCardProgramming(&cardState);
  }

  return errorStatus;
}

/*!
 * @brief        This function handles SDIO interrupt Handler
 *
 * @param        None
 *
 * @retval       SD error code
 *
 * @note         This function need to put into  void SDIO_IRQHandler(void)
 */
SD_ERROR_T SD_Process_Isr(void)
{
  if (SDIO_ReadStatusFlag(SDIO_FLAG_DATAEND) != RESET)
  {
    if (s_stopCondition == 1)
    {
      /* Send CMD12 to stop transmission */
      sdioCmdConfigStructure.argument = 0;
      sdioCmdConfigStructure.cmdIndex = SD_CMD_STOP_TRANSMISSION;
      sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
      sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
      sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
      SDIO_TxCommand(&sdioCmdConfigStructure);

      /* Wait R1 response */
      g_transferError = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
    }
    else
    {
      g_transferError = SD_OK;
    }
    /* Clear interrupt flag */
    SDIO->ICF |= 1 << 8;
    /* Disable the related interrupts */
    SDIO->MASK    &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
    g_transferEnd  = 1;

    return (g_transferError);
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_DBDR) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_DBDR);

    /* Disable the related interrupts */
    SDIO->MASK      &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
    g_transferError  = SD_DATA_CRC_FAIL;

    return (SD_DATA_CRC_FAIL);
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_DATATO) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_DATATO);

    /* Disable the related interrupts */
    SDIO->MASK      &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
    g_transferError  = SD_DATA_TIMEOUT;

    return (SD_DATA_TIMEOUT);
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_RXOVRER) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_RXOVRER);

    /* Disable the related interrupts */
    SDIO->MASK      &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
    g_transferError  = SD_RX_OVERRUN;

    return (SD_RX_OVERRUN);
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_TXUDRER) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_TXUDRER);

    /* Disable the related interrupts */
    SDIO->MASK      &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
    g_transferError  = SD_TX_UNDERRUN;

    return (SD_TX_UNDERRUN);
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_SBE) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_SBE);

    /* Disable the related interrupts */
    SDIO->MASK      &= ~((1 << 1) | (1 << 3) | (1 << 8) | (1 << 14) | (1 << 15) | (1 << 4) | (1 << 5) | (1 << 9));
    g_transferError  = SD_START_BIT_ERR;

    return (SD_START_BIT_ERR);
  }

  return (SD_OK);
}

/*!
 * @brief        Check response of CMD0
 *
 * @param        None
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T CmdError(void)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint32_t   timeout     = SDIO_CMD0TIMEOUT;

  while (timeout--)
  {
    if (SDIO_ReadStatusFlag(SDIO_FLAG_CMDSENT) != RESET)
    {
      break;
    }
  }
  if (timeout == 0)
  {
    return SD_CMD_RSP_TIMEOUT;
  }
  /* Clear interrupt flag */
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  return errorStatus;
}

/*!
 * @brief        Check R7 response
 *
 * @param        None
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T CmdResp7Error(void)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint32_t   status;
  uint32_t   timeout = SDIO_CMD0TIMEOUT;

  while (timeout--)
  {
    status = SDIO->STS;
    if (status & ((1 << 0) | (1 << 2) | (1 << 6)))
    {
      break;
    }
  }
  /* Response timeout */
  if ((timeout == 0) || (status & (1 << 2)))
  {
    errorStatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRESTO);
    return errorStatus;
  }
  /* Receive response */
  if (status & 1 << 6)
  {
    errorStatus = SD_OK;
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRES);
  }

  return errorStatus;
}

/*!
 * @brief        Check R1 response of current command
 *
 * @param        cmd: command
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T CmdResp1Error(uint8_t cmd)
{
  uint32_t status;

  while (1)
  {
    status = SDIO->STS;
    if (status & ((1 << 0) | (1 << 2) | (1 << 6)))
    {
      break;
    }
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_CMDRESTO) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRESTO);
    return SD_CMD_RSP_TIMEOUT;
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_COMRESP) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_COMRESP);
    return SD_CMD_CRC_FAIL;
  }
  /* Illegal command */
  if (SDIO->CMDRES != cmd)
  {
    return SD_ILLEGAL_CMD;
  }
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  return (SD_ERROR_T)(SDIO->RES1 & SD_OCR_ERRORBITS);
}

/*!
 * @brief        Check R3 response
 *
 * @param        None
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T CmdResp3Error(void)
{
  uint32_t status;

  while (1)
  {
    status = SDIO->STS;
    if (status & ((1 << 0) | (1 << 2) | (1 << 6)))
    {
      break;
    }
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_CMDRESTO) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRESTO);
    return SD_CMD_RSP_TIMEOUT;
  }
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  return SD_OK;
}

/*!
 * @brief        Check R2 response
 *
 * @param        None
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T CmdResp2Error(void)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint32_t   status;
  uint32_t   timeout = SDIO_CMD0TIMEOUT;

  while (timeout--)
  {
    status = SDIO->STS;
    if (status & ((1 << 0) | (1 << 2) | (1 << 6)))
    {
      break;
    }
  }
  /* Response timeout */
  if ((timeout == 0) || (status & (1 << 2)))
  {
    errorStatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRESTO);
    return errorStatus;
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_COMRESP) != RESET)
  {
    errorStatus = SD_CMD_CRC_FAIL;
    SDIO_ClearStatusFlag(SDIO_FLAG_COMRESP);
  }
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  return errorStatus;
}

/*!
 * @brief        Check R6 response of current command
 *
 * @param        cmd: command
 *
 * @param        prca: RCA value
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T CmdResp6Error(uint8_t cmd, uint16_t *prca)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint32_t   status;
  uint32_t   rspr1;

  while (1)
  {
    status = SDIO->STS;
    if (status & ((1 << 0) | (1 << 2) | (1 << 6)))
    {
      break;
    }
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_CMDRESTO) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRESTO);
    return SD_CMD_RSP_TIMEOUT;
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_COMRESP) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_COMRESP);
    return SD_CMD_CRC_FAIL;
  }
  if (SDIO->CMDRES != cmd)
  {
    return SD_ILLEGAL_CMD;
  }

  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);
  /* Get response */
  rspr1 = SDIO_ReadResponse(SDIO_RES1);

  if (SD_ALLZERO == (rspr1 & (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD | SD_R6_COM_CRC_FAILED)))
  {
    *prca = (uint16_t)(rspr1 >> 16);
    return errorStatus;
  }
  if (rspr1 & SD_R6_GENERAL_UNKNOWN_ERROR)
  {
    return SD_GENERAL_UNKNOWN_ERROR;
  }
  if (rspr1 & SD_R6_ILLEGAL_CMD)
  {
    return SD_ILLEGAL_CMD;
  }
  if (rspr1 & SD_R6_COM_CRC_FAILED)
  {
    return SD_COM_CRC_FAILED;
  }

  return errorStatus;
}

/*!
 * @brief        Enable SDIO bus wide mode
 *
 * @param        enx:
 *               0: disable
 *               1: enable
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T SD_EnWideBus(uint8_t enx)
{
  SD_ERROR_T errorStatus = SD_OK;
  uint32_t   scr[2]      = {0, 0};
  uint8_t    arg         = 0X00;

  if (enx)
  {
    /* 4 bit bus wide */
    arg = 0x02;
  }
  else
  {
    /* 1 bit bus wide */
    arg = 0x00;
  }

  if (SDIO->RES1 & SD_CARD_LOCKED)
  {
    return SD_LOCK_UNLOCK_FAILED;
  }
  /* Get SCR register data */
  errorStatus = SD_FindSCR(s_rca, scr);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }
  /* SD card support bus wide mode */
  if ((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO)
  {
    /* Send CMD55 + RCA */
    sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_APP_CMD;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_APP_CMD);

    if (errorStatus != SD_OK)
    {
      return errorStatus;
    }
    /* Send ACMD6 */
    sdioCmdConfigStructure.argument = arg;
    sdioCmdConfigStructure.cmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
    sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
    sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
    sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
    SDIO_TxCommand(&sdioCmdConfigStructure);

    /* Wait R1 response */
    errorStatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);

    return errorStatus;
  }
  else
  {
    return SD_REQUEST_NOT_APPLICABLE;
  }
}

/*!
 * @brief        Check SDID status
 *
 * @param        pstatus: current status
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T IsCardProgramming(uint8_t *pstatus)
{
  vu32 respR1 = 0;
  vu32 status = 0;

  /* Send CMD13 + RCA */
  sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_SEND_STATUS;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Get SDIO flag */
  status = SDIO->STS;

  while (!(status & ((1 << 0) | (1 << 6) | (1 << 2))))
  {
    status = SDIO->STS;
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_COMRESP) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_COMRESP);

    return SD_CMD_CRC_FAIL;
  }
  if (SDIO_ReadStatusFlag(SDIO_FLAG_CMDRESTO) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_CMDRESTO);

    return SD_CMD_RSP_TIMEOUT;
  }
  if (SDIO->CMDRES != SD_CMD_SEND_STATUS)
  {
    return SD_ILLEGAL_CMD;
  }
  /* Clear all flags */
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);

  respR1 = SDIO_ReadResponse(SDIO_RES1);

  *pstatus = (uint8_t)((respR1 >> 9) & 0x0000000F);

  return SD_OK;
}

/*!
 * @brief        Check SD card status
 *
 * @param        pcardStatus: card status
 *
 * @retval       SD error code
 *
 */
SD_ERROR_T SD_SendStatus(uint32_t *pcardStatus)
{
  SD_ERROR_T errorStatus = SD_OK;

  if (pcardStatus == NULL)
  {
    errorStatus = SD_INVALID_PARAMETER;

    return errorStatus;
  }
  /* Send CMD13 + RCA */
  sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_SEND_STATUS;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_SEND_STATUS);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }
  *pcardStatus = SDIO_ReadResponse(SDIO_RES1);

  return errorStatus;
}

/*!
 * @brief        Return SD card status
 *
 * @param        None
 *
 * @retval       SD card state code
 *
 */
SD_CardState_T SD_GetState(void)
{
  uint32_t resp1 = 0;

  if (SD_SendStatus(&resp1) != SD_OK)
  {
    return SD_CARD_ERROR;
  }
  else
  {
    return (SD_CardState_T)((resp1 >> 9) & 0x0F);
  }
}

/*!
 * @brief        Find SD card SCR register
 *
 * @param        rca: RCA
 *
 * @param        pscr: SCR buffer
 *
 * @retval       SD card state code
 *
 */
SD_ERROR_T SD_FindSCR(uint16_t rca, uint32_t *pscr)
{
  uint32_t   index       = 0;
  SD_ERROR_T errorStatus = SD_OK;
  uint32_t   tempscr[2]  = {0, 0};

  /* Send CMD16 to set block size as 8 bytes */
  sdioCmdConfigStructure.argument = (uint32_t)8;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_SET_BLOCKLEN;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }
  /* Send CMD55 + RCA */
  sdioCmdConfigStructure.argument = (uint32_t)s_rca << 16;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_APP_CMD;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_APP_CMD);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }
  /* Send ACMD51 */
  sdioCmdConfigStructure.argument = 0x0;
  sdioCmdConfigStructure.cmdIndex = SD_CMD_SD_APP_SEND_SCR;
  sdioCmdConfigStructure.response = SDIO_RESPONSE_SHORT;
  sdioCmdConfigStructure.wait     = SDIO_WAIT_NO;
  sdioCmdConfigStructure.CPSM     = SDIO_CPSM_ENABLE;
  SDIO_TxCommand(&sdioCmdConfigStructure);

  /* Wait R1 response */
  errorStatus = CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);

  if (errorStatus != SD_OK)
  {
    return errorStatus;
  }

  /* Config data format */
  sdioDataConfigStructure.dataTimeOut   = SD_DATATIMEOUT;
  sdioDataConfigStructure.dataLength    = 8;
  sdioDataConfigStructure.dataBlockSize = SDIO_DATA_BLOCKSIZE_8B;
  sdioDataConfigStructure.transferDir   = SDIO_TRANSFER_DIR_TOSDIO;
  sdioDataConfigStructure.transferMode  = SDIO_TRANSFER_MODE_BLOCK;
  sdioDataConfigStructure.DPSM          = SDIO_DPSM_ENABLE;
  SDIO_ConfigData(&sdioDataConfigStructure);

  while (!(SDIO->STS & (SDIO_FLAG_RXOVRER | SDIO_FLAG_DBDR | SDIO_FLAG_DATATO | SDIO_FLAG_DBCP | SDIO_FLAG_SBE)))
  {
    if (SDIO_ReadStatusFlag(SDIO_FLAG_RXDA) != RESET)
    {
      *(tempscr + index) = SDIO_ReadData();

      index++;
      if (index >= 2)
      {
        break;
      }
    }
  }

  if (SDIO_ReadStatusFlag(SDIO_FLAG_DATATO) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_DATATO);

    return SD_DATA_TIMEOUT;
  }
  else if (SDIO_ReadStatusFlag(SDIO_FLAG_DBDR) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_DBDR);

    return SD_DATA_CRC_FAIL;
  }
  else if (SDIO_ReadStatusFlag(SDIO_FLAG_RXOVRER) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_RXOVRER);

    return SD_RX_OVERRUN;
  }
  else if (SDIO_ReadStatusFlag(SDIO_FLAG_SBE) != RESET)
  {
    SDIO_ClearStatusFlag(SDIO_FLAG_SBE);

    return SD_START_BIT_ERR;
  }
  /* Clear all flags */
  SDIO_ClearStatusFlag(SDIO_STATIC_FLAGS);
  /* collate the data */
  *(pscr + 1) = ((tempscr[0] & SD_0TO7BITS) << 24) | ((tempscr[0] & SD_8TO15BITS) << 8) | ((tempscr[0] & SD_16TO23BITS) >> 8) | ((tempscr[0] & SD_24TO31BITS) >> 24);
  *(pscr)     = ((tempscr[1] & SD_0TO7BITS) << 24) | ((tempscr[1] & SD_8TO15BITS) << 8) | ((tempscr[1] & SD_16TO23BITS) >> 8) | ((tempscr[1] & SD_24TO31BITS) >> 24);

  return errorStatus;
}

/*!
 * @brief        Convert data to a 2-based index
 *
 * @param        NumberOfBytes: The data need to be converted
 *
 * @retval       2-based index
 *
 */
uint8_t Convert_FromBytes2PowerofTwo(uint16_t NumberOfBytes)
{
  uint8_t count = 0;

  while (NumberOfBytes != 1)
  {
    NumberOfBytes >>= 1;
    count++;
  }

  return count;
}

/*!
 * @brief        Config SD DMA
 *
 * @param        memoryBaseAddr: Memory address
 *
 * @param        bufSize: The buffer size of transmission
 *
 * @param        dir: Data transmission direction
 *
 * @retval       None
 *
 */
void SD_DMA_Config(uint32_t *memoryBaseAddr, uint32_t bufSize, DMA_DIR_T dir)
{
  DMA_Config_T DMA_InitStructure;

  /* Clear all DMA2 stream4 intrrupt flag */
  DMA_ClearStatusFlag(DMA2_FLAG_GINT4 | DMA2_FLAG_TC4 | DMA2_FLAG_HT4 | DMA2_FLAG_TERR4);

  /* Disable DMA2 stream4 */
  DMA_Disable(DMA2_Channel4);

  /* Set peripheral addr as SDIO data register addr */
  DMA_InitStructure.peripheralBaseAddr = SDIO_DATA_ADDRESS;
  /* memory base addr */
  DMA_InitStructure.memoryBaseAddr = (uint32_t)memoryBaseAddr;
  /* data transfer direction */
  DMA_InitStructure.dir = dir;
  /* buffer size in byte, switch to word */
  DMA_InitStructure.bufferSize = bufSize / 4;
  /* peripheral no increase mode */
  DMA_InitStructure.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
  /* memory increase mode */
  DMA_InitStructure.memoryInc = DMA_MEMORY_INC_ENABLE;
  /* set peripheral data size as 32 bits */
  DMA_InitStructure.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_WOED;
  /* memory data size */
  DMA_InitStructure.memoryDataSize = DMA_MEMORY_DATA_SIZE_WOED;
  /* normal mode */
  DMA_InitStructure.loopMode = DMA_MODE_NORMAL;
  /* very high priority mode */
  DMA_InitStructure.priority = DMA_PRIORITY_VERYHIGH;
  /* enable Memory to Memory */
  DMA_InitStructure.M2M = DMA_M2MEN_DISABLE;
  /* init DAM stream */
  DMA_Config(DMA2_Channel4, &DMA_InitStructure);

  /* Enable DMA2 stream */
  DMA_Enable(DMA2_Channel4);
}

void SDIO_IRQHandler(void)
{
  SD_Process_Isr();
}

HAL_StatusTypeDef HAL_SD_Init(SD_HandleTypeDef *hsd)
{
  HAL_StatusTypeDef status = HAL_OK;

  hsd->error = SD_Init();
  status = hsd->error == SD_OK ? HAL_OK:HAL_ERROR;

  return status;
}

HAL_StatusTypeDef HAL_SD_DeInit(SD_HandleTypeDef *hsd)
{
  HAL_StatusTypeDef status = HAL_OK;

  return status;  
}

HAL_StatusTypeDef HAL_SD_Erase(SD_HandleTypeDef *hsd, uint32_t BlockStartAdd, uint32_t BlockEndAdd)
{
  HAL_StatusTypeDef status = HAL_OK;

  return status;  
}

HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, const uint8_t *pData, uint32_t BlockAdd,
                                         uint32_t NumberOfBlocks, uint32_t timeout)
{
  HAL_StatusTypeDef status = HAL_OK;

  hsd->error = SD_WriteMultiBlocks((uint8_t *)pData, BlockAdd * 512, 512, NumberOfBlocks, timeout);
  status = hsd->error == SD_OK ? HAL_OK:HAL_ERROR;

  return status;  
}

HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd,
                                        uint32_t NumberOfBlocks, uint32_t Timeout)
{
  HAL_StatusTypeDef status = HAL_OK;

  hsd->error = SD_ReadMultiBlocks(pData, BlockAdd * 512, 512, NumberOfBlocks, Timeout);
  status = hsd->error == SD_OK ? HAL_OK:HAL_ERROR;

  return status;  
}

HAL_SD_CardStateTypeDef HAL_SD_GetCardState(SD_HandleTypeDef *hsd)
{
  return (HAL_SD_CardStateTypeDef)SD_GetState();
}

HAL_StatusTypeDef HAL_SD_GetCardInfo(SD_HandleTypeDef *hsd, HAL_SD_CardInfoTypeDef *pCardInfo)
{
  HAL_StatusTypeDef status = HAL_OK;

  hsd->error = SD_GetCardInfo(&SDCardInfo);
  status = hsd->error == SD_OK ? HAL_OK:HAL_ERROR;

  if (status == HAL_OK)
  {
    pCardInfo->CardType     = SDCardInfo.CardType;
    pCardInfo->CardVersion  = SDCardInfo.SD_csd.SysSpecVersion;
    pCardInfo->Class        = SDCardInfo.SD_csd.CardComdClasses;
    pCardInfo->RelCardAdd   = SDCardInfo.RCA;
    pCardInfo->BlockNbr     = SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;
    pCardInfo->BlockSize    = SDCardInfo.CardBlockSize;
    pCardInfo->LogBlockNbr  = pCardInfo->BlockNbr;
    pCardInfo->LogBlockSize = pCardInfo->BlockSize;
  }

  return status;  
}

HAL_StatusTypeDef HAL_SD_Clock(uint32_t *p_clock)
{
  uint32_t sdio_clk;

  sdio_clk = RCM_ReadHCLKFreq() / ((SDIO->CLKCTRL & 0xFF) + 2);
  *p_clock = sdio_clk;

  return HAL_OK;
}