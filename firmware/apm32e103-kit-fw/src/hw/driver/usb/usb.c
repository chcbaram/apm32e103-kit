/*
 * usb.c
 *
 *  Created on: 2018. 3. 16.
 *      Author: HanCheol Cho
 */


#include "usb.h"



#ifdef _USE_HW_USB
#include "cdc.h"
#include "cli.h"

static bool is_init = false;
static UsbMode_t is_usb_mode = USB_NON_MODE;

USBD_INFO_T gUsbDeviceFS;
extern USBD_HANDLE_T usbDeviceHandler;


#if HW_USE_MSC == 1
extern USBD_StorageTypeDef USBD_DISK_fops;
#endif

#if CLI_USE(HW_USB)
static void cliCmd(cli_args_t *args);
#endif
static void usbDevHandler(USBD_INFO_T *usbInfo, uint8_t userStatus);




bool usbInit(void)
{
#if CLI_USE(HW_USB)
  cliAdd("usb", cliCmd);
#endif
  return true;
}

bool usbBegin(UsbMode_t usb_mode)
{
  is_init = true;

  if (usb_mode == USB_CDC_MODE)
  {
    /* USB CDC register interface handler */
    USBD_CDC_RegisterItf(&gUsbDeviceFS, &USBD_CDC_INTERFACE_FS);
    
    /* USB device and class init */
    USBD_Init(&gUsbDeviceFS, USBD_SPEED_FS, &USBD_DESC_FS, &USBD_CDC_CLASS, usbDevHandler);


    is_usb_mode = USB_CDC_MODE;
    
    logPrintf("[OK] usbBegin()\n");
    logPrintf("     USB_CDC\r\n");
  }
  else if (usb_mode == USB_MSC_MODE)
  {
    #if HW_USE_MSC == 1
    logPrintf("[OK] usbBegin()\n");
    logPrintf("     USB_MSC\r\n");
    #endif
  }
  else
  {
    is_init = false;

    logPrintf("[NG] usbBegin()\n");
  }

  return is_init;
}

void usbDeInit(void)
{
  if (is_init == true)
  {
    USBD_DeInit(&gUsbDeviceFS);
  }
}

bool usbIsOpen(void)
{
  return cdcIsConnect();
}

bool usbIsConnect(void)
{
  if (gUsbDeviceFS.devClass == NULL)
  {
    return false;
  }
  if (gUsbDeviceFS.devState != USBD_DEV_CONFIGURE)
  {
    return false;
  }
  if (USBD_is_connected() == false)
  {
    return false;
  }
  
  return true;
}

UsbMode_t usbGetMode(void)
{
  return is_usb_mode;
}

UsbType_t usbGetType(void)
{
  return (UsbType_t)cdcGetType();
}

void usbDevHandler(USBD_INFO_T *usbInfo, uint8_t userStatus)
{
  switch (userStatus)
  {
    case USBD_USER_RESET:
      // logPrintf("USBD_USER_RESET\n");
      break;

    case USBD_USER_RESUME:
      // logPrintf("USBD_USER_RESUME\n");
      break;

    case USBD_USER_SUSPEND:
      // logPrintf("USBD_USER_SUSPEND\n");
      break;

    case USBD_USER_CONNECT:
      // logPrintf("USBD_USER_CONNECT\n");
      break;

    case USBD_USER_DISCONNECT:
      // logPrintf("USBD_USER_DISCONNECT\n");
      break;

    case USBD_USER_ERROR:
      // logPrintf("USBD_USER_ERROR\n");
      break;

    default:
      break;
  }
}


#if defined (USB_DEVICE)
#if USB_SELECT == USB1
void USBD1_LP_CAN1_RX0_IRQHandler(void)
#else
void USBD2_LP_CAN2_RX0_IRQHandler(void)
#endif 
{
    USBD_IsrHandler(&usbDeviceHandler);
}

#if USB_SELECT == USB1
void USBD1_HP_CAN1_TX_IRQHandler(void)
#else
void USBD2_HP_CAN2_TX_IRQHandler(void)
#endif 
{
    
}
#endif 



#if CLI_USE(HW_USB)
void cliCmd(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    while(cliKeepLoop())
    {
      cliPrintf("USB Mode    : %d\n", usbGetMode());
      cliPrintf("USB Type    : %d\n", usbGetType());
      cliPrintf("USB Connect : %d\n", usbIsConnect());
      cliPrintf("USB Open    : %d\n", usbIsOpen());
      cliPrintf("\x1B[%dA", 4);
      delay(100);
    }
    cliPrintf("\x1B[%dB", 4);

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "tx") == true)
  {
    uint32_t pre_time;
    uint32_t tx_cnt = 0;
    uint32_t sent_len = 0;

    pre_time = millis();
    while(cliKeepLoop())
    {
      if (millis()-pre_time >= 1000)
      {
        pre_time = millis();
        logPrintf("tx : %d KB/s\n", tx_cnt/1024);
        tx_cnt = 0;
      }
      sent_len = cdcWrite((uint8_t *)"123456789012345678901234567890\n", 31);
      tx_cnt += sent_len;
    }
    cliPrintf("\x1B[%dB", 2);

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "rx") == true)
  {
    uint32_t pre_time;
    uint32_t rx_cnt = 0;
    uint32_t rx_len;

    pre_time = millis();
    while(cliKeepLoop())
    {
      if (millis()-pre_time >= 1000)
      {
        pre_time = millis();
        logPrintf("rx : %d KB/s\n", rx_cnt/1024);
        rx_cnt = 0;
      }

      rx_len = cdcAvailable();

      for (int i=0; i<rx_len; i++)
      {
        cdcRead();
      }

      rx_cnt += rx_len;
    }
    cliPrintf("\x1B[%dB", 2);

    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("usb info\n");
    cliPrintf("usb tx\n");
    cliPrintf("usb rx\n");
  }
}
#endif

#endif