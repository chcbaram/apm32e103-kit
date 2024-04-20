#include "gpio.h"


#ifdef _USE_HW_GPIO
#include "cli.h"
#include "cli_gui.h"

#define NAME_DEF(x)  x, #x

typedef enum
{
  GPIO_HW,
  GPIO_EX,
} GpioType_t;

typedef struct
{
  GpioType_t    type;
  GPIO_T       *port;
  GPIO_PIN_T    pin;
  uint8_t       mode;
  uint8_t       on_state;
  uint8_t       off_state;
  bool          init_value;
  GpioPinName_t pin_name;
  const char   *p_name;
} gpio_tbl_t;


static const gpio_tbl_t gpio_tbl[GPIO_MAX_CH] =
    {
      {GPIO_HW, GPIOC, GPIO_PIN_7 ,  _DEF_INPUT  , _DEF_HIGH, _DEF_LOW,  _DEF_HIGH, NAME_DEF(SD_DETECT) },  
      {GPIO_HW, GPIOA, GPIO_PIN_4 ,  _DEF_OUTPUT , _DEF_HIGH, _DEF_LOW,  _DEF_HIGH, NAME_DEF(SPI_CS)    },  
      {GPIO_HW, GPIOB, GPIO_PIN_4 ,  _DEF_OUTPUT , _DEF_HIGH, _DEF_LOW,  _DEF_HIGH, NAME_DEF(I2S_MUTE)  }, 
      {GPIO_HW, GPIOB, GPIO_PIN_12,  _DEF_OUTPUT , _DEF_HIGH, _DEF_LOW,  _DEF_HIGH, NAME_DEF(SPI2_CS)    },  
      {GPIO_HW, GPIOA, GPIO_PIN_0 ,  _DEF_OUTPUT , _DEF_HIGH, _DEF_LOW,  _DEF_HIGH, NAME_DEF(W5500_RST)  },  
      {GPIO_HW, GPIOC, GPIO_PIN_4 ,  _DEF_INPUT  , _DEF_HIGH, _DEF_LOW,  _DEF_HIGH, NAME_DEF(W5500_INT)  },  
    };


static uint8_t gpio_data[GPIO_MAX_CH];

#if CLI_USE(HW_GPIO)
static void cliGpio(cli_args_t *args);
#endif







bool gpioInit(void)
{
  bool ret = true;


  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);  
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOC);


  for (int i=0; i<GPIO_MAX_CH; i++)
  {
    if (gpio_tbl[i].mode & _DEF_OUTPUT)
    {
      gpioPinWrite(i, gpio_tbl[i].init_value);
    }
    gpioPinMode(i, gpio_tbl[i].mode);
  }

#if CLI_USE(HW_GPIO)
  cliAdd("gpio", cliGpio);
#endif

  return ret;
}

bool gpioPinMode(uint8_t ch, uint8_t mode)
{
  bool ret = true;
  GPIO_Config_T GPIO_InitStruct = {0};


  if (ch >= GPIO_MAX_CH)
  {
    return false;
  }

  switch(mode)
  {
    case _DEF_INPUT:
      GPIO_InitStruct.mode = GPIO_MODE_IN_FLOATING;
      break;

    case _DEF_INPUT_PULLUP:
      GPIO_InitStruct.mode = GPIO_MODE_IN_PU;
      break;

    case _DEF_INPUT_PULLDOWN:
      GPIO_InitStruct.mode = GPIO_MODE_IN_PD;
      break;

    case _DEF_OUTPUT:
      GPIO_InitStruct.mode = GPIO_MODE_OUT_PP;
      break;

    case _DEF_OUTPUT_PULLUP:
      GPIO_InitStruct.mode = GPIO_MODE_OUT_PP;
      break;

    case _DEF_OUTPUT_PULLDOWN:
      GPIO_InitStruct.mode = GPIO_MODE_OUT_PP;
      break;
  }

  GPIO_InitStruct.speed = GPIO_SPEED_50MHz;
  GPIO_InitStruct.pin   = gpio_tbl[ch].pin;
  GPIO_Config(gpio_tbl[ch].port, &GPIO_InitStruct);

  return ret;
}

void gpioPinWrite(uint8_t ch, uint8_t value)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }

  if (value == _DEF_HIGH)
  {
    GPIO_WriteBitValue(gpio_tbl[ch].port, gpio_tbl[ch].pin, gpio_tbl[ch].on_state);
  }
  else
  {
    GPIO_WriteBitValue(gpio_tbl[ch].port, gpio_tbl[ch].pin, gpio_tbl[ch].off_state);
  }

  gpio_data[ch] = value;
}

uint8_t gpioPinRead(uint8_t ch)
{
  uint8_t ret = _DEF_LOW;

  if (ch >= GPIO_MAX_CH)
  {
    return ret;
  }

  if (gpio_tbl[ch].mode & _DEF_OUTPUT)
  {
    if (GPIO_ReadOutputBit(gpio_tbl[ch].port, gpio_tbl[ch].pin) == gpio_tbl[ch].on_state)
    {
      ret = _DEF_HIGH;
    }
  }
  else
  {
    if (GPIO_ReadInputBit(gpio_tbl[ch].port, gpio_tbl[ch].pin) == gpio_tbl[ch].on_state)
    {
      ret = _DEF_HIGH;
    }
  }  
  gpio_data[ch] = ret;
  return ret;
}

void gpioPinToggle(uint8_t ch)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }
  
  gpio_data[ch] = !gpio_data[ch];
  gpioPinWrite(ch, gpio_data[ch]);
}





#if CLI_USE(HW_GPIO)
void cliGpio(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    for (int i=0; i<GPIO_MAX_CH; i++)
    {
      cliPrintf("%02d. %s %s %-16s - %d\n", 
        i,
        gpio_tbl[i].mode & _DEF_INPUT ? "I":"O", 
        gpio_tbl[i].type == GPIO_HW ? "HW":"EX",
        gpio_tbl[i].p_name, 
        gpioPinRead(i));
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    while(cliKeepLoop())
    {
      for (int i=0; i<GPIO_MAX_CH; i++)
      {
        cliPrintf("%d", gpioPinRead(i));
      }
      cliPrintf("\n");
      delay(100);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") && args->isStr(1, "input"))
  {
    uint16_t line = 0;

    cliShowCursor(false);
    while(cliKeepLoop())
    {  
      line = 0;
      for (int i=0; i<GPIO_MAX_CH; i++)
      {
        if (gpio_tbl[i].mode & _DEF_INPUT)
        {
          cliPrintf("%02d. %s %s %-16s - %d\n", 
            i,
            gpio_tbl[i].mode & _DEF_INPUT ? "I":"O", 
            gpio_tbl[i].type == GPIO_HW ? "HW":"EX",
            gpio_tbl[i].p_name, 
            gpioPinRead(i));
          line++;
        }
      }
      cliPrintf("\x1B[%dA", line);
    }
    cliPrintf("\x1B[%dB", line);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") && args->isStr(1, "output"))
  {
    uint16_t line = 0;

    cliShowCursor(false);
    while(cliKeepLoop())
    {  
      line = 0;
      for (int i=0; i<GPIO_MAX_CH; i++)
      {
        if (gpio_tbl[i].mode & _DEF_OUTPUT)
        {
          cliPrintf("%02d. %s %s %-16s - %d\n", 
            i,
            gpio_tbl[i].mode & _DEF_INPUT ? "I":"O", 
            gpio_tbl[i].type == GPIO_HW ? "HW":"EX",
            gpio_tbl[i].p_name, 
            gpioPinRead(i));
          line++;
        }
      }
      cliPrintf("\x1B[%dA", line);
    }
    cliPrintf("\x1B[%dB", line);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") && args->isStr(1, "ex"))
  {
    uint16_t line = 0;

    cliShowCursor(false);
    while(cliKeepLoop())
    {  
      line = 0;
      for (int i=0; i<GPIO_MAX_CH; i++)
      {
        if (gpio_tbl[i].type == GPIO_EX)
        {
          cliPrintf("%02d. %s %s %-16s - %d\n", 
            i,
            gpio_tbl[i].mode & _DEF_INPUT ? "I":"O", 
            gpio_tbl[i].type == GPIO_HW ? "HW":"EX",
            gpio_tbl[i].p_name, 
            gpioPinRead(i));
          line++;
        }
      }
      cliPrintf("\x1B[%dA", line);
    }
    cliPrintf("\x1B[%dB", line);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read") == true)
  {
    uint8_t ch;

    ch = (uint8_t)args->getData(1);

    while(cliKeepLoop())
    {
      cliPrintf("gpio read %s %d : %d\n", gpio_tbl[ch].p_name, ch, gpioPinRead(ch));
      delay(100);
    }

    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "write") == true)
  {
    uint8_t ch;
    uint8_t data;

    ch   = (uint8_t)args->getData(1);
    data = (uint8_t)args->getData(2);

    gpioPinWrite(ch, data);

    cliPrintf("gpio write %s %d : %d\n", gpio_tbl[ch].p_name, ch, data);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("gpio info\n");
    cliPrintf("gpio show\n");
    cliPrintf("gpio show input\n");
    cliPrintf("gpio show output\n");
    cliPrintf("gpio show ex\n");
    cliPrintf("gpio read ch[0~%d]\n", GPIO_MAX_CH-1);
    cliPrintf("gpio write ch[0~%d] 0:1\n", GPIO_MAX_CH-1);
  }
}
#endif


#endif