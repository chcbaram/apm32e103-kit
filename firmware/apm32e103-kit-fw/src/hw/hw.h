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


bool hwInit(void);


#ifdef __cplusplus
}
#endif

#endif