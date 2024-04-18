#ifndef BSP_H_
#define BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "def.h"


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



void logPrintf(const char *fmt, ...);


bool bspInit(void);

void delay(uint32_t time_ms);
uint32_t millis(void);


#ifdef __cplusplus
}
#endif

#endif