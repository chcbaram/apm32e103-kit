#include "adc.h"



#ifdef _USE_HW_ADC
#include "cli.h"
#include "cli_gui.h"


#define NAME_DEF(x)  x, #x

#ifdef _USE_HW_RTOS
#define lock()      xSemaphoreTake(mutex_lock, portMAX_DELAY);
#define unLock()    xSemaphoreGive(mutex_lock);
#else
#define lock()      
#define unLock()    
#endif



typedef struct
{
  ADC_T         *h_adc;
  ADC_Config_T  *p_cfg;
  ADC_CHANNEL_T  channel;
  uint8_t        rank;
  DMA_Channel_T *h_hdma_rx;
  AdcPinName_t   pin_name;
  const char    *p_name;
} adc_tbl_t;


#if CLI_USE(HW_ADC)
static void cliAdc(cli_args_t *args);
#endif
static bool adcInitHw(void);



#ifdef _USE_HW_RTOS
static SemaphoreHandle_t mutex_lock;
#endif
static bool is_init = false;

static uint16_t adc_data_buf[ADC_MAX_CH];
static int32_t adc_cali = 4095/4;

static ADC_Config_T  adc1_cfg;  

static adc_tbl_t adc_tbl[ADC_MAX_CH] = 
  {
    {ADC1, &adc1_cfg, ADC_CHANNEL_8, 1, DMA1_Channel1, NAME_DEF(LIGHT_ADC)},
  };




bool adcInit(void)
{
  bool ret = true;  

#ifdef _USE_HW_RTOS
  mutex_lock = xSemaphoreCreateMutex();
#endif

  adcInitHw();

  ADC_Reset(ADC1);

  ADC_ConfigStructInit(&adc1_cfg);
  adc1_cfg.mode              = ADC_MODE_INDEPENDENT;
  adc1_cfg.scanConvMode      = DISABLE;
  adc1_cfg.continuosConvMode = ENABLE;
  adc1_cfg.externalTrigConv  = ADC_EXT_TRIG_CONV_None;
  adc1_cfg.dataAlign         = ADC_DATA_ALIGN_RIGHT;
  adc1_cfg.nbrOfChannel      = 1;
  ADC_Config(ADC1, &adc1_cfg);

  /* ADC channel Convert configuration */
  ADC_ConfigRegularChannel(ADC1, ADC_CHANNEL_8, 1, ADC_SAMPLETIME_239CYCLES5);

  /* Enable ADC DMA */
  ADC_EnableDMA(ADC1);

  /* Enable ADC */
  ADC_Enable(ADC1);

  /* Enable ADC1 reset calibration register */
  ADC_ResetCalibration(ADC1);
  while (ADC_ReadResetCalibrationStatus(ADC1));

  ADC_StartCalibration(ADC1);
  while (ADC_ReadCalibrationStartFlag(ADC1));

  /* Start ADC1 Software Conversion */
  ADC_EnableSoftwareStartConv(ADC1);


  is_init = ret;
  adc_cali = true;

  logPrintf("[%s] adcInit()\n", is_init ? "OK":"NG");

#if CLI_USE(HW_ADC)
  cliAdd("adc", cliAdc);
#endif
  return ret;
}

bool adcInitHw(void)
{
  GPIO_Config_T gpioConfig;


  /* Enable GPIOA clock */
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);

  /* Configure PB0 (ADC Channel8) as analog input */
  GPIO_ConfigStructInit(&gpioConfig);
  gpioConfig.mode = GPIO_MODE_ANALOG;
  gpioConfig.pin  = GPIO_PIN_0;
  GPIO_Config(GPIOB, &gpioConfig);

  /* ADCCLK = PCLK2/8 */
  RCM_ConfigADCCLK(RCM_PCLK2_DIV_8);

  /* Enable ADC clock */
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_ADC1);


  DMA_Config_T dmaConfig;

  /* Enable DMA Clock */
  RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

  /* DMA config */
  dmaConfig.peripheralBaseAddr = ((uint32_t)ADC1_BASE + 0x4C);
  dmaConfig.memoryBaseAddr     = (uint32_t)adc_data_buf;
  dmaConfig.dir                = DMA_DIR_PERIPHERAL_SRC;
  dmaConfig.bufferSize         = ADC_MAX_CH;
  dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
  dmaConfig.memoryInc          = DMA_MEMORY_INC_DISABLE;
  dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_HALFWORD;
  dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_HALFWORD;
  dmaConfig.loopMode           = DMA_MODE_CIRCULAR;
  dmaConfig.priority           = DMA_PRIORITY_LOW;
  dmaConfig.M2M                = DMA_M2MEN_DISABLE;

  /* Enable DMA channel */
  DMA_Config(DMA1_Channel1, &dmaConfig);

  /* Enable DMA */
  DMA_Enable(DMA1_Channel1);

  return true;
}

bool adcIsInit(void)
{
  return is_init;
}

int32_t adcRead(uint8_t ch)
{
  return adc_data_buf[ch];
}

int32_t adcRead8(uint8_t ch)
{
  return adcRead(ch)>>4;
}

int32_t adcRead10(uint8_t ch)
{
  return adcRead(ch)>>2;
}

int32_t adcRead12(uint8_t ch)
{
  return adcRead(ch)>>0;
}

int32_t adcRead16(uint8_t ch)
{
  return adcRead(ch)<<4;  
}

uint8_t adcGetRes(uint8_t ch)
{
  return 12;
}

float adcReadVoltage(uint8_t ch)
{
  return adcConvVoltage(ch, adcRead(ch));
}

float adcConvVoltage(uint8_t ch, uint32_t adc_value)
{
  float ret = 0;


  switch (ch)
  {
    default :
      ret  = ((float)adc_value * 3.3f) / (4095.f);
      break;
  }

  return ret;
}


#if CLI_USE(HW_ADC)
void cliAdc(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("adc init : %d\n", is_init);
    cliPrintf("adc res  : %d\n", adcGetRes(0));
    for (int i=0; i<ADC_MAX_CH; i++)
    {
      cliPrintf("%02d. %-32s : %04d\n", i, adc_tbl[i].p_name, adcRead(i));
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    cliShowCursor(false);
    while(cliKeepLoop())
    {
      for (int i=0; i<ADC_MAX_CH; i++)
      {
        cliPrintf("%02d. %-32s : %04d \n", i, adc_tbl[i].p_name, adcRead(i));
      }
      delay(50);
      cliPrintf("\x1B[%dA", ADC_MAX_CH);
    }
    cliPrintf("\x1B[%dB", ADC_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") && args->isStr(1, "vol"))
  {
    cliShowCursor(false);
    while(cliKeepLoop())
    {
      for (int i=0; i<ADC_MAX_CH; i++)
      {
        float adc_data;

        adc_data = adcReadVoltage(i);

        cliPrintf("%02d. %-32s : %d.%02dV \n",i, adc_tbl[i].p_name, (int)adc_data, (int)(adc_data * 100)%100);
      }
      delay(50);
      cliPrintf("\x1B[%dA", ADC_MAX_CH);
    }
    cliPrintf("\x1B[%dB", ADC_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "graph"))
  {
    while(cliKeepLoop())
    {
      for (int i=0; i<ADC_MAX_CH; i++)
      {
        float adc_data;

        adc_data = adcReadVoltage(i);

        cliPrintf(">%s:%d\n",adc_tbl[i].p_name, (int)(adc_data*1000));
      }
      delay(50);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("adc info\n");
    cliPrintf("adc show\n");
    cliPrintf("adc show vol\n");
    cliPrintf("adc graph\n");
  }
}
#endif

#endif