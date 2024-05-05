#include "ap.h"
#include "modules.h"


void updateLED(void);
void updateSD(void);
void updateWiznet(void);
void updateLCD(void);
void updateCMD(void);




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
  cmdTaskInit();

  while(1)
  {
    cliMain();

    updateLED();
    updateLCD();
    updateSD();
    updateWiznet();
    updateCMD();
  }
}

void updateCMD(void)
{
  cmdTaskUpdate();
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
  int16_t        x_offset = 10;
  static uint8_t menu     = 0;
  uint8_t        menu_max = 5;


  if (!lcdIsInit())
  {
    return;
  }

  if (buttonGetPressed(_DEF_BUTTON1))
  {
    delay(10);
    while(buttonGetPressed(_DEF_BUTTON1));
    
    menu = (menu + 1) % menu_max;
  }


  if (lcdDrawAvailable())
  {
    lcdClearBuffer(black);

    lcdDrawRect(0, 0, 4, 32, white);
    for (int i=0; i<menu_max; i++)
    {
      if (i == menu)
        lcdDrawFillRect(0, i*(32/menu_max), 4, (32/menu_max), white);
    }


    if (menu == 0)
    {
      if (wiznetIsLink() == true)
      {
        if (wiznetIsGetIP() == true)
        {
          wiznet_info_t net_info;

          wiznetGetInfo(&net_info);

          lcdPrintf(x_offset,  0, white,
                    "IP %d.%d.%d.%d", 
                    net_info.ip[0], 
                    net_info.ip[1],
                    net_info.ip[2],
                    net_info.ip[3]);
          lcdPrintf(x_offset, 16, white,
                    "DHCP : %s\n", wiznetIsGetIP() ? "True":"False");          
        }
        else
        {
          lcdPrintf(x_offset, 8, white, "Getting_IP..");        
        }      
      }
      else
      {
        lcdPrintf(x_offset, 8, white, "Not Connected");        
      }
    }

    if (menu == 1)
    {
      rtc_time_t rtc_time;
      rtc_date_t rtc_date;
      const char *week_str[] = {"일", "월", "화", "수", "목", "금", "토"};

      rtcGetTime(&rtc_time);
      rtcGetDate(&rtc_date);

      lcdPrintf(x_offset, 0, white,
                "%02d-%02d-%02d (%s)",
                rtc_date.year, rtc_date.month, rtc_date.day, week_str[rtc_date.week]);

      lcdPrintf(x_offset, 16, white,
                "%02d:%02d:%02d",
                rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
    }

    if (menu == 2)
    {
      imu_info_t imu_info;

      imuUpdate();

      imuGetInfo(&imu_info);

      lcdPrintf(x_offset,  0, white,
                "R %-4d P %-4d", 
                (int)imu_info.roll, 
                (int)imu_info.pitch);
      lcdPrintf(x_offset, 16, white,
                "Y %-4d", 
                (int)imu_info.yaw);
    }     

    if (menu == 3)
    {
      static hdc1080_info_t hdc_info;

      hdc1080GetInfo(&hdc_info);

      lcdPrintf(x_offset, 0, white,
                "온도 : %-3d", (int)hdc_info.temp);
      lcdPrintf(x_offset, 16, white, 
                "습도 : %-3d%%", (int)hdc_info.humidity);
    }     

    if (menu == 4)
    {
      uint16_t adc_data;
      uint16_t adc_vol;

      adc_data = adcRead12(LIGHT_ADC);
      adc_vol = (uint16_t)(adcReadVoltage(LIGHT_ADC) * 1000);

      lcdPrintf(x_offset, 0, white,
                "ADC  : %04d", adc_data);
      lcdPrintf(x_offset, 16, white, 
                "전압 : %-4d mV", adc_vol);
    }

    lcdRequestDraw();
  }
}