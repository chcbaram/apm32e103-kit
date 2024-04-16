#include "led.h"


#ifdef _USE_HW_LED
#include "cli.h"


const typedef struct 
{
  GPIO_T    *port;
  GPIO_PIN_T pin;
  uint8_t    on_state;
  uint8_t    off_state;
} led_tbl_t;


#if CLI_USE(HW_LED)
static void cliLed(cli_args_t *args);
#endif


static const led_tbl_t led_tbl[LED_MAX_CH] = 
{
  {GPIOC, GPIO_PIN_13, _DEF_LOW, _DEF_HIGH},   // 0. LED1
};

#if CLI_USE(HW_LED)
static const char *led_name[LED_MAX_CH] = 
{
  "0_LED_B",   
  "1_LED_L",
};
#endif


bool ledInit(void)
{
  GPIO_Config_T   GPIO_InitStructure;


  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOC);

  GPIO_InitStructure.mode = GPIO_MODE_OUT_PP;
  GPIO_InitStructure.speed = GPIO_SPEED_50MHz;

  for (int i=0; i<LED_MAX_CH; i++)
  {
    GPIO_InitStructure.pin = led_tbl[i].pin;
    GPIO_Config(led_tbl[i].port, &GPIO_InitStructure);

    ledOff(i);
  }

#if CLI_USE(HW_LED)
  cliAdd("led", cliLed);
#endif
  return true;
}

void ledOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  GPIO_WriteBitValue(led_tbl[ch].port, led_tbl[ch].pin, led_tbl[ch].on_state);
}

void ledOff(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  GPIO_WriteBitValue(led_tbl[ch].port, led_tbl[ch].pin, led_tbl[ch].off_state);
}

void ledToggle(uint8_t ch)
{
  uint8_t out_state;

  if (ch >= LED_MAX_CH) return;


  out_state = GPIO_ReadOutputBit(led_tbl[ch].port, led_tbl[ch].pin);
  GPIO_WriteBitValue(led_tbl[ch].port, led_tbl[ch].pin, !out_state);
}

#if CLI_USE(HW_LED)
void cliLed(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<LED_MAX_CH; i++)
    {
      cliPrintf("%-12s \n", led_name[i]);
    }
    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "toggle"))
  {
    uint8_t  ch;
    uint32_t toggle_time;
    uint32_t pre_time;

    ch = (uint8_t)args->getData(1);
    ch = constrain(ch, 0, LED_MAX_CH-1);
    toggle_time = (uint32_t)args->getData(2);

    pre_time = millis();
    while(cliKeepLoop())
    {
      if (millis()-pre_time >= toggle_time)
      {
        pre_time = millis();
        ledToggle(ch);
      }
    }
    ledOff(ch);
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("led info\n");
    cliPrintf("led toggle 0~%d ms\n", LED_MAX_CH);
  }
}
#endif

#endif