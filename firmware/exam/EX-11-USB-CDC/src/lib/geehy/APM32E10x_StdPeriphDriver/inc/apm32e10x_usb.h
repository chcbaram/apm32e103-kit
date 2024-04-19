/*!
 * @file        apm32f10x_usb.h
 *
 * @brief       This file provides all the USB driver functions
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
#ifndef __APM32F10X_USB_H
#define __APM32F10X_USB_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "apm32e10x.h"

  /** @addtogroup APM32F10x_StdPeriphDriver
    @{
  */

  /** @addtogroup USB_Driver USB Driver
    @{
  */

  /** @defgroup USB_Macros Macros
    @{
  */

#define USBD_PMA_ADDR    (USBD_BASE + 0x400)
#define USBD_PMA_ACCESS  2
#define USBD_BUFFTB_ADDR 0x0000

  /**@} end of group USB_Macros*/

  /** @defgroup USB_Enumerations Enumerations
    @{
  */

  /**
   * @brief    USB Interrupt definition
   */
  typedef enum
  {
    USBD_INT_ESOF  = BIT8,
    USBD_INT_SOF   = BIT9,
    USBD_INT_RST   = BIT10,
    USBD_INT_SUS   = BIT11,
    USBD_INT_WKUP  = BIT12,
    USBD_INT_ERR   = BIT13,
    USBD_INT_PMAOU = BIT14,
    USBD_INT_CTR   = BIT15,
    USBD_INT_ALL   = 0xFF00,
  } USBD_INT_T;

  /**
   * @brief   USBD Endpoint register bit definition
   */
  typedef enum
  {
    USBD_EP_BIT_ADDR   = (uint32_t)(BIT0 | BIT1 | BIT2 | BIT3),
    USBD_EP_BIT_TXSTS  = (uint32_t)(BIT4 | BIT5),
    USBD_EP_BIT_TXDTOG = (uint32_t)(BIT6),
    USBD_EP_BIT_CTFT   = (uint32_t)(BIT7),
    USBD_EP_BIT_KIND   = (uint32_t)(BIT8),
    USBD_EP_BIT_TYPE   = (uint32_t)(BIT9 | BIT10),
    USBD_EP_BIT_SETUP  = (uint32_t)(BIT11),
    USBD_EP_BIT_RXSTS  = (uint32_t)(BIT12 | BIT13),
    USBD_EP_BIT_RXDTOG = (uint32_t)(BIT14),
    USBD_EP_BIT_CTFR   = (uint32_t)(BIT15)
  } USBD_EP_BIT_T;

  /**
   * @brief   Endpoint buffer type
   */
  typedef enum
  {
    USBD_EP_BUFFER_SINGLE,
    USBD_EP_BUFFER_DOUBLE,
  } USBD_EP_BUFFER_T;

  /**
   * @brief   Endpoint status
   */
  typedef enum
  {
    USBD_EP_STATUS_DISABLE,
    USBD_EP_STATUS_STALL,
    USBD_EP_STATUS_NAK,
    USBD_EP_STATUS_VALID,
  } USBD_EP_STATUS_T;

/**@} end of group USB_Enumerations*/

/** @defgroup USB_Macros Macros
  @{
*/

/* Endpoint register mask value default */
#define USBD_EP_MASK_DEFAULT (USBD_EP_BIT_CTFR | USBD_EP_BIT_SETUP | USBD_EP_BIT_TYPE | USBD_EP_BIT_KIND | USBD_EP_BIT_CTFT | USBD_EP_BIT_ADDR)

  /**@} end of group USB_Macros*/

  /** @defgroup USB_Enumerations Enumerations
    @{
  */

  /**
   * @brief   USB endpoint type
   */
  typedef enum
  {
    EP_TYPE_CONTROL,
    EP_TYPE_ISO,
    EP_TYPE_BULK,
    EP_TYPE_INTERRUPT
  } USB_EP_TYPE_T;

  /**
   * @brief   USBD Endpoint type for register
   */
  typedef enum
  {
    USBD_REG_EP_TYPE_BULK,
    USBD_REG_EP_TYPE_CONTROL,
    USBD_REG_EP_TYPE_ISO,
    USBD_REG_EP_TYPE_INTERRUPT
  } USBD_REG_EP_TYPE_T;

  /**
   * @brief   USB endpoint direction
   */
  typedef enum
  {
    EP_DIR_OUT,
    EP_DIR_IN
  } USB_EP_DIR_T;

  /**
   * @brief   USB speed
   */
  typedef enum
  {
    USB_SPEED_HSFSLS,
    USB_SPEED_FSLS,
  } USB_SPEED_T;

  /**
   * @brief   USB device endpoint id
   */
  typedef enum
  {
    USBD_EP_0,
    USBD_EP_1,
    USBD_EP_2,
    USBD_EP_3,
    USBD_EP_4,
    USBD_EP_5,
    USBD_EP_6,
    USBD_EP_7,
  } USBD_EP_ID_T;

  /**@} end of group USB_Enumerations*/

  /** @defgroup USB_Structures Structures
    @{
  */

  /**
   * @brief USB device endpoint information structure
   */
  typedef struct
  {
    uint8_t epNum;
    uint8_t epDir;
    uint8_t epType;
    uint8_t stallStatus;

    uint8_t  dataPID;
    uint32_t mps;
    uint16_t txFifoNum;

    uint16_t pmaAddr;
    uint16_t pmaAddr0;
    uint16_t pmaAddr1;

    USBD_EP_BUFFER_T bufferStatus;
    uint8_t         *buffer;
    uint32_t         bufLen;
    uint32_t         bufCount;

    uint32_t dbBufferLen;
    uint8_t  dbBufferFill;
  } USBD_ENDPOINT_INFO_T;

  /**
   * @brief USB device init structure definition
   */
  typedef struct
  {
    uint32_t lpmStatus;
    uint32_t lowPowerStatus; /*!< Low power mode status */
    uint32_t sofStatus;      /*!< SOF output status */
    uint32_t ep0MaxPackSize;
    uint32_t devEndpointNum; /*!< USB device endpoint number */
    uint32_t speed;          /*!< USB core speed */
    uint8_t  batteryStatus;  /*!< BACKUP BAT status */
  } USBD_CFG_T;

  /**@} end of group USB_Structures*/

  /** @defgroup USB_Functions Functions
    @{
  */

  void     USBD_Enable(USBD_T *usbx);
  void     USBD_Disable(USBD_T *usbx);
  void     USBD_ConfigEP(USBD_T *usbx, USBD_ENDPOINT_INFO_T *ep);
  void     USBD_ResetEP(USBD_T *usbx, USBD_ENDPOINT_INFO_T *ep);
  void     USBD_EP_SetAddr(USBD_T *usbx, uint8_t epNum, uint8_t addr);
  void     USBD_EP_SetType(USBD_T *usbx, uint8_t epNum, USBD_REG_EP_TYPE_T type);
  void     USBD_EP_SetKind(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ResetKind(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ResetTxToggle(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ResetRxToggle(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ToggleTx(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ToggleRx(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ReadPacketData(USBD_T *usbx, uint16_t pmaBufAddr, uint8_t *rBuf, uint32_t rLen);
  void     USBD_EP_WritePacketData(USBD_T *usbx, uint16_t pmaBufAddr, uint8_t *wBuf, uint32_t wLen);
  void     USBD_EP_SetTxStatus(USBD_T *usbx, uint8_t epNum, USBD_EP_STATUS_T status);
  void     USBD_EP_SetRxStatus(USBD_T *usbx, uint8_t epNum, USBD_EP_STATUS_T status);
  void     USBD_EP_SetTxRxStatus(USBD_T *usbx, uint8_t epNum, USBD_EP_STATUS_T txStatus, USBD_EP_STATUS_T rxStatus);
  uint16_t USBD_EP_ReadStatus(USBD_T *usbx, uint8_t epNum);
  uint32_t USBD_EP_ReadTxCnt(USBD_T *usbx, uint8_t epNum);
  uint32_t USBD_EP_ReadRxCnt(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_SetTxAddr(USBD_T *usbx, uint8_t epNum, uint16_t addr);
  void     USBD_EP_SetRxAddr(USBD_T *usbx, uint8_t epNum, uint16_t addr);
  void     USBD_EP_SetTxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt);
  void     USBD_EP_SetRxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt);
  void     USBD_EP_SetBuffer0RxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt);
  void     USBD_EP_SetBuffer0TxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt);
  void     USBD_EP_ResetRxFlag(USBD_T *usbx, uint8_t epNum);
  void     USBD_EP_ResetTxFlag(USBD_T *usbx, uint8_t epNum);
  uint8_t  USBD_EP_ReadDir(USBD_T *usbx);
  uint8_t  USBD_EP_ReadID(USBD_T *usbx);
  void     USBD_SetForceReset(USBD_T *usbx);
  void     USBD_ResetForceReset(USBD_T *usbx);
  void     USBD_SetForceSuspend(USBD_T *usbx);
  void     USBD_ResetForceSuspend(USBD_T *usbx);
  void     USBD_SetWakeupRequest(USBD_T *usbx);
  void     USBD_ResetWakeupRequest(USBD_T *usbx);
  void     USBD_SetLowerPowerMode(USBD_T *usbx);
  void     USBD_ResetLowerPowerMode(USBD_T *usbx);
  void     USBD_SetDeviceAddr(USBD_T *usbx, uint8_t address);
  void     USBD2_Enable(USBD_T *usbx);
  void     USBD2_Disable(USBD_T *usbx);

  void    USBD_EnableInterrupt(USBD_T *usbx, uint32_t interrupt);
  void    USBD_DisableInterrupt(USBD_T *usbx, uint32_t interrupt);
  uint8_t USBD_ReadIntFlag(USBD_T *usbx, uint32_t interrupt);
  void    USBD_ClearIntFlag(USBD_T *usbx, uint32_t interrupt);

  /**@} end of group USB_Device_Functions*/
  /**@} end of group USB_Device_Driver*/
  /**@} end of group APM32F10x_StdPeriphDriver*/

#ifdef __cplusplus
}
#endif

#endif
