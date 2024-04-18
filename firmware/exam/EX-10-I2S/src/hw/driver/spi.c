#include "spi.h"



#ifdef _USE_HW_SPI

#define SPI_TX_DMA_MAX_LENGTH   0xFFFF



typedef struct
{
  SPI_T         *h_spi;
  SPI_Config_T  *p_cfg;
  DMA_Channel_T *h_hdma_tx;
  DMA_Channel_T *h_hdma_rx;
} spi_hw_t;

typedef struct
{
  bool is_open;
  bool is_tx_done;
  bool is_rx_done;
  bool is_error;

  void (*func_tx)(void);

  spi_hw_t *p_hw;
} spi_t;



static spi_t spi_tbl[SPI_MAX_CH];
static SPI_Config_T spi1_cfg;

const static spi_hw_t spi_hw_tbl[SPI_MAX_CH] = 
  {
    {SPI1, &spi1_cfg, DMA1_Channel3, DMA1_Channel2},
  };

static bool spiInitHw(uint8_t ch);






bool spiInit(void)
{
  bool ret = true;


  for (int i=0; i<SPI_MAX_CH; i++)
  {
    spi_tbl[i].is_open    = false;
    spi_tbl[i].is_tx_done = true;
    spi_tbl[i].is_rx_done = true;
    spi_tbl[i].is_error   = false;
    spi_tbl[i].func_tx    = NULL;
    spi_tbl[i].p_hw       = (spi_hw_t *)&spi_hw_tbl[i];
  }

  return ret;
}

bool spiBegin(uint8_t ch)
{
  bool ret = false;
  spi_t *p_spi = &spi_tbl[ch];

  switch(ch)
  {
    case _DEF_SPI1:

      SPI_ConfigStructInit(p_spi->p_hw->p_cfg);

      p_spi->p_hw->p_cfg->mode        = SPI_MODE_MASTER;
      p_spi->p_hw->p_cfg->length      = SPI_DATA_LENGTH_8B;
      p_spi->p_hw->p_cfg->baudrateDiv = SPI_BAUDRATE_DIV_2;
      p_spi->p_hw->p_cfg->direction   = SPI_DIRECTION_2LINES_FULLDUPLEX;
      p_spi->p_hw->p_cfg->firstBit    = SPI_FIRSTBIT_MSB;
      p_spi->p_hw->p_cfg->polarity    = SPI_CLKPOL_LOW;
      p_spi->p_hw->p_cfg->nss         = SPI_NSS_SOFT;
      p_spi->p_hw->p_cfg->phase       = SPI_CLKPHA_1EDGE;


      spiInitHw(ch);

      SPI_Config(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg);
      SPI_Enable(p_spi->p_hw->h_spi);
      SPI_DisableCRC(p_spi->p_hw->h_spi);

      p_spi->is_open = true;
      ret = true;
      break;
  }

  return ret;
}

bool spiInitHw(uint8_t ch)
{
  bool ret = true;
  GPIO_Config_T gpioConfig;


  switch(ch)
  {
    case _DEF_SPI1:
      /* Enable related Clock */
      RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SPI1);
      RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);

      /* config PIN_6  MISO*/
      gpioConfig.pin   = GPIO_PIN_6;
      gpioConfig.mode  = GPIO_MODE_IN_PU;
      gpioConfig.speed = GPIO_SPEED_50MHz;
      GPIO_Config(GPIOA, &gpioConfig);

      /* config PIN_5->SCK , PIN_7->MOSI */
      gpioConfig.pin   = GPIO_PIN_5 | GPIO_PIN_7;
      gpioConfig.mode  = GPIO_MODE_AF_PP;
      gpioConfig.speed = GPIO_SPEED_50MHz;
      GPIO_Config(GPIOA, &gpioConfig);
      break;

    default:
      ret = false;
      break;
  }

  return ret;
}


void spiSetDataMode(uint8_t ch, uint8_t dataMode)
{
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return;


  switch( dataMode )
  {
    // CPOL=0, CPHA=0
    case SPI_MODE0:
      p_spi->p_hw->p_cfg->polarity    = SPI_CLKPOL_LOW;
      p_spi->p_hw->p_cfg->phase       = SPI_CLKPHA_1EDGE;      
      SPI_Config(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg);
      break;

    // CPOL=0, CPHA=1
    case SPI_MODE1:
      p_spi->p_hw->p_cfg->polarity    = SPI_CLKPOL_LOW;
      p_spi->p_hw->p_cfg->phase       = SPI_CLKPHA_2EDGE;      
      SPI_Config(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg);      
      break;

    // CPOL=1, CPHA=0
    case SPI_MODE2:
      p_spi->p_hw->p_cfg->polarity    = SPI_CLKPOL_HIGH;
      p_spi->p_hw->p_cfg->phase       = SPI_CLKPHA_1EDGE;      
      SPI_Config(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg);         
      break;

    // CPOL=1, CPHA=1
    case SPI_MODE3:
      p_spi->p_hw->p_cfg->polarity    = SPI_CLKPOL_HIGH;
      p_spi->p_hw->p_cfg->phase       = SPI_CLKPHA_2EDGE;      
      SPI_Config(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg);         
      break;
  }
}

void spiSetBitWidth(uint8_t ch, uint8_t bit_width)
{
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false) return;

  
  switch(bit_width)
  {
    case 16:
      p_spi->p_hw->p_cfg->length = SPI_DATA_LENGTH_16B;
      SPI_ConfigDataSize(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg->length);
      break;

    default:
      p_spi->p_hw->p_cfg->length = SPI_DATA_LENGTH_8B;
      SPI_ConfigDataSize(p_spi->p_hw->h_spi, p_spi->p_hw->p_cfg->length);
      break;
  }
}

bool spiTransmitReceive8(uint8_t ch, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t length, uint32_t timeout)
{
  bool      ret = true;
  spi_hw_t *p_hw = spi_tbl[ch].p_hw;
  uint32_t pre_time;


  pre_time = millis();
  for (int i=0; i<length; i++)
  {
    uint8_t tx_data = 0;

    if (tx_buf != NULL)
      tx_data = tx_buf[i];


    SPI_I2S_TxData(p_hw->h_spi, tx_data);

    while (1)
    {
      if (SPI_I2S_ReadStatusFlag(p_hw->h_spi, SPI_FLAG_TXBE) == SET)    // 전송 완료
      {
        if (SPI_I2S_ReadStatusFlag(p_hw->h_spi, SPI_FLAG_RXBNE) == SET) // 수신 데이터 있음?
        {
          uint8_t rx_data;

          rx_data = SPI_I2S_RxData(p_hw->h_spi);
          if (rx_buf != NULL)
            rx_buf[i] = rx_data;
          break;
        }
      }

      if (millis()-pre_time >= timeout)
      {
        ret = false;
        break;
      }
    }

    if (ret == false)
      break;
  }

  return ret;
}

bool spiTransmitReceive16(uint8_t ch, uint16_t *tx_buf, uint16_t *rx_buf, uint32_t length, uint32_t timeout)
{
  bool      ret = true;
  spi_hw_t *p_hw = spi_tbl[ch].p_hw;
  uint32_t pre_time;


  pre_time = millis();
  for (int i=0; i<length; i++)
  {
    uint16_t tx_data = 0;

    if (tx_buf != NULL)
      tx_data = tx_buf[i];
      
    SPI_I2S_TxData(p_hw->h_spi, tx_data);

    while (1)
    {
      if (SPI_I2S_ReadStatusFlag(p_hw->h_spi, SPI_FLAG_TXBE) == SET)    // 전송 완료
      {
        if (SPI_I2S_ReadStatusFlag(p_hw->h_spi, SPI_FLAG_RXBNE) == SET) // 수신 데이터 있음?
        {
          uint16_t rx_data;

          rx_data = SPI_I2S_RxData(p_hw->h_spi);

          if (rx_buf != NULL)
            rx_buf[i] = rx_data;
          break;
        }
      }

      if (millis()-pre_time >= timeout)
      {
        ret = false;
        break;
      }
    }
  }

  return ret;
}

uint8_t spiTransfer8(uint8_t ch, uint8_t data)
{
  uint8_t ret = 0;
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false) return 0;

  spiTransmitReceive8(ch, &data, &ret, 1, 10);

  return ret;
}

uint16_t spiTransfer16(uint8_t ch, uint16_t data)
{
  uint8_t tBuf[2];
  uint8_t rBuf[2];
  uint16_t ret;
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false) return 0;

  if (p_spi->p_hw->p_cfg->length == SPI_DATA_LENGTH_8B)
  {
    tBuf[1] = (uint8_t)data;
    tBuf[0] = (uint8_t)(data>>8);
    spiTransmitReceive8(ch, (uint8_t *)&tBuf, (uint8_t *)&rBuf, 2, 10);

    ret = rBuf[0];
    ret <<= 8;
    ret += rBuf[1];
  }
  else
  {
    spiTransmitReceive16(ch, &data, &ret, 1, 10);
  }

  return ret;
}

bool spiTransfer(uint8_t ch, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t length, uint32_t timeout)
{
  bool ret = true;
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false) return false;


  ret = spiTransmitReceive8(ch, tx_buf, rx_buf, length, timeout);

  return ret;
}

bool spiTransferDMA(uint8_t ch, uint8_t *tx_buf, uint8_t *rx_buf, uint32_t length, uint32_t timeout)
{
  #if 0
  bool ret = false;
  HAL_StatusTypeDef status;
  spi_t  *p_spi = &spi_tbl[ch];
  bool is_dma = false;

  if (p_spi->is_open == false) return false;

  if (rx_buf == NULL)
  {
    status = HAL_SPI_Transmit(p_spi->h_spi, tx_buf, length, timeout);
  }
  else if (tx_buf == NULL)
  {
    p_spi->is_rx_done = false;
    status = HAL_SPI_Receive_DMA(p_spi->h_spi, rx_buf, length);
    is_dma = true;
  }
  else
  {
    status = HAL_SPI_TransmitReceive(p_spi->h_spi, tx_buf, rx_buf, length, timeout);
  }

  if (status == HAL_OK)
  {
    uint32_t pre_time;

    ret = true;
    pre_time = millis();
    if (is_dma == true)
    {
      while(1)
      {
        if(p_spi->is_rx_done == true)
          break;

        if((millis()-pre_time) >= timeout)
        {
          ret = false;
          break;
        }
      }
    }
  }

  return ret;
  #else
  return false;
  #endif
}

void spiDmaTxStart(uint8_t spi_ch, uint8_t *p_buf, uint32_t length)
{
  #if 0
  spi_t  *p_spi = &spi_tbl[spi_ch];

  if (p_spi->is_open == false) return;

  p_spi->is_tx_done = false;
  HAL_SPI_Transmit_DMA(p_spi->h_spi, p_buf, length);
  #endif
}

bool spiDmaTxTransfer(uint8_t ch, void *buf, uint32_t length, uint32_t timeout)
{
  #if 0
  bool ret = true;
  uint32_t t_time;


  spiDmaTxStart(ch, (uint8_t *)buf, length);

  t_time = millis();

  if (timeout == 0) return true;

  while(1)
  {
    if(spiDmaTxIsDone(ch))
    {
      break;
    }
    if((millis()-t_time) > timeout)
    {
      ret = false;
      break;
    }
  }

  return ret;
  #else 
  return false;
  #endif
}

bool spiDmaTxIsDone(uint8_t ch)
{
  spi_t  *p_spi = &spi_tbl[ch];

  if (p_spi->is_open == false)     return true;

  return p_spi->is_tx_done;
}

void spiAttachTxInterrupt(uint8_t ch, void (*func)())
{
  spi_t  *p_spi = &spi_tbl[ch];


  if (p_spi->is_open == false)     return;

  p_spi->func_tx = func;
}


#endif