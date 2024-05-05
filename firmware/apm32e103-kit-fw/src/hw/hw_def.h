#ifndef HW_DEF_H_
#define HW_DEF_H_



#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION    "V240503R1"
#define _DEF_BOARD_NAME           "APM32E103-KIT-FW"


#define _USE_HW_GPIO
#define _USE_HW_SD
#define _USE_HW_FATFS
#define _USE_HW_FILES
#define _USE_HW_BUZZER
#define _USE_HW_ICM42670
#define _USE_HW_IMU
#define _USE_HW_HDC1080 
#define _USE_HW_FLASH


#define _USE_HW_LED
#define      HW_LED_MAX_CH          1
#define      HW_LED_CH_DOWN         _DEF_LED1
#define      HW_LED_CH_UPDATE       _DEF_LED1

#define _USE_HW_UART
#define      HW_UART_MAX_CH         3
#define      HW_UART_CH_SWD         _DEF_UART1
#define      HW_UART_CH_CLI         _DEF_UART1
#define      HW_UART_CH_USB         _DEF_UART2
#define      HW_UART_CH_EXT         _DEF_UART3

#define _USE_HW_LOG
#define      HW_LOG_CH              _DEF_UART1
#define      HW_LOG_BOOT_BUF_MAX    2048
#define      HW_LOG_LIST_BUF_MAX    4096

#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    32
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    8
#define      HW_CLI_LINE_BUF_MAX    64

#define _USE_HW_BUTTON
#define      HW_BUTTON_MAX_CH       1

#define _USE_HW_I2C
#define      HW_I2C_MAX_CH          2
#define      HW_I2C_CH_EEPROM       _DEF_I2C1
#define      HW_I2C_CH_OLED         _DEF_I2C2
#define      HW_I2C_CH_IMU          _DEF_I2C1

#define _USE_HW_EEPROM
#define      HW_EEPROM_MAX_SIZE     (8*1024)

#define _USE_HW_SPI
#define      HW_SPI_MAX_CH          2

#define _USE_HW_SPI_FLASH
#define      HW_SPI_FLASH_ADDR      0x91000000

#define _USE_HW_I2S
#define      HW_I2S_LCD             0

#define _USE_HW_MIXER
#define      HW_MIXER_MAX_CH        4
#define      HW_MIXER_MAX_BUF_LEN   (48*2*4*4) // 48Khz * Stereo * 4ms * 4

#define _USE_HW_USB
#define _USE_HW_CDC
#define      HW_USE_CDC             1

#define _USE_HW_CAN
#define      HW_CAN_FD              0
#define      HW_CAN_MAX_CH          1
#define      HW_CAN_MSG_RX_BUF_MAX  32

#define _USE_HW_WS2812
#define     HW_WS2812_MAX_CH        1

#define _USE_HW_LCD
#define _USE_HW_SSD1306
#define      HW_LCD_WIDTH           128
#define      HW_LCD_HEIGHT          32

#define _USE_HW_SWTIMER
#define      HW_SWTIMER_MAX_CH      8

#define _USE_HW_WIZNET
#define      HW_WIZNET_SOCKET_CMD   0
#define      HW_WIZNET_SOCKET_DHCP  1
#define      HW_WIZNET_SOCKET_SNTP  2

#define _USE_HW_EVENT
#define      HW_EVENT_Q_MAX         8
#define      HW_EVENT_NODE_MAX      16  

#define _USE_HW_ADC                 
#define      HW_ADC_MAX_CH          1

#define _USE_HW_RTC
#define      HW_RTC_BOOT_MODE       BAKPR_DATA3
#define      HW_RTC_RESET_BITS      BAKPR_DATA4

#define _USE_HW_RESET
#define      HW_RESET_BOOT          1

#define _USE_HW_CMD
#define      HW_CMD_MAX_DATA_LENGTH 1024



#define FLASH_SIZE_TAG              0x400
#define FLASH_SIZE_VEC              0x400
#define FLASH_SIZE_VER              0x400
#define FLASH_SIZE_FIRM             (384*1024)

#define FLASH_ADDR_BOOT             0x08000000
#define FLASH_ADDR_FIRM             0x08020000
#define FLASH_ADDR_UPDATE           0x91800000


//-- USE CLI
//
#define _USE_CLI_HW_LED             1
#define _USE_CLI_HW_BUTTON          1
#define _USE_CLI_HW_GPIO            1
#define _USE_CLI_HW_I2C             1
#define _USE_CLI_HW_EEPROM          1
#define _USE_CLI_HW_SPI_FLASH       1
#define _USE_CLI_HW_RTC             1
#define _USE_CLI_HW_SD              1
#define _USE_CLI_HW_FATFS           1
#define _USE_CLI_HW_I2S             1
#define _USE_CLI_HW_USB             1
#define _USE_CLI_HW_CAN             1
#define _USE_CLI_HW_WS2812          1
#define _USE_CLI_HW_EVENT           1
#define _USE_CLI_HW_WIZNET          1
#define _USE_CLI_HW_ICM42670        1
#define _USE_CLI_HW_IMU             1
#define _USE_CLI_HW_HDC1080         1
#define _USE_CLI_HW_ADC             1
#define _USE_CLI_HW_FLASH           1
#define _USE_CLI_HW_RESET           1


typedef enum
{
  SD_DETECT,
  SPI_CS,
  I2S_MUTE,
  SPI2_CS,
  W5500_RST,
  W5500_INT,
  GPIO_PIN_MAX,  
} GpioPinName_t;

typedef enum
{
  LIGHT_ADC = 0,
  ADC_PIN_MAX
} AdcPinName_t;

#endif