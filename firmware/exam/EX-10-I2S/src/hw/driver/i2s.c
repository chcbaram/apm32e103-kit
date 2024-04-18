#include "i2s.h"


#ifdef _USE_HW_I2S
#include "cli.h"
#include "gpio.h"
#include "qbuffer.h"
#include "buzzer.h"
#include "files.h"
#include "mixer.h"
#include "nvs.h"
#include <math.h>

#define I2S_CFG_NAME     "cfg.i2s"
#define _PIN_GPIO_SPK_EN I2S_MUTE

#if HW_I2S_LCD > 0
#include "lcd.h"
#define FFT_LEN         512
#define BLOCK_X_CNT     12
#define BLOCK_Y_CNT     12

typedef struct
{
  uint32_t pre_time_lcd;
  uint8_t  update_cnt;
  q15_t    buf_q15[FFT_LEN*2];

  uint8_t block_target[BLOCK_X_CNT];
  uint8_t block_peak[BLOCK_X_CNT];
  uint8_t block_value[BLOCK_X_CNT];

} i2s_cli_t;

static void drawBlock(int16_t bx, int16_t by, uint16_t color);
static bool lcdUpdate(i2s_cli_t *p_args);
#endif

typedef struct
{
  int16_t volume;
} i2s_cfg_t;

typedef struct
{
  SPI_T         *h_i2s;
  I2S_Config_T  *p_cfg;
  DMA_Channel_T *h_hdma_tx;
} i2s_hw_t;

#define I2S_SAMPLERATE_HZ       16000
#define I2S_BUF_MS              (4)
#define I2S_BUF_LEN             (8*1024)
#define I2S_BUF_FRAME_LEN       ((48000 * 2 * I2S_BUF_MS) / 1000)  // 48Khz, Stereo, 4ms


#if CLI_USE(HW_I2S)
static void cliI2s(cli_args_t *args);
#endif

static bool is_init = false;
static bool is_started = false;
static bool is_busy = false;
static uint32_t i2s_sample_rate = I2S_SAMPLERATE_HZ;

__attribute__((section(".non_cache")))
static int16_t  i2s_frame_buf[I2S_BUF_FRAME_LEN * 2];
static uint32_t i2s_frame_len = 0;
static int16_t  i2s_volume = 100;
static mixer_t   mixer;
static i2s_cfg_t i2s_cfg;


static I2S_Config_T i2s3_cfg;

const static i2s_hw_t i2s_hw = {SPI3, &i2s3_cfg, DMA2_Channel2};


static bool i2sInitHw(void);
static bool i2sUpdateDMA(void);





bool i2sInit(void)
{
  bool ret = true;

  mixerInit(&mixer);
  i2sCfgLoad();

  i2s_frame_len = (i2s_sample_rate * 2 * I2S_BUF_MS) / 1000;


  i2sInitHw();

  i2s_hw.p_cfg->mode       = I2S_MODE_MASTER_TX;
  i2s_hw.p_cfg->standard   = I2S_STANDARD_PHILLIPS;
  i2s_hw.p_cfg->length     = I2S_DATA_LENGHT_16B;
  i2s_hw.p_cfg->MCLKOutput = I2S_MCLK_OUTPUT_DISABLE;
  i2s_hw.p_cfg->audioDiv   = I2S_AUDIO_DIV_16K;
  i2s_hw.p_cfg->polarity   = I2S_CLKPOL_LOW;

  I2S_Config(i2s_hw.h_i2s, i2s_hw.p_cfg);
  

  i2sStart();

  delay(50);
  gpioPinWrite(_PIN_GPIO_SPK_EN, _DEF_HIGH);

  is_init = ret;

  logPrintf("[%s] i2sInit()\n", ret ? "OK" : "NG");
  
#if CLI_USE(HW_I2S)
  cliAdd("i2s", cliI2s);
#endif

  return ret;
}

bool i2sInitHw(void)
{
  bool ret = true;
  GPIO_Config_T gpioConfig;
  DMA_Config_T dmaConfig;


  SPI_I2S_Reset(i2s_hw.h_i2s);

  /* Enable related Clock */
  RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_SPI3);
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_AFIO);
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);


  GPIO_ConfigPinRemap(GPIO_REMAP_SWJ_JTAGDISABLE);

  gpioConfig.pin   = GPIO_PIN_3 | GPIO_PIN_5;
  gpioConfig.mode  = GPIO_MODE_AF_PP;
  gpioConfig.speed = GPIO_SPEED_50MHz;
  GPIO_Config(GPIOB, &gpioConfig);

  gpioConfig.pin   = GPIO_PIN_15;
  gpioConfig.mode  = GPIO_MODE_AF_PP;
  gpioConfig.speed = GPIO_SPEED_50MHz;
  GPIO_Config(GPIOA, &gpioConfig);

  /* Enable DMA Clock */
  RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA2);

  NVIC_EnableIRQRequest(DMA2_Channel2_IRQn, 0, 0);

  /* DMA config */
  DMA_Reset(i2s_hw.h_hdma_tx);

  dmaConfig.peripheralBaseAddr = (uint32_t)&i2s_hw.h_i2s->DATA;
  dmaConfig.memoryBaseAddr     = (uint32_t)i2s_frame_buf;
  dmaConfig.dir                = DMA_DIR_PERIPHERAL_DST;
  dmaConfig.bufferSize         = i2s_frame_len * 2;
  dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
  dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
  dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_HALFWORD;
  dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_HALFWORD;
  dmaConfig.loopMode           = DMA_MODE_CIRCULAR;
  dmaConfig.priority           = DMA_PRIORITY_MEDIUM;
  dmaConfig.M2M                = DMA_M2MEN_DISABLE;

  /* Enable DMA channel */
  DMA_Config(i2s_hw.h_hdma_tx, &dmaConfig);

  DMA_EnableInterrupt(i2s_hw.h_hdma_tx, DMA_INT_TC | DMA_INT_HT | DMA_INT_TERR);
  
  /* Enable DMA */
  DMA_Enable(i2s_hw.h_hdma_tx);

  return ret;
}

bool i2sUpdateDMA(void)
{
  bool ret = true;
  DMA_Config_T dmaConfig;


  DMA_Disable(i2s_hw.h_hdma_tx);


  dmaConfig.peripheralBaseAddr = (uint32_t)&i2s_hw.h_i2s->DATA;
  dmaConfig.memoryBaseAddr     = (uint32_t)i2s_frame_buf;
  dmaConfig.dir                = DMA_DIR_PERIPHERAL_DST;
  dmaConfig.bufferSize         = i2s_frame_len * 2;
  dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
  dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
  dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_HALFWORD;
  dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_HALFWORD;
  dmaConfig.loopMode           = DMA_MODE_CIRCULAR;
  dmaConfig.priority           = DMA_PRIORITY_MEDIUM;
  dmaConfig.M2M                = DMA_M2MEN_DISABLE;

  /* Enable DMA channel */
  DMA_Config(i2s_hw.h_hdma_tx, &dmaConfig);

  DMA_EnableInterrupt(i2s_hw.h_hdma_tx, DMA_INT_TC | DMA_INT_HT | DMA_INT_TERR);
  
  /* Enable DMA */
  DMA_Enable(i2s_hw.h_hdma_tx);

  return ret;
}

bool i2sCfgLoad(void)
{
  bool ret = true;

  #ifdef _USE_HW_NVS
  if (nvsGet(I2S_CFG_NAME, &i2s_cfg, sizeof(i2s_cfg)) == true)
  {
    i2s_volume = i2s_cfg.volume;
  }
  else
  {
    i2s_cfg.volume = i2s_volume;
    ret = nvsSet(I2S_CFG_NAME, &i2s_cfg, sizeof(i2s_cfg));
    logPrintf("[NG] i2sCfgLoad()\n");
  }  
  #else
  i2s_cfg.volume = i2s_volume;
  #endif
  i2sSetVolume(i2s_cfg.volume);
  return ret;
}

bool i2sCfgSave(void)
{
  bool ret = true;

  i2s_cfg.volume = i2s_volume;
  #ifdef _USE_HW_NVS
  ret = nvsSet(I2S_CFG_NAME, &i2s_cfg, sizeof(i2s_cfg));
  #endif
  return ret;
}

bool i2sIsBusy(void)
{
  return is_busy;
}

bool i2sSetSampleRate(uint32_t freq)
{
  bool ret = true;
  uint32_t frame_len;
  const uint32_t freq_tbl[7] = 
  {
    I2S_AUDIO_DIV_48K,  
    I2S_AUDIO_DIV_44K,
    I2S_AUDIO_DIV_32K,
    I2S_AUDIO_DIV_22K,
    I2S_AUDIO_DIV_16K,
    I2S_AUDIO_DIV_11K,
    I2S_AUDIO_DIV_8K,
  };

  ret = false;
  for (int i=0; i<7; i++)
  {
    if (freq_tbl[i] == freq)
    {
      ret = true;
      break;
    }
  }
  if (ret != true)
  {
    return false;
  }

  i2sStop();
  delay(10);

  i2s_sample_rate = freq;
  frame_len = (i2s_sample_rate * 2 * I2S_BUF_MS) / 1000;
  i2s_frame_len = frame_len;


  gpioPinWrite(_PIN_GPIO_SPK_EN, _DEF_LOW);

  i2s_hw.p_cfg->audioDiv   = freq;
  I2S_Config(i2s_hw.h_i2s, i2s_hw.p_cfg);

  i2sStart();

  gpioPinWrite(_PIN_GPIO_SPK_EN, _DEF_HIGH);

  return ret;
}

uint32_t i2sGetSampleRate(void)
{
  return i2s_sample_rate;
}

bool i2sStart(void)
{
  bool ret = false;

  memset(i2s_frame_buf, 0, sizeof(i2s_frame_buf));

  I2S_Disable(i2s_hw.h_i2s);

  i2s_frame_len = (i2s_sample_rate * 2 * I2S_BUF_MS) / 1000;
  i2sUpdateDMA();

  I2S_Enable(i2s_hw.h_i2s);
  SPI_I2S_EnableDMA(i2s_hw.h_i2s, SPI_I2S_DMA_REQ_TX);


  is_started = true;

  return ret;
}

bool i2sStop(void)
{
  is_started = false;
  SPI_I2S_DisableDMA(i2s_hw.h_i2s, SPI_I2S_DMA_REQ_TX);
  I2S_Disable(i2s_hw.h_i2s);

  return true;
}

int8_t i2sGetEmptyChannel(void)
{
  return mixerGetEmptyChannel(&mixer);
}

uint32_t i2sGetFrameSize(void)
{
  return i2s_frame_len;
}

uint32_t i2sAvailableForWrite(uint8_t ch)
{
  return mixerAvailableForWrite(&mixer, ch);
}

bool i2sWrite(uint8_t ch, int16_t *p_data, uint32_t length)
{
  return mixerWrite(&mixer, ch, p_data, length);
}

uint32_t i2sWriteTimeout(uint8_t ch, int16_t *p_data, uint32_t length, uint32_t timeout)
{
  uint32_t pre_time;
  uint32_t buf_i;
  uint32_t remain;
  uint32_t wr_len;

  buf_i = 0;
  pre_time = millis();
  while(buf_i < length)
  {
    remain = length - buf_i;
    wr_len = cmin(mixerAvailableForWrite(&mixer, ch), remain);

    mixerWrite(&mixer, ch, &p_data[buf_i], wr_len);
    buf_i += wr_len;

    if (millis()-pre_time >= timeout)
    {
      break;
    }
    #ifdef _USE_HW_RTOS
    delay(1);
    #endif
  }

  return buf_i;
}

// https://m.blog.naver.com/PostView.nhn?blogId=hojoon108&logNo=80145019745&proxyReferer=https:%2F%2Fwww.google.com%2F
//
float i2sGetNoteHz(int8_t octave, int8_t note)
{
  float hz;
  float f_note;

  if (octave < 1) octave = 1;
  if (octave > 8) octave = 8;

  if (note <  1) note = 1;
  if (note > 12) note = 12;

  f_note = (float)(note-10)/12.0f;

  hz = pow(2, (octave-1)) * 55 * pow(2, f_note);

  return hz;
}

// https://gamedev.stackexchange.com/questions/4779/is-there-a-faster-sine-function
//
float i2sSin(float x)
{
  const float B = 4 / M_PI;
  const float C = -4 / (M_PI * M_PI);

  return -(B * x + C * x * ((x < 0) ? -x : x));
}

bool i2sPlayBeep(uint32_t freq_hz, uint16_t volume, uint32_t time_ms)
{
  uint32_t pre_time;
  int32_t sample_rate = i2s_sample_rate;
  int32_t num_samples = i2s_frame_len;
  float sample_point;
  int16_t sample[num_samples];
  int16_t sample_index = 0;
  float div_freq;
  int8_t mix_ch;
  int32_t volume_out;


  volume = constrain(volume, 0, 100);
  volume_out = (INT16_MAX/40) * volume / 100;

  mix_ch =  i2sGetEmptyChannel();

  div_freq = (float)sample_rate/(float)freq_hz;

  pre_time = millis();
  while(millis()-pre_time <= time_ms)
  {
    if (i2sAvailableForWrite(mix_ch) >= num_samples)
    {
      for (int i=0; i<num_samples; i+=2)
      {
        sample_point = i2sSin(2.0f * M_PI * (float)(sample_index) / ((float)div_freq));
        sample[i + 0] = (int16_t)(sample_point * volume_out);
        sample[i + 1] = (int16_t)(sample_point * volume_out);
        sample_index = (sample_index + 1) % (int)(div_freq);
      }
      i2sWrite(mix_ch, (int16_t *)sample, num_samples);
    }
    delay(2);
  }

  return true;
}

#if HW_I2S_LCD > 0
bool i2sPlayBeepLcd(i2s_cli_t *p_i2s_cli, uint32_t freq_hz, uint16_t volume, uint32_t time_ms)
{
  uint32_t pre_time;
  int32_t sample_rate = i2s_sample_rate;
  int32_t num_samples = i2s_frame_len;
  float sample_point;
  int16_t sample[num_samples];
  int16_t sample_index = 0;
  float div_freq;
  int8_t mix_ch;
  int32_t volume_out;
  uint32_t q15_buf_index = 0;


  volume = constrain(volume, 0, 100);
  volume_out = (INT16_MAX/40) * volume / 100;

  mix_ch =  i2sGetEmptyChannel();

  div_freq = (float)sample_rate/(float)freq_hz;

  pre_time = millis();
  while(millis()-pre_time <= time_ms)
  {
    if (i2sAvailableForWrite(mix_ch) >= num_samples)
    {
      for (int i=0; i<num_samples; i+=2)
      {
        sample_point = i2sSin(2.0f * M_PI * (float)(sample_index) / ((float)div_freq));
        sample[i + 0] = (int16_t)(sample_point * volume_out);
        sample[i + 1] = (int16_t)(sample_point * volume_out);
        sample_index = (sample_index + 1) % (int)(div_freq);

        if (q15_buf_index < 512)
        {
          p_i2s_cli->buf_q15[q15_buf_index*2 + 0] = sample[i + 0];
          p_i2s_cli->buf_q15[q15_buf_index*2 + 1] = 0;
          
          q15_buf_index++;            
        }
      }
      i2sWrite(mix_ch, (int16_t *)sample, num_samples);
    }
    delay(1);

    if (q15_buf_index == 512)
    {        
      if (lcdUpdate(p_i2s_cli) == true)
      {
        q15_buf_index = 0;
      }        
    }
  }

  lcdLogoOn();

  return true;
}
#endif

int16_t i2sGetVolume(void)
{
  return i2s_volume;
}

bool i2sSetVolume(int16_t volume)
{
  volume = constrain(volume, 0, 100);
  i2s_volume = volume;

  mixerSetVolume(&mixer, i2s_volume);
  return true;
}

void i2sUpdateBuffer(uint8_t index)
{
  if (mixerAvailable(&mixer) >= i2s_frame_len)
  {    
    mixerRead(&mixer, &i2s_frame_buf[index * i2s_frame_len], i2s_frame_len);
    is_busy = true;
  }
  else
  {
    memset(&i2s_frame_buf[index * i2s_frame_len], 0, i2s_frame_len * 2);
    is_busy = false;
  }
}


void DMA2_Channel2_IRQHandler(void)
{
  if (DMA_ReadIntFlag(DMA2_INT_FLAG_HT2))
  {
    i2sUpdateBuffer(0);
    DMA_ClearIntFlag(DMA2_INT_FLAG_HT2);
  }
  else if (DMA_ReadIntFlag(DMA2_INT_FLAG_TC2))
  {
    i2sUpdateBuffer(1);
    DMA_ClearIntFlag(DMA2_INT_FLAG_TC2);
  }
  else
  {
    DMA_ClearIntFlag(DMA2_INT_FLAG_TERR2);
  }
}


#if CLI_USE(HW_I2S)

typedef struct wavfile_header_s
{
  char    ChunkID[4];     /*  4   */
  int32_t ChunkSize;      /*  4   */
  char    Format[4];      /*  4   */

  char    Subchunk1ID[4]; /*  4   */
  int32_t Subchunk1Size;  /*  4   */
  int16_t AudioFormat;    /*  2   */
  int16_t NumChannels;    /*  2   */
  int32_t SampleRate;     /*  4   */
  int32_t ByteRate;       /*  4   */
  int16_t BlockAlign;     /*  2   */
  int16_t BitsPerSample;  /*  2   */

  char    Subchunk2ID[4];
  int32_t Subchunk2Size;
} wavfile_header_t;

#if HW_I2S_LCD > 0
static void drawBlock(int16_t bx, int16_t by, uint16_t color)
{
  int16_t x;
  int16_t y;
  int16_t bw;
  int16_t bh;
  int16_t top_space = 32;
  int16_t sw;
  int16_t sh;

  sw = lcdGetWidth();
  sh = lcdGetHeight()-top_space;

  bw = (sw / BLOCK_X_CNT);
  bh = (sh / BLOCK_Y_CNT);

  x = bx*bw;
  y = sh - bh*by - bh;

  lcdDrawFillRect(x, y+top_space, bw-2, bh-2, color);
}

bool lcdUpdate(i2s_cli_t *p_args)
{
  bool ret = false;

  if (millis()-p_args->pre_time_lcd >= 50 && lcdDrawAvailable() == true)
  {
    p_args->pre_time_lcd = millis();

    lcdClearBuffer(black);

    arm_cfft_q15(&arm_cfft_sR_q15_len512, p_args->buf_q15, 0, 1);

    int16_t xi;

    xi = 0;
    for (int i=0; i<BLOCK_X_CNT; i++)
    {
      int32_t h;
      int32_t max_h;


      max_h = 0;
      for (int j=0; j<FFT_LEN/2/BLOCK_X_CNT; j++)
      {
        h = p_args->buf_q15[2*xi + 1];
        h = constrain(h, 0, 500);
        h = cmap(h, 0, 500, 0, 80);
        if (h > max_h)
        {
          max_h = h;
        }
        xi++;
      }
      h = cmap(max_h, 0, 80, 0, BLOCK_Y_CNT-1);

      p_args->block_target[i] = h;

      if (p_args->update_cnt%2 == 0)
      {
        if (p_args->block_peak[i] > 0)
        {
          p_args->block_peak[i]--;
        }
      }
      if (h >= p_args->block_peak[i])
      {
        p_args->block_peak[i] = h;
        p_args->block_value[i] = h;
      }
    }

    p_args->update_cnt++;

    for (int i=0; i<BLOCK_X_CNT; i++)
    {
      drawBlock(i, p_args->block_peak[i], red);

      if (p_args->block_value[i] > p_args->block_target[i])
      {
        p_args->block_value[i]--;
      }
      for (int j=0; j<p_args->block_value[i]; j++)
      {
        drawBlock(i, j, yellow);
      }
    }

    lcdRequestDraw();
    ret = true;
  }

  return ret;
}
#endif


void cliI2s(cli_args_t *args)
{
  bool ret = false;

  #if HW_I2S_LCD > 0
  uint32_t q15_buf_index = 0;
  i2s_cli_t i2s_args;

  memset(i2s_args.block_peak, 0, sizeof(i2s_args.block_peak));
  memset(i2s_args.block_value, 0, sizeof(i2s_args.block_value));
  memset(i2s_args.block_target, 0, sizeof(i2s_args.block_target));
  #endif


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {

    cliPrintf("i2s init      : %d\n", is_init);
    cliPrintf("i2s rate      : %d Khz\n", i2s_sample_rate/1000);
    cliPrintf("i2s buf ms    : %d ms\n", I2S_BUF_MS);
    cliPrintf("i2s frame len : %d \n", i2s_frame_len);
    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "beep") == true)
  {
    uint32_t freq;
    uint32_t time_ms;

    freq = args->getData(1);
    time_ms = args->getData(2);
    
    #if HW_I2S_LCD > 0
    i2sPlayBeepLcd(&i2s_args, freq, 100, time_ms);
    #else
    i2sPlayBeep(freq, 100, time_ms);
    #endif

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "melody"))
  {
    uint16_t melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    int note_durations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };

    for (int i=0; i<8; i++) 
    {
      int note_duration = 1000 / note_durations[i];

      i2sPlayBeep(melody[i], 100, note_duration);
      delay(note_duration * 0.3);    
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "play-wav") == true)
  {
    char *file_name;
    FILE *fp;
    wavfile_header_t header;
    uint32_t r_len;
    int32_t  volume = 100;
    int8_t ch;

    file_name = args->getStr(1);

    cliPrintf("FileName      : %s\n", file_name);


    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
      cliPrintf("fopen fail : %s\n", file_name);
      return;
    }
    fread(&header, sizeof(wavfile_header_t), 1, fp);

    cliPrintf("ChunkSize     : %d\n", header.ChunkSize);
    cliPrintf("Format        : %c%c%c%c\n", header.Format[0], header.Format[1], header.Format[2], header.Format[3]);
    cliPrintf("Subchunk1Size : %d\n", header.Subchunk1Size);
    cliPrintf("AudioFormat   : %d\n", header.AudioFormat);
    cliPrintf("NumChannels   : %d\n", header.NumChannels);
    cliPrintf("SampleRate    : %d\n", header.SampleRate);
    cliPrintf("ByteRate      : %d\n", header.ByteRate);
    cliPrintf("BlockAlign    : %d\n", header.BlockAlign);
    cliPrintf("BitsPerSample : %d\n", header.BitsPerSample);
    cliPrintf("Subchunk2Size : %d\n", header.Subchunk2Size);


    i2sSetSampleRate(header.SampleRate);

    r_len = i2sGetFrameSize()/2;

    int16_t buf_frame[i2sGetFrameSize()];

    fseek(fp, sizeof(wavfile_header_t) + 1024, SEEK_SET);

    ch = i2sGetEmptyChannel();

    while(cliKeepLoop())
    {
      int len;


      if (i2sAvailableForWrite(ch) >= i2s_frame_len)
      {
        len = fread(buf_frame, r_len, 2*header.NumChannels, fp);

        if (len != r_len*2*header.NumChannels)
        {
          break;
        }

        int16_t buf_data[2];

        for (int i=0; i<r_len; i++)
        {
          if (header.NumChannels == 2)
          {
            buf_data[0] = buf_frame[i*2 + 0] * volume / 100;;
            buf_data[1] = buf_frame[i*2 + 1] * volume / 100;;
          }
          else
          {
            buf_data[0] = buf_frame[i] * volume / 100;;
            buf_data[1] = buf_frame[i] * volume / 100;;
          }

          #if HW_I2S_LCD > 0
          if (q15_buf_index < FFT_LEN)
          {
            i2s_args.buf_q15[q15_buf_index*2 + 0] = buf_data[0];
            i2s_args.buf_q15[q15_buf_index*2 + 1] = 0;
            
            q15_buf_index++;            
          }
          #endif

          i2sWrite(ch, (int16_t *)buf_data, 2);
        }

        #if HW_I2S_LCD > 0
        if (q15_buf_index == FFT_LEN)
        {        
          if (lcdUpdate(&i2s_args) == true)
          {
            q15_buf_index = 0;
          }        
        }
        #endif
      }
    }
    fclose(fp);

    #if HW_I2S_LCD > 0
    lcdLogoOn();
    #endif

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("i2s info\n");
    cliPrintf("i2s melody\n");
    cliPrintf("i2s beep freq time_ms\n");
    cliPrintf("i2s play-wav filename\n");
  }
}
#endif

#endif
