#include "hw.h"




extern uint32_t _fw_flash_begin;
extern uint32_t _fw_size;

volatile const firm_ver_t firm_ver __attribute__((section(".version"))) = 
{
  .magic_number = VERSION_MAGIC_NUMBER,
  .version_str  = _DEF_FIRMWATRE_VERSION,
  .name_str     = _DEF_BOARD_NAME,
  .firm_addr    = (uint32_t)&_fw_flash_begin
};




bool hwInit(void)
{
  bspInit();

  cliInit();
  logInit();
  swtimerInit();
  ledInit();
  uartInit();
  for (int i=0; i<HW_UART_MAX_CH; i++)
  {
    uartOpen(i, 115200);
  }

  logOpen(HW_LOG_CH, 115200);
  logPrintf("\r\n[ Firmware Begin... ]\r\n");
  logPrintf("Booting..Name \t\t: %s\r\n", _DEF_BOARD_NAME);
  logPrintf("Booting..Ver  \t\t: %s\r\n", _DEF_FIRMWATRE_VERSION);  
  logPrintf("Booting..SYS  \t\t: %d Mhz\r\n", (int)RCM_ReadSYSCLKFreq()/1000000);
  logPrintf("Booting..HCLK \t\t: %d Mhz\r\n", (int)RCM_ReadHCLKFreq()/1000000);

  uint32_t pclk[2];
  RCM_ReadPCLKFreq(&pclk[0], &pclk[1]);
  logPrintf("Booting..PCLK1 \t\t: %d Mhz\r\n", (int)pclk[0]/1000000);
  logPrintf("Booting..PCLK2 \t\t: %d Mhz\r\n", (int)pclk[1]/1000000);

  logPrintf("\n");

  rtcInit();
  gpioInit();
  buttonInit();
  i2cInit();
  eepromInit();
  spiInit();
  spiFlashInit();
  sdInit();
  fatfsInit();
  i2sInit();
  

  usbInit();
  usbBegin(USB_CDC_MODE);
  cdcInit();

  canInit();
  ws2812Init();
  lcdInit();
  lcdSetFps(20);
  
  eventInit();
  wiznetInit();
  wiznetDHCP();
  wiznetSNTP();

  imuInit();
  hdc1080Init();

  adcInit();
  return true;
}