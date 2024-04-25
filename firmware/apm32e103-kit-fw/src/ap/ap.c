#include "ap.h"


void updateLED(void);
void updateSD(void);
void updateWiznet(void);
void updateLCD(void);





void apInit(void)
{
  cliOpen(HW_UART_CH_CLI, 115200);
  cliLogo();

  for (int i = 0; i < 32; i += 1) 
  {
    lcdClearBuffer(black);
    lcdPrintfResize(0, 40 - i, green, 16, "  -- BARAM --");
    lcdDrawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, white);
    lcdUpdateDraw();
    delay(10);
  }  
  delay(500);
  lcdClear(black);
}


void apMain(void)
{
  while(1)
  {
    cliMain();

    updateLED();
    updateLCD();
    // updateWiznet();
  }
}

void updateLED(void)
{
  static uint32_t pre_time = 0;
  
  
  if (millis() - pre_time >= 500)
  {
    pre_time = millis();
    ledToggle(_DEF_LED1);
  }
}

void updateSD(void)
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

void updateWiznet(void)
{
  eventUpdate();
  wiznetUpdate();  
}

void updateLCD(void)
{

  if (lcdDrawAvailable())
  {
    lcdClearBuffer(black);

    if (wiznetIsLink() == false)
    {
      lcdPrintf(0, 8, white, "Not Connected");        
    }
    else 
    {
      if (wiznetIsGetIP() == true)
      {
        if (buttonGetPressed(_DEF_BUTTON1))
        {
           wiznet_info_t net_info;

          wiznetGetInfo(&net_info);

          lcdPrintf(0,  0, white,
                    "IP %d.%d.%d.%d", 
                    net_info.ip[0], 
                    net_info.ip[1],
                    net_info.ip[2],
                    net_info.ip[3]);
          lcdPrintf(0, 16, white,
                    "DHCP : %s\n", wiznetIsGetIP() ? "True":"False");
        }
        else
        {
          rtc_time_t rtc_time;
          rtc_date_t rtc_date;
          const char *week_str[] = {"일", "월", "화", "수", "목", "금", "토"};

          rtcGetTime(&rtc_time);
          rtcGetDate(&rtc_date);

          lcdPrintf(0, 0, white,
                    "%02d-%02d-%02d (%s)",
                    rtc_date.year, rtc_date.month, rtc_date.day, week_str[rtc_date.week]);

          lcdPrintf(0, 16, white,
                    "%02d:%02d:%02d",
                    rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
        }
      }
      else
      {
        lcdPrintf(0, 8, white, "Getting_IP..");        
      }
    }

    lcdRequestDraw();
  }
}