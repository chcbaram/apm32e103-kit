#include "ws2812.h"



#ifdef _USE_HW_WS2812
#include "cli.h"

#define BIT_PERIOD      (78)  // 1300ns, 60Mhz
#define BIT_HIGH        (42)  // 700ns
#define BIT_LOW         (21)  // 350ns
#define BIT_ZERO        (50)

bool is_init = false;


typedef struct
{
  TMR_T *h_timer;  
  uint16_t led_cnt;
} ws2812_t;

static uint16_t bit_buf[BIT_ZERO + 24*HW_WS2812_MAX_CH];


ws2812_t ws2812;

#if CLI_USE(HW_WS2812)
static void cliCmd(cli_args_t *args);
#endif
static bool ws2812InitHw(void);





bool ws2812Init(void)
{
  TMR_BaseConfig_T TMR_TimeBaseStruct;  
  TMR_OCConfig_T   OCcongigStruct;  


  memset(bit_buf, 0, sizeof(bit_buf));
  
  ws2812.h_timer = TMR3;

  ws2812InitHw();

  // Timer 
  //
  RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_TMR3);

  TMR_TimeBaseStruct.clockDivision = TMR_CLOCK_DIV_1;
  TMR_TimeBaseStruct.countMode     = TMR_COUNTER_MODE_UP;
  TMR_TimeBaseStruct.division      = 1;
  TMR_TimeBaseStruct.period        = 78-1;
  TMR_ConfigTimeBase(ws2812.h_timer, &TMR_TimeBaseStruct);

  OCcongigStruct.idleState    = TMR_OC_IDLE_STATE_RESET;
  OCcongigStruct.mode         = TMR_OC_MODE_PWM1;
  OCcongigStruct.nIdleState   = TMR_OC_NIDLE_STATE_RESET;
  OCcongigStruct.nPolarity    = TMR_OC_NPOLARITY_HIGH;
  OCcongigStruct.outputNState = TMR_OC_NSTATE_ENABLE;
  OCcongigStruct.outputState  = TMR_OC_STATE_ENABLE;
  OCcongigStruct.polarity     = TMR_OC_POLARITY_HIGH;
  OCcongigStruct.pulse        = 0;
  TMR_ConfigOC4(ws2812.h_timer, &OCcongigStruct);

  TMR_EnableDMASoure(ws2812.h_timer, TMR_DMA_SOURCE_UPDATE);

  TMR_ConfigOC4Preload(ws2812.h_timer, TMR_OC_PRELOAD_ENABLE);
  TMR_EnableAutoReload(ws2812.h_timer);
  TMR_Enable(ws2812.h_timer);
  TMR_EnablePWMOutputs(ws2812.h_timer);

  ws2812.led_cnt = WS2812_MAX_CH;
  is_init = true;

  for (int i=0; i<WS2812_MAX_CH; i++)
  {
    ws2812SetColor(i, WS2812_COLOR_OFF);
  }
  ws2812Refresh();

#if CLI_USE(HW_WS2812)
  cliAdd("ws2812", cliCmd);
#endif
  return true;
}

bool ws2812InitHw(void)
{
  GPIO_Config_T GPIO_ConfigStruct;


  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);

  /* TMR3_CH4 */
  GPIO_ConfigStruct.pin   = GPIO_PIN_1;
  GPIO_ConfigStruct.mode  = GPIO_MODE_AF_PP;
  GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
  GPIO_Config(GPIOB, &GPIO_ConfigStruct);



  DMA_Config_T dmaConfig;

  /* Enable DMA Clock */
  RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

  /* DMA config */
  dmaConfig.peripheralBaseAddr = (uint32_t)&ws2812.h_timer->CC4;
  dmaConfig.memoryBaseAddr     = (uint32_t)bit_buf;
  dmaConfig.dir                = DMA_DIR_PERIPHERAL_DST;
  dmaConfig.bufferSize         = sizeof(bit_buf);
  dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
  dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
  dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_HALFWORD;
  dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_HALFWORD;
  dmaConfig.loopMode           = DMA_MODE_NORMAL;
  dmaConfig.priority           = DMA_PRIORITY_LOW;
  dmaConfig.M2M                = DMA_M2MEN_DISABLE;

  /* Config TMR3 UP DMA channel */
  DMA_Config(DMA1_Channel3, &dmaConfig);

  /* Enable DMA */
  DMA_Enable(DMA1_Channel3);

  return true;
}

bool ws2812Refresh(void)
{
  DMA_Disable(DMA1_Channel3);
  // 전송 사이즈는 둘중에 가장 큰 단위로 계산해야 하는 것 같다.
  DMA_ConfigDataNumber(DMA1_Channel3, sizeof(bit_buf));
  DMA_Enable(DMA1_Channel3);
  return true;
}

void ws2812SetColor(uint32_t ch, uint32_t color)
{
  uint16_t r_bit[8];
  uint16_t g_bit[8];
  uint16_t b_bit[8];
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint32_t offset;

  if (ch >= WS2812_MAX_CH)
    return;

  red   = (color >> 16) & 0xFF;
  green = (color >> 8) & 0xFF;
  blue  = (color >> 0) & 0xFF;


  for (int i=0; i<8; i++)
  {
    if (red & (1<<7))
    {
      r_bit[i] = BIT_HIGH;
    }
    else
    {
      r_bit[i] = BIT_LOW;
    }
    red <<= 1;

    if (green & (1<<7))
    {
      g_bit[i] = BIT_HIGH;
    }
    else
    {
      g_bit[i] = BIT_LOW;
    }
    green <<= 1;

    if (blue & (1<<7))
    {
      b_bit[i] = BIT_HIGH;
    }
    else
    {
      b_bit[i] = BIT_LOW;
    }
    blue <<= 1;
  }

  offset = BIT_ZERO;

  memcpy(&bit_buf[offset + ch*24 + 8*0], g_bit, 8*2);
  memcpy(&bit_buf[offset + ch*24 + 8*1], r_bit, 8*2);
  memcpy(&bit_buf[offset + ch*24 + 8*2], b_bit, 8*2);
}

#if CLI_USE(HW_LED)
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("ws2812 led cnt : %d\n", WS2812_MAX_CH);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test"))
  {
    uint32_t color[6] = {WS2812_COLOR_RED,
                         WS2812_COLOR_OFF,
                         WS2812_COLOR_GREEN,
                         WS2812_COLOR_OFF,
                         WS2812_COLOR_BLUE,
                         WS2812_COLOR_OFF};

    uint8_t color_idx = 0;

    while(cliKeepLoop())
    {
      ws2812SetColor(0, color[color_idx]);
      ws2812Refresh();
      color_idx = (color_idx + 1) % 6;

      delay(500);
    }
    ret = true;
  }


  if (args->argc == 5 && args->isStr(0, "color"))
  {
    uint8_t  ch;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    ch    = (uint8_t)args->getData(1);
    red   = (uint8_t)args->getData(2);
    green = (uint8_t)args->getData(3);
    blue  = (uint8_t)args->getData(4);

    ws2812SetColor(ch, WS2812_COLOR(red, green, blue));
    ws2812Refresh();

    while(cliKeepLoop())
    {
    }
    ws2812SetColor(0, 0);
    ws2812Refresh();
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("ws2812 info\n");
    cliPrintf("ws2812 test\n");
    cliPrintf("ws2812 color ch r g b\n");
  }
}
#endif

#endif