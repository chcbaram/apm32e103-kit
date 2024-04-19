
#include "usbd_conf.h"
#include "usbd_core.h"


USBD_HANDLE_T usbDeviceHandler;
static bool   is_connected = false;

/*!
 * @brief       Init USB hardware
 *
 * @param       usbInfo:
 *
 * @retval      None
 */
void USBD_HardwareInit(USBD_INFO_T *usbInfo)
{
  GPIO_Config_T gpioConfig;

  /* Configure USB */
  RCM_ConfigUSBCLK(RCM_USB_DIV_2_5); // 120Mhz / 2.5 = 48Mhz
  RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USB);

  /* Configure USB GPIO */
  RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);

  /* Link structure */
  usbDeviceHandler.usbGlobal = USBD;

  /* Link data */
  usbDeviceHandler.dataPoint = usbInfo;
  usbInfo->dataPoint         = &usbDeviceHandler;

  usbDeviceHandler.usbCfg.sofStatus      = DISABLE;
  usbDeviceHandler.usbCfg.speed          = USB_SPEED_FSLS;
  usbDeviceHandler.usbCfg.devEndpointNum = 8;
  usbDeviceHandler.usbCfg.lowPowerStatus = DISABLE;
  usbDeviceHandler.usbCfg.lpmStatus      = DISABLE;
  usbDeviceHandler.usbCfg.batteryStatus  = DISABLE;

  /* USB DM, DP pin configuration */
  gpioConfig.mode  = GPIO_MODE_AF_PP;
  gpioConfig.speed = GPIO_SPEED_50MHz;
  gpioConfig.pin   = GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_Config(GPIOA, &gpioConfig);
  
  /* NVIC */
  NVIC_ConfigPriorityGroup(NVIC_PRIORITY_GROUP_4);

#if USB_SELECT == USB1
  NVIC_EnableIRQRequest(USBD1_LP_CAN1_RX0_IRQn, 1, 0);
#else
  NVIC_EnableIRQRequest(USBD2_LP_CAN2_RX0_IRQn, 1, 0);
#endif


#if USB_SELECT == USB1
  USBD2_Disable(usbDeviceHandler.usbGlobal);
#else
  USBD2_Enable(usbDeviceHandler.usbGlobal);
#endif /* USB_SELECT */

  /* Disable USB all global interrupt */
  USBD_DisableInterrupt(usbDeviceHandler.usbGlobal,
                        USBD_INT_CTR | USBD_INT_WKUP | USBD_INT_SUS | USBD_INT_ERR | USBD_INT_RST | USBD_INT_SOF | USBD_INT_ESOF);

  /* Init USB Core */
  USBD_Config(&usbDeviceHandler);

  USBD_ConfigPMA(&usbDeviceHandler, USBD_EP0_OUT_ADDR, USBD_EP_BUFFER_SINGLE, USBD_EP0_OUT_PMA_SIZE);
  USBD_ConfigPMA(&usbDeviceHandler, USBD_EP0_IN_ADDR, USBD_EP_BUFFER_SINGLE, USBD_EP0_IN_PMA_SIZE);

  USBD_ConfigPMA(&usbDeviceHandler, USBD_CDC_EP_OUT_ADDR, USBD_EP_BUFFER_SINGLE, USBD_CDC_EP_OUT_PMA_SIZE);
  USBD_ConfigPMA(&usbDeviceHandler, USBD_CDC_EP_IN_ADDR, USBD_EP_BUFFER_SINGLE, USBD_CDC_EP_IN_PMA_SIZE);
  USBD_ConfigPMA(&usbDeviceHandler, USBD_CDC_EP_CMD_ADDR, USBD_EP_BUFFER_SINGLE, USBD_CDC_EP_CMD_PMA_SIZE);

  USBD_StartCallback(usbInfo);
}

/*!
 * @brief       Reset USB hardware
 *
 * @param       usbInfo:usb handler information
 *
 * @retval      None
 */
void USBD_HardwareReset(USBD_INFO_T *usbInfo)
{
  if (usbInfo->devSpeed == USBD_SPEED_FS)
  {
    RCM_DisableAPB1PeriphClock(RCM_APB1_PERIPH_USB);

#if USB_SELECT == USB1
    NVIC_DisableIRQRequest(USBD1_LP_CAN1_RX0_IRQn);
#else
    NVIC_DisableIRQRequest(USBD2_LP_CAN2_RX0_IRQn);
#endif
  }
}

/*!
 * @brief       USB device start event callback function
 *
 * @param       usbInfo
 *
 * @retval      None
 */
void USBD_StartCallback(USBD_INFO_T *usbInfo)
{
  USBD_Start(usbInfo->dataPoint);
}

/*!
 * @brief     USB device stop handler callback
 *
 * @param     usbInfo : usb handler information
 *
 * @retval    None
 */
void USBD_StopCallback(USBD_INFO_T *usbInfo)
{
  USBD_Stop(usbInfo->dataPoint);
}

/*!
 * @brief     USB device stop device mode handler callback
 *
 * @param     usbInfo : usb handler information
 *
 * @retval    None
 */
void USBD_StopDeviceCallback(USBD_INFO_T *usbInfo)
{
  USBD_StopDevice(usbInfo->dataPoint);
}

/*!
 * @brief     USB OTG device resume callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_ResumeCallback(USBD_HANDLE_T *usbdh)
{
  USBD_Resume(usbdh->dataPoint);
}

/*!
 * @brief     USB OTG device suspend callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_SuspendCallback(USBD_HANDLE_T *usbdh)
{
  USBD_Suspend(usbdh->dataPoint);

  if (usbdh->usbCfg.lowPowerStatus == ENABLE)
  {
    /* Set SLEEPDEEP bit and SLEEPONEXIT SCR */
    SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
  }

  is_connected = false;
}

/*!
 * @brief     USB OTG device enum done callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_EnumDoneCallback(USBD_HANDLE_T *usbdh)
{
  USBD_DEVICE_SPEED_T speed = USBD_DEVICE_SPEED_FS;

  switch (usbdh->usbCfg.speed)
  {
    case USB_SPEED_FSLS:
      speed = USBD_DEVICE_SPEED_FS;
      break;

    default:
      /* Speed error status */
      break;
  }

  /* Set USB core speed */
  USBD_SetSpeed(usbdh->dataPoint, speed);

  /* Reset device */
  USBD_Reset(usbdh->dataPoint);
}

/*!
 * @brief     USB OTG device SETUP stage callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_SetupStageCallback(USBD_HANDLE_T *usbdh)
{
  USBD_SetupStage(usbdh->dataPoint, (uint8_t *)usbdh->setup);
}

/*!
 * @brief     USB OTG device data OUT stage callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
void USBD_DataOutStageCallback(USBD_HANDLE_T *usbdh, uint8_t epNum)
{
  USBD_DataOutStage(usbdh->dataPoint, epNum, usbdh->epOUT[epNum].buffer);
}

/*!
 * @brief     USB OTG device data IN stage callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
void USBD_DataInStageCallback(USBD_HANDLE_T *usbdh, uint8_t epNum)
{
  USBD_DataInStage(usbdh->dataPoint, epNum, usbdh->epIN[epNum].buffer);
}

/*!
 * @brief     USB device set EP on stall status callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr: endpoint address
 *
 * @retval    None
 */
USBD_STA_T USBD_EP_StallCallback(USBD_INFO_T *usbInfo, uint8_t epAddr)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_EP_Stall(usbInfo->dataPoint, epAddr);

  return usbStatus;
}

/*!
 * @brief     USB device clear EP stall status callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr: endpoint address
 *
 * @retval    None
 */
USBD_STA_T USBD_EP_ClearStallCallback(USBD_INFO_T *usbInfo, uint8_t epAddr)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_EP_ClearStall(usbInfo->dataPoint, epAddr);

  return usbStatus;
}

/*!
 * @brief     USB device read EP stall status callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr: endpoint address
 *
 * @retval    Stall status
 */
uint8_t USBD_EP_ReadStallStatusCallback(USBD_INFO_T *usbInfo, uint8_t epAddr)
{
  return (USBD_EP_ReadStallStatus(usbInfo->dataPoint, epAddr));
}

/*!
 * @brief     USB device read EP last receive data size callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr: endpoint address
 *
 * @retval    size of last receive data
 */
uint32_t USBD_EP_ReadRxDataLenCallback(USBD_INFO_T *usbInfo, uint8_t epAddr)
{
  return USBD_EP_ReadRxDataLen(usbInfo->dataPoint, epAddr);
}

/*!
 * @brief     USB device open EP callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr: endpoint address
 *
 * @param     epType: endpoint type
 *
 * @param     epMps: endpoint maxinum of packet size
 *
 * @retval    None
 */
void USBD_EP_OpenCallback(USBD_INFO_T *usbInfo, uint8_t epAddr,
                          uint8_t epType, uint16_t epMps)
{
  USBD_EP_Open(usbInfo->dataPoint, epAddr, epType, epMps);
}

/*!
 * @brief     USB device close EP callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr: endpoint address
 *
 * @retval    None
 */
void USBD_EP_CloseCallback(USBD_INFO_T *usbInfo, uint8_t epAddr)
{
  USBD_EP_Close(usbInfo->dataPoint, epAddr);
}

/*!
 * @brief     USB device EP receive handler callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr : endpoint address
 *
 * @param     buffer : data buffer
 *
 * @param     length : length of data
 *
 * @retval    usb device status
 */
USBD_STA_T USBD_EP_ReceiveCallback(USBD_INFO_T *usbInfo, uint8_t epAddr,
                                   uint8_t *buffer, uint32_t length)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_EP_Receive(usbInfo->dataPoint, epAddr, buffer, length);

  return usbStatus;
}

/*!
 * @brief     USB device EP transfer handler callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr : endpoint address
 *
 * @param     buffer : data buffer
 *
 * @param     length : length of data
 *
 * @retval    usb device status
 */
USBD_STA_T USBD_EP_TransferCallback(USBD_INFO_T *usbInfo, uint8_t epAddr,
                                    uint8_t *buffer, uint32_t length)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_EP_Transfer(usbInfo->dataPoint, epAddr, buffer, length);

  return usbStatus;
}

/*!
 * @brief     USB device flush EP handler callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     epAddr : endpoint address
 *
 * @retval    usb device status
 */
USBD_STA_T USBD_EP_FlushCallback(USBD_INFO_T *usbInfo, uint8_t epAddr)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_EP_Flush(usbInfo->dataPoint, epAddr);

  return usbStatus;
}

/*!
 * @brief     USB device set device address handler callback
 *
 * @param     usbInfo : usb handler information
 *
 * @param     address : address
 *
 * @retval    usb device status
 */
USBD_STA_T USBD_SetDevAddressCallback(USBD_INFO_T *usbInfo, uint8_t address)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_SetDevAddress(usbInfo->dataPoint, address);

  is_connected = true;

  return usbStatus;
}

/*!
 * @brief       USB OTG device SOF event callback function
 *
 * @param       usbhh: USB host handler.
 *
 * @retval      None
 */
void USBD_SOFCallback(USBD_HANDLE_T *usbdh)
{
  USBD_HandleSOF(usbdh->dataPoint);
}

/*!
 * @brief     USB OTG device ISO IN in complete callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
void USBD_IsoInInCompleteCallback(USBD_HANDLE_T *usbdh, uint8_t epNum)
{
  USBD_IsoInInComplete(usbdh->dataPoint, epNum);
}

/*!
 * @brief     USB OTG device ISO OUT in complete callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
void USBD_IsoOutInCompleteCallback(USBD_HANDLE_T *usbdh, uint8_t epNum)
{
  USBD_IsoOutInComplete(usbdh->dataPoint, epNum);
}

/*!
 * @brief     USB OTG device connect callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_ConnectCallback(USBD_HANDLE_T *usbdh)
{
  USBD_Connect(usbdh->dataPoint);
}

/*!
 * @brief     USB OTG device disconnect callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_DisconnectCallback(USBD_HANDLE_T *usbdh)
{
  USBD_Disconnect(usbdh->dataPoint);
}

bool USBD_is_connected(void)
{
  return is_connected;
}