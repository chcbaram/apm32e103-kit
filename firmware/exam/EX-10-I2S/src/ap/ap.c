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
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
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