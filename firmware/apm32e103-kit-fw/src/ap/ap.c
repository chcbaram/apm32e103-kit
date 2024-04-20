#include "ap.h"


void sdMain(void);


void apInit(void)
{
  cliOpen(HW_UART_CH_CLI, 115200);
  cliLogo();
}

extern volatile uint32_t i2s_cnt;

void apMain(void)
{
  uint32_t pre_time;

  pre_time = millis();
  while(1)
  {
    static bool color_enable = false;
    if (buttonGetPressed(_DEF_BUTTON1))
    {
      delay(50);
      while(buttonGetPressed(_DEF_BUTTON1));
      color_enable ^= 1;
    }

    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);

      uint32_t color[6] = {WS2812_COLOR_RED,
                           WS2812_COLOR_OFF,
                           WS2812_COLOR_GREEN,
                           WS2812_COLOR_OFF,
                           WS2812_COLOR_BLUE,
                           WS2812_COLOR_OFF};

      static uint8_t color_idx = 0;
      if (color_enable)
      {
        ws2812SetColor(0, color[color_idx]);
        ws2812Refresh();
        color_idx = (color_idx + 1) % 6;
      }
      else
      {
        ws2812SetColor(0, 0);
        ws2812Refresh();
      }      
    }

    cliMain(); 
    sdMain();
  }
}

void sdMain(void)
{
  sd_state_t sd_state;


  sd_state = sdUpdate();
  if (sd_state == SDCARD_CONNECTED)
  {
    fatfsReInit();
    logPrintf("\nSDCARD_CONNECTED\n");
  }
  if (sd_state == SDCARD_DISCONNECTED)
  {
    logPrintf("\nSDCARD_DISCONNECTED\n");
  }
}