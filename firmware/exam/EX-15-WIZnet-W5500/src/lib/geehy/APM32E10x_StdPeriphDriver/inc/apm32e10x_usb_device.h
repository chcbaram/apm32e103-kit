/*!
 * @file        apm32f10x_usb_device.h
 *
 * @brief       USB device function handle
 *
 * @version     V1.0.1
 *
 * @date        2023-03-27
 *
 * @attention
 *
 *  Copyright (C) 2023 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

/* Define to prevent recursive inclusion */
#ifndef __APM32F10X_USB_DEVICE_H
#define __APM32F10X_USB_DEVICE_H

/* Includes */
#include "apm32e10x.h"
#include "apm32e10x_rcm.h"
#include "apm32e10x_usb.h"

/** @addtogroup APM32F10x_StdPeriphDriver
  @{
*/

/** @addtogroup USB_Device_Driver USB Device Driver
  @{
*/

/** @defgroup USB_Device_Macros Macros
  @{
*/

#if defined (USB_DEVICE)

#define USBD_WAKEUP_EINT_LINE                   0x40000

#endif /* defined (USB_DEVICE) */

/**
 * @brief USB device handle
 */
typedef struct
{

#if defined (USB_DEVICE)
    USBD_T*                     usbGlobal;
    USBD_CFG_T                  usbCfg;
    
    USBD_ENDPOINT_INFO_T        epIN[8];
    USBD_ENDPOINT_INFO_T        epOUT[8];
#endif /* defined (USB_DEVICE) */
    uint32_t                    setup[12];
    __IO uint8_t                address;

    void*                       dataPoint;
} USBD_HANDLE_T;

#if defined (USB_DEVICE)
void USBD_IsrHandler(USBD_HANDLE_T* usbdh);
void USBD_WakeupIsrHandler(USBD_HANDLE_T* usbdh);
void USBD_ConfigPMA(USBD_HANDLE_T* usbdh, uint16_t epAddr, uint16_t bufferStatus, uint32_t pmaAddr);
#endif /* defined (USB_DEVICE) */

#define USB_FS_WAKEUP_EINT_LINE             EINT_LINE_18
#define USBD_FS_WAKEUP_EINT_ENABLE_IT()     EINT->IMASK |= USB_FS_WAKEUP_EINT_LINE
#define USBD_FS_WAKEUP_EINT_DISABLE_IT()    EINT->IMASK &= ~(USB_FS_WAKEUP_EINT_LINE)
#define USBD_FS_WAKEUP_EINT_GET_FLAG()      EINT->IPEND & (USB_FS_WAKEUP_EINT_LINE)
#define USBD_FS_WAKEUP_EINT_CLEAR_FLAG()    EINT->IPEND = USB_FS_WAKEUP_EINT_LINE

#define USBD_FS_WAKEUP_EINT_ENABLE_RISING_EDGE() \
                        do { \
                             EINT->FTEN &= ~(USB_FS_WAKEUP_EINT_LINE); \
                             EINT->RTEN |= USB_FS_WAKEUP_EINT_LINE; \
                           } while(0U)

void USBD_Start(USBD_HANDLE_T* usbdh);
void USBD_Stop(USBD_HANDLE_T* usbdh);
void USBD_StopDevice(USBD_HANDLE_T* usbdh);
void USBD_Config(USBD_HANDLE_T* usbdh);
void USBD_SetDevAddress(USBD_HANDLE_T* usbdh, uint8_t address);
void USBD_EP_Open(USBD_HANDLE_T* usbdh, uint8_t epAddr, \
                  uint8_t epType, uint16_t epMps);
void USBD_EP_Close(USBD_HANDLE_T* usbdh, uint8_t epAddr);
void USBD_EP_Stall(USBD_HANDLE_T* usbdh, uint8_t epAddr);
void USBD_EP_ClearStall(USBD_HANDLE_T* usbdh, uint8_t epAddr);
uint8_t USBD_EP_ReadStallStatus(USBD_HANDLE_T* usbdh, uint8_t epAddr);
uint32_t USBD_EP_ReadRxDataLen(USBD_HANDLE_T* usbdh, uint8_t epAddr);
void USBD_EP_Flush(USBD_HANDLE_T* usbdh, uint8_t epAddr);
void USBD_EP_Receive(USBD_HANDLE_T* usbdh, uint8_t epAddr, \
                     uint8_t* buffer, uint32_t length);

void USBD_EP_Transfer(USBD_HANDLE_T* usbdh, uint8_t epAddr, \
                      uint8_t* buffer, uint32_t length);

void USBD_DisconnectCallback(USBD_HANDLE_T* usbdh);
void USBD_ConnectCallback(USBD_HANDLE_T* usbdh);
void USBD_ResumeCallback(USBD_HANDLE_T* usbdh);
void USBD_SuspendCallback(USBD_HANDLE_T* usbdh);
void USBD_EnumDoneCallback(USBD_HANDLE_T* usbdh);
void USBD_SetupStageCallback(USBD_HANDLE_T* usbdh);
void USBD_DataInStageCallback(USBD_HANDLE_T* usbdh, uint8_t epNum);
void USBD_DataOutStageCallback(USBD_HANDLE_T* usbdh, uint8_t epNum);
void USBD_SOFCallback(USBD_HANDLE_T* usbdh);
void USBD_IsoInInCompleteCallback(USBD_HANDLE_T* usbdh, uint8_t epNum);
void USBD_IsoOutInCompleteCallback(USBD_HANDLE_T* usbdh, uint8_t epNum);

/**@} end of group USB_Device_Functions*/
/**@} end of group USB_Device_Driver*/
/**@} end of group APM32F10x_StdPeriphDriver*/

#endif
