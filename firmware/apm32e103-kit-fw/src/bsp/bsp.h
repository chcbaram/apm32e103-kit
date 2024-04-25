#ifndef BSP_H_
#define BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "def.h"


#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /* ARM Compiler V6 */
  #ifndef __weak
    #define __weak  __attribute__((weak))
  #endif
  #ifndef __packed
    #define __packed  __attribute__((packed))
  #endif
#elif defined ( __GNUC__ ) && !defined (__CC_ARM) /* GNU Compiler */
  #ifndef __weak
    #define __weak   __attribute__((weak))
  #endif /* __weak */
  #ifndef __packed
    #define __packed __attribute__((__packed__))
  #endif /* __packed */
#endif /* __GNUC__ */



#include "apm32e10x.h"
#include "apm32e10x_gpio.h"
#include "apm32e10x_rcm.h"
#include "apm32e10x_usart.h"
#include "apm32e10x_dma.h"
#include "apm32e10x_i2c.h"
#include "apm32e10x_misc.h"
#include "apm32e10x_spi.h"
#include "apm32e10x_bakpr.h"
#include "apm32e10x_pmu.h"
#include "apm32e10x_rtc.h"
#include "apm32e10x_sdio.h"
#include "apm32e10x_usb.h"
#include "apm32e10x_usb_device.h"
#include "apm32e10x_can.h"
#include "apm32e10x_tmr.h"


void logPrintf(const char *fmt, ...);


bool bspInit(void);

void delay(uint32_t time_ms);
uint32_t millis(void);
uint32_t micros(void);


#ifdef __cplusplus
}
#endif

#endif