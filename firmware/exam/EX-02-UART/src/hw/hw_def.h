#ifndef HW_DEF_H_
#define HW_DEF_H_



#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION    "V240416R1"
#define _DEF_BOARD_NAME           "APM32E103-KIT-FW"



#define _USE_HW_LED
#define      HW_LED_MAX_CH          1

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1
#define      HW_UART_CH_SWD         _DEF_UART1



//-- USE CLI
//
#define _USE_CLI_HW_LED             1


#endif