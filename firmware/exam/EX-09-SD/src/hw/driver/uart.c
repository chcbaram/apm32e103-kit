#include "uart.h"

#ifdef _USE_HW_UART
#include "qbuffer.h"
#include "cli.h"
#ifdef _USE_HW_USB
#include "cdc.h"
#endif


#define UART_RX_BUF_LENGTH        1024


typedef struct
{
  const char         *p_msg;
  USART_T            *p_uart;
  USART_Config_T     *p_cfg;
  DMA_Channel_T      *p_hdma_rx;
} uart_hw_t;


typedef struct
{
  bool     is_open;
  uint32_t baud;

  uint8_t   rx_buf[UART_RX_BUF_LENGTH];
  qbuffer_t qbuffer;
  uart_hw_t *p_hw;

  uint32_t rx_cnt;
  uint32_t tx_cnt;
} uart_tbl_t;





#ifdef _USE_HW_CLI
static void cliUart(cli_args_t *args);
#endif
static bool uartInitHw(uint8_t ch);


static bool is_init = false;

__attribute__((section(".non_cache")))
static uart_tbl_t uart_tbl[UART_MAX_CH];

static USART_Config_T uart1_cfg;


const static uart_hw_t uart_hw_tbl[UART_MAX_CH] = 
  {
    {"USART1 SWD   ", USART1, &uart1_cfg, DMA1_Channel5},
  };





bool uartInit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud = 57600;
    uart_tbl[i].rx_cnt = 0;
    uart_tbl[i].tx_cnt = 0;    
  }

  is_init = true;

#ifdef _USE_HW_CLI
  cliAdd("uart", cliUart);
#endif
  return true;
}

bool uartDeInit(void)
{
  return true;
}

bool uartIsInit(void)
{
  return is_init;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;


  if (ch >= UART_MAX_CH) return false;

  if (uart_tbl[ch].is_open == true && uart_tbl[ch].baud == baud)
  {
    return true;
  }


  switch(ch)
  {
    case _DEF_UART1:
      uart_tbl[ch].baud = baud;

      uart_tbl[ch].p_hw = (uart_hw_t *)&uart_hw_tbl[ch];

      uart_tbl[ch].p_hw->p_cfg->baudRate     = baud;
      uart_tbl[ch].p_hw->p_cfg->hardwareFlow = USART_HARDWARE_FLOW_NONE;
      uart_tbl[ch].p_hw->p_cfg->mode         = USART_MODE_TX_RX;
      uart_tbl[ch].p_hw->p_cfg->parity       = USART_PARITY_NONE;
      uart_tbl[ch].p_hw->p_cfg->stopBits     = USART_STOP_BIT_1;
      uart_tbl[ch].p_hw->p_cfg->wordLength   = USART_WORD_LEN_8B;


      qbufferCreate(&uart_tbl[ch].qbuffer, &uart_tbl[ch].rx_buf[0], UART_RX_BUF_LENGTH);


      uartInitHw(ch);

      USART_Config(uart_tbl[ch].p_hw->p_uart, uart_tbl[ch].p_hw->p_cfg);
      USART_Enable(uart_tbl[ch].p_hw->p_uart);

      USART_EnableDMA(uart_tbl[ch].p_hw->p_uart, USART_DMA_RX);

      ret = true;
      uart_tbl[ch].is_open = true;


      uart_tbl[ch].qbuffer.in  = uart_tbl[ch].qbuffer.len - uart_tbl[ch].p_hw->p_hdma_rx->CHNDATA_B.NDATA;
      uart_tbl[ch].qbuffer.out = uart_tbl[ch].qbuffer.in;
      break;

    case _DEF_UART2:
      uart_tbl[ch].baud    = baud;
      uart_tbl[ch].is_open = true;
      ret = true;
      break;      
  }

  return ret;
}

bool uartClose(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  uart_tbl[ch].is_open = false;

  return true;
}

bool uartInitHw(uint8_t ch)
{
  bool ret = true;
  GPIO_Config_T GPIO_configStruct;
  DMA_Config_T dmaConfig;


  switch(ch)
  {
    case _DEF_UART1:
      RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);
      RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_USART1);

      /* Configure USART Tx as alternate function push-pull */
      GPIO_configStruct.mode  = GPIO_MODE_AF_PP;
      GPIO_configStruct.pin   = GPIO_PIN_9;
      GPIO_configStruct.speed = GPIO_SPEED_50MHz;
      GPIO_Config(GPIOA, &GPIO_configStruct);

      /* Configure USART Rx as input floating */
      GPIO_configStruct.mode = GPIO_MODE_IN_FLOATING;
      GPIO_configStruct.pin  = GPIO_PIN_10;
      GPIO_Config(GPIOA, &GPIO_configStruct);


      /* Enable DMA Clock */
      RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

      /* DMA config */
      dmaConfig.peripheralBaseAddr = (uint32_t)&uart_tbl[ch].p_hw->p_uart->DATA;
      dmaConfig.memoryBaseAddr     = (uint32_t)uart_tbl[ch].rx_buf;
      dmaConfig.dir                = DMA_DIR_PERIPHERAL_SRC;
      dmaConfig.bufferSize         = UART_RX_BUF_LENGTH;
      dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
      dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
      dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
      dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_BYTE;
      dmaConfig.loopMode           = DMA_MODE_CIRCULAR;
      dmaConfig.priority           = DMA_PRIORITY_MEDIUM;
      dmaConfig.M2M                = DMA_M2MEN_DISABLE;

      /* Enable DMA channel */
      DMA_Config(uart_tbl[ch].p_hw->p_hdma_rx, &dmaConfig);

      /* Enable DMA */
      DMA_Enable(uart_tbl[ch].p_hw->p_hdma_rx);
      break;

    default:
      ret = false;
      break;
  }

  return ret;
}


uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;


  switch(ch)
  {
    case _DEF_UART1:
      uart_tbl[ch].qbuffer.in = uart_tbl[ch].qbuffer.len - uart_tbl[ch].p_hw->p_hdma_rx->CHNDATA_B.NDATA;
      ret = qbufferAvailable(&uart_tbl[ch].qbuffer);      
      break;

    case _DEF_UART2:
      #ifdef _USE_HW_USB
      ret = cdcAvailable();
      #endif
      break;      
  }

  return ret;
}

bool uartFlush(uint8_t ch)
{
  uint32_t pre_time;


  pre_time = millis();
  while(uartAvailable(ch))
  {
    if (millis()-pre_time >= 10)
    {
      break;
    }
    uartRead(ch);
  }

  return true;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;


  switch(ch)
  {
    case _DEF_UART1:
      qbufferRead(&uart_tbl[ch].qbuffer, &ret, 1);
      break;

    case _DEF_UART2:
      #ifdef _USE_HW_USB
      ret = cdcRead();
      #endif
      break;      
  }
  uart_tbl[ch].rx_cnt++;

  return ret;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;
  uint32_t pre_time;
  uint32_t index;


  pre_time = millis();
  switch(ch)
  {
    case _DEF_UART1:
      index = 0;
      while(millis()-pre_time < 100)
      {
        if (USART_ReadStatusFlag(USART1, USART_FLAG_TXBE) == SET)
        {
          USART_TxData(uart_tbl[ch].p_hw->p_uart, p_data[index]);
          index++;
          if (index >= length)
          {
            ret = index;
            break;
          }
        }
      }
      break;

    case _DEF_UART2:
      #ifdef _USE_HW_USB
      ret = cdcWrite(p_data, length);
      #endif
      break;      
  }
  uart_tbl[ch].tx_cnt += ret;

  return ret;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;
  uint32_t ret;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);

  ret = uartWrite(ch, (uint8_t *)buf, len);

  va_end(args);


  return ret;
}

uint32_t uartGetBaud(uint8_t ch)
{
  uint32_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;

  #ifdef _USE_HW_USB
  if (ch == HW_UART_CH_USB)
    ret = cdcGetBaud();
  else
    ret = uart_tbl[ch].baud;
  #else
  ret = uart_tbl[ch].baud;
  #endif
  
  return ret;
}

uint32_t uartGetRxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].rx_cnt;
}

uint32_t uartGetTxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].tx_cnt;
}


#ifdef _USE_HW_CLI
void cliUart(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<UART_MAX_CH; i++)
    {
      cliPrintf("_DEF_UART%d : %s, %d bps\n", i+1, uart_hw_tbl[i].p_msg, uartGetBaud(i));
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "test"))
  {
    uint8_t uart_ch;

    uart_ch = constrain(args->getData(1), 1, UART_MAX_CH) - 1;

    if (uart_ch != cliGetPort())
    {
      uint8_t rx_data;

      while(1)
      {
        if (uartAvailable(uart_ch) > 0)
        {
          rx_data = uartRead(uart_ch);
          cliPrintf("<- _DEF_UART%d RX : 0x%X\n", uart_ch + 1, rx_data);
        }

        if (cliAvailable() > 0)
        {
          rx_data = cliRead();
          if (rx_data == 'q')
          {
            break;
          }
          else
          {
            uartWrite(uart_ch, &rx_data, 1);
            cliPrintf("-> _DEF_UART%d TX : 0x%X\n", uart_ch + 1, rx_data);            
          }
        }
      }
    }
    else
    {
      cliPrintf("This is cliPort\n");
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("uart info\n");
    cliPrintf("uart test ch[1~%d]\n", HW_UART_MAX_CH);
  }
}
#endif


#endif

