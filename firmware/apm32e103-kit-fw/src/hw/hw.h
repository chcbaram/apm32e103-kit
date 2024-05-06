#ifndef HW_H_
#define HW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"

#include "led.h"
#include "uart.h"
#include "log.h"
#include "cli.h"
#include "cli_gui.h"
#include "button.h"
#include "gpio.h"
#include "i2c.h"
#include "eeprom.h"
#include "spi.h"
#include "spi_flash.h"
#include "rtc.h"
#include "sd.h"
#include "fatfs.h"
#include "files.h"
#include "i2s.h"
#include "usb.h"
#include "cdc.h"
#include "can.h"
#include "ws2812.h"
#include "lcd.h"
#include "swtimer.h"
#include "event.h"
#include "wiznet/wiznet.h"
#include "imu.h"
#include "hdc1080.h"
#include "adc.h"
#include "flash.h"
#include "reset.h"
#include "cmd.h"
#include "util.h"
#include "qbuffer.h"


bool hwInit(void);


#ifdef __cplusplus
}
#endif

#endif