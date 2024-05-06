#include "ap.h"
#include "modules.h"



void updateLED(void);
void updateSD(void);
void updateWiznet(void);
void updateLCD(void);
void updateCMD(void);


static bool is_run_fw = true;
static bool is_update_fw = false;




void apInit(void)
{
  uint32_t boot_param;
  uint16_t err_code;

  boot_param = resetGetBootMode();


  if (boot_param & (1<<MODE_BIT_BOOT))
  {
    boot_param &= ~(1<<MODE_BIT_BOOT);
    resetSetBootMode(boot_param);    
    is_run_fw = false;
  }

  if (buttonGetPressed(HW_BUTTON_CH_BOOT) == true)
  {
    if (lcdIsInit())
    {
      lcdClearBuffer(black);
      lcdPrintfResize(0, 8, green, 16, "      BOOT   ");
      lcdDrawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, white);
      lcdUpdateDraw();
      delay(500);
    }
    is_run_fw = false;
  }

  if (boot_param & (1<<MODE_BIT_UPDATE))
  {
    boot_param &= ~(1<<MODE_BIT_UPDATE);
    resetSetBootMode(boot_param);
    
    is_run_fw = true;
    is_update_fw = true;
  }
  logPrintf("\n");

  if (is_update_fw)
  {
    if (lcdIsInit())
    {
      lcdClearBuffer(black);
      lcdPrintf(0, 8, white, " Update Firm..");
      lcdUpdateDraw();
    }

    logPrintf("[  ] bootUpdateFirm()\n");
    err_code = bootUpdateFirm();
    if (err_code == OK)
      logPrintf("[OK]\n");
    else
      logPrintf("[E_] err : 0x%04X\n", err_code);    
  }

  if (is_run_fw)
  {
    if (lcdIsInit())
    {
      lcdClearBuffer(black);
      lcdPrintf(0, 8, white, " Jump Firm...");
      lcdUpdateDraw();
    }    
  
    err_code = bootJumpFirm();
    if (err_code != OK)
    {
      logPrintf("[  ] bootJumpFirm()\n");
      logPrintf("[E_] err : 0x%04X\n", err_code);
      if (bootVerifyUpdate() == OK)
      {
        logPrintf("[  ] retry update\n");
        if (bootUpdateFirm() == OK)
        {
          err_code = bootJumpFirm();
          if (err_code != OK)
            logPrintf("[E_] err : 0x%04X\n", err_code);
        }
      }      
    }
  }

  logPrintf("\n");
  logPrintf("Boot Mode..\n"); 

  #ifdef _USE_HW_CLI
  cliOpen(HW_UART_CH_CLI, 115200);
  cliLogo();
  #endif
}


void apMain(void)
{
  cmdTaskInit();


  while(1)
  {
    #ifdef _USE_HW_CLI
    cliMain();
    #endif

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
  
  
  if (millis() - pre_time >= 100)
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
  int16_t         x_offset = 10;
  static uint8_t  menu     = 0;
  uint8_t         menu_max = 1;
  uint8_t         menu_cur = 0;
  cmd_boot_info_t cmd_boot_info;


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

  if (cmdBootIsBusy())
  {
    menu_cur = menu_max;
  }
  else
  {
    menu_cur = menu;
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


    if (menu_cur == 0)
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
        lcdPrintf(x_offset, 0, white, "BOOT");        
        lcdPrintf(x_offset, 16, white, "Not Connected");        
      }
    }

    if (cmdBootIsBusy())
    {
      uint16_t percent;

      cmdBootGetInfo(&cmd_boot_info);

      percent = cmd_boot_info.fw_receive_size * 100 / cmd_boot_info.fw_size;
      lcdClearBuffer(black);
      lcdPrintf(96, 0, white, "%3d%%", percent);
      lcdDrawRect(0, 16, 128, 16, white);
      lcdDrawFillRect(2, 19, percent * 124 / 100, 10, white);      
    }

    lcdRequestDraw();
  }
}