/*!
 * @file        apm32f10x_usb.c
 *
 * @brief       This file provides all the USB dirver functions
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

/* Includes */
#include "apm32e10x_usb.h"

/** @addtogroup APM32F10x_StdPeriphDriver
  @{
*/

/** @addtogroup USB_Driver USB Driver
  @{
*/

/** @defgroup USB_Functions Functions
  @{
*/

///*!
// * @brief     Read EP Tx count pointer
// *
// * @param     usbx: USB peripheral
// *
// * @param     epNum: EP number
// *
// * @retval    EP Tx count pointer
// */
//uint32_t* USBD_EP_ReadTxCntPointer(USBD_T *usbx, uint8_t epNum)
//{
//    return (uint32_t *)((USBD->BUFFTB + epNum * 8 + 2) * USBD_PMA_ACCESS + USBD_PMA_ADDR);
//}

///*!
// * @brief     Read EP Rx count pointer
// *
// * @param     usbx: USB peripheral
// *
// * @param     epNum: EP number
// *
// * @retval    EP Tx count pointer
// */
//uint32_t* USBD_EP_ReadRxCntPointer(USBD_T *usbx, uint8_t epNum)
//{
//    return (uint32_t *)((USBD->BUFFTB + epNum * 8 + 6) * USBD_PMA_ACCESS + USBD_PMA_ADDR);
//}

///*!
// * @brief     Read EP Tx address pointer
// *
// * @param     usbx: USB peripheral
// *
// * @param     epNum: EP number
// *
// * @retval    EP Tx address pointer
// */
//uint32_t* USBD_EP_ReadTxAddrPointer(USBD_T *usbx, uint8_t epNum)
//{
//    return (uint32_t *)((USBD->BUFFTB + epNum * 8) * USBD_PMA_ACCESS + USBD_PMA_ADDR);
//}

///*!
// * @brief     Read EP Rx address pointer
// *
// * @param     usbx: USB peripheral
// *
// * @param     epNum: EP number
// *
// * @retval    EP Tx address pointer
// */
//uint32_t* USBD_EP_ReadRxAddrPointer(USBD_T *usbx, uint8_t epNum)
//{
//    return (uint32_t *)((USBD->BUFFTB + epNum * 8 + 4) * USBD_PMA_ACCESS + USBD_PMA_ADDR);
//}

/*!
 * @brief     Read EP Tx count pointer
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx count pointer
 */
#define USBD_EP_ReadTxCntPointer(usbx, epNum)       (uint16_t *)(((uint32_t)USBD->BUFFTB + ((uint32_t)epNum * 8) + 2) * USBD_PMA_ACCESS + (uint32_t)USBD_PMA_ADDR)

/*!
 * @brief     Read EP Rx count pointer
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx count pointer
 */
#define USBD_EP_ReadRxCntPointer(usbx, epNum)       (uint16_t *)(((uint32_t)USBD->BUFFTB + ((uint32_t)epNum * 8) + 6) * USBD_PMA_ACCESS + (uint32_t)USBD_PMA_ADDR)

/*!
 * @brief     Read EP Tx address pointer
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx address pointer
 */
#define USBD_EP_ReadTxAddrPointer(usbx, epNum)       (uint32_t *)(((uint32_t)USBD->BUFFTB + (uint32_t)epNum * 8) * USBD_PMA_ACCESS + (uint32_t)USBD_PMA_ADDR)

/*!
 * @brief     Read EP Rx address pointer
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx address pointer
 */
#define USBD_EP_ReadRxAddrPointer(usbx, epNum)       (uint32_t *)(((uint32_t)USBD->BUFFTB + ((uint32_t)epNum * 8) + 4) * USBD_PMA_ACCESS + (uint32_t)USBD_PMA_ADDR)

/*!
 * @brief     Read EP Tx addr
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx addr
 */
uint32_t USBD_EP_ReadTxAddr(USBD_T *usbx, uint8_t epNum)
{
    return ((uint32_t)*USBD_EP_ReadTxAddrPointer(usbx, epNum));
}

/*!
 * @brief     Read EP Rx addr
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx addr
 */
uint32_t USBD_EP_ReadRxAddr(USBD_T *usbx, uint8_t epNum)
{
    return ((uint32_t)*USBD_EP_ReadRxAddrPointer(usbx, epNum));
}

/*!
 * @brief     Set EP Tx addr
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @param     addr: EP address
 *
 * @retval    None
 */
void USBD_EP_SetTxAddr(USBD_T *usbx, uint8_t epNum, uint16_t addr)
{
    (*USBD_EP_ReadTxAddrPointer(usbx, epNum) = (addr >> 1) << 1);
}

/*!
 * @brief     Set EP Rx addr
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @param     addr: EP address
 *
 * @retval    None
 */
void USBD_EP_SetRxAddr(USBD_T *usbx, uint8_t epNum, uint16_t addr)
{
    (*USBD_EP_ReadRxAddrPointer(usbx, epNum) = (addr >> 1) << 1);
}

/*!
 * @brief     Read EP Tx Buffer Pointer
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx Buffer Pointer
 */
uint32_t* USBD_EP_ReadTxBufferPointer(USBD_T *usbx, uint8_t epNum)
{
    return (uint32_t *)(((uint32_t)USBD_EP_ReadTxAddr(usbx, epNum) << 1) + USBD_PMA_ADDR);
}

/*!
 * @brief     Read EP Rx Buffer Pointer
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx Buffer Pointer
 */
uint32_t* USBD_EP_ReadRxBufferPointer(USBD_T *usbx, uint8_t epNum)
{
    return (uint32_t *)(((uint32_t)USBD_EP_ReadRxAddr(usbx, epNum) << 1) + USBD_PMA_ADDR);
}

/*!
 * @brief     Set Endpoint type
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     type: Endpoint type
 *
 * @retval    None
 */
void USBD_EP_SetType(USBD_T *usbx, uint8_t epNum, USBD_REG_EP_TYPE_T type)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg &= ~USBD_EP_BIT_TYPE;
    reg |= type << 9;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Set EP address
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     addr: Address
 *
 * @retval    None
 */
void USBD_EP_SetAddr(USBD_T *usbx, uint8_t epNum, uint8_t addr)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg &= ~USBD_EP_BIT_ADDR;
    reg |= addr;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Set EP kind
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_SetKind(USBD_T *usbx, uint8_t epNum)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg |= USBD_EP_BIT_KIND;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Reset EP kind
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ResetKind(USBD_T *usbx, uint8_t epNum)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg &= ~USBD_EP_BIT_KIND;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Read EP Tx count
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Tx count
 */
uint32_t USBD_EP_ReadTxCnt(USBD_T *usbx, uint8_t epNum)
{
    return ((uint32_t)*USBD_EP_ReadTxCntPointer(usbx, epNum) & 0x3FF);
}

/*!
 * @brief     Read EP Rx count
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP Rx count
 */
uint32_t USBD_EP_ReadRxCnt(USBD_T *usbx, uint8_t epNum)
{
    return ((uint32_t)*USBD_EP_ReadRxCntPointer(usbx, epNum) & 0x3FF);
}

/*!
 * @brief     Set EP Rx buffer0 Count
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     cnt: Rx count
 *
 * @retval    None
 */
void USBD_EP_SetBuffer0RxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt)
{
    __IO uint16_t* p;
    __IO uint16_t block = 0;

    p = (__IO uint16_t*)USBD_EP_ReadTxCntPointer(usbx, epNum);

    if(cnt == 0)
    {
        *p &= (uint16_t)~(0x1F << 10);
        *p |= BIT15;
    }
    else if (cnt > 62)
    {
        block = cnt >> 5;

        if (!(cnt & 0x1F))
        {
            block -= 1;
        }

        *p = (uint16_t)((block << 10) | BIT15);
    }
    else
    {
        block = cnt >> 1;

        if (cnt & 0x01)
        {
            block += 1;
        }

        *p = (uint16_t)(block << 10);
    }
}

/*!
 * @brief     Set EP Tx Count
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     cnt: Tx count
 *
 * @retval    None
 */
void USBD_EP_SetTxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt)
{
    __IO uint16_t* p;
//    __IO uint16_t block = 0;

    p = (__IO uint16_t*)USBD_EP_ReadTxCntPointer(usbx, epNum);
    
    *p = (uint16_t)cnt;
}

/*!
 * @brief     Set EP Tx buffer0 Count
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     cnt: Tx count
 *
 * @retval    None
 */
void USBD_EP_SetBuffer0TxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt)
{
    __IO uint16_t* p;
//    __IO uint16_t block = 0;

    p = (__IO uint16_t*)USBD_EP_ReadRxCntPointer(usbx, epNum);
    
    *p = (uint16_t)cnt;
}

/*!
 * @brief     Set EP Rx Count
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     cnt: Rx count
 *
 * @retval    None
 */
void USBD_EP_SetRxCnt(USBD_T *usbx, uint8_t epNum, uint32_t cnt)
{
    __IO uint16_t* p;
    __IO uint16_t block = 0;

    p = (__IO uint16_t*)USBD_EP_ReadRxCntPointer(usbx, epNum);

    if(cnt == 0)
    {
        *p &= (uint16_t)~(0x1F << 10);
        *p |= BIT15;
    }
    else if (cnt > 62)
    {
        block = cnt >> 5;

        if (!(cnt & 0x1F))
        {
            block -= 1;
        }

        *p = (uint16_t)((block << 10) | BIT15);
    }
    else
    {
        block = cnt >> 1;

        if (cnt & 0x01)
        {
            block += 1;
        }

        *p = (uint16_t)(block << 10);
    }
}

/*!
 * @brief     Set EP Tx status
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     status: status
 *
 * @retval    None
 */
void USBD_EP_SetTxStatus(USBD_T *usbx, uint8_t epNum, USBD_EP_STATUS_T status)
{
    __IOM uint32_t reg;

    status <<= 4;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT | USBD_EP_BIT_TXSTS);
    reg ^= ((uint32_t)status & (uint32_t)USBD_EP_BIT_TXSTS);

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Set EP Rx status
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     status: status
 *
 * @retval    None
 */
void USBD_EP_SetRxStatus(USBD_T *usbx, uint8_t epNum, USBD_EP_STATUS_T status)
{
    __IOM uint32_t reg;
    uint32_t tmp;

    tmp = status << 12;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT | USBD_EP_BIT_RXSTS);
    reg ^= (tmp & USBD_EP_BIT_RXSTS);

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Toggle Tx DTOG
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ToggleTx(USBD_T *usbx, uint8_t epNum)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg |= USBD_EP_BIT_TXDTOG;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Reset Toggle Tx DTOG
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ResetTxToggle(USBD_T *usbx, uint8_t epNum)
{
    if (usbx->EP[epNum].EP_B.TXDTOG)
    {
        USBD_EP_ToggleTx(usbx, epNum);
    }
}

/*!
 * @brief     Toggle Rx DTOG
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ToggleRx(USBD_T *usbx, uint8_t epNum)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg |= USBD_EP_BIT_RXDTOG;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Reset Toggle Rx DTOG
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ResetRxToggle(USBD_T *usbx, uint8_t epNum)
{
    if (usbx->EP[epNum].EP_B.RXDTOG)
    {
        USBD_EP_ToggleRx(usbx, epNum);
    }
}

/*!
 * @brief     Configure endpoint
 *
 * @param     usbx: USB peripheral
 *
 * @param     ep: Endpoint handler
 *
 * @retval    None
 */
void USBD_ConfigEP(USBD_T *usbx, USBD_ENDPOINT_INFO_T *ep)
{
    switch(ep->epType)
    {
        case EP_TYPE_BULK:
            USBD_EP_SetType(usbx, ep->epNum, USBD_REG_EP_TYPE_BULK);
            break;
        
        case EP_TYPE_CONTROL:
            USBD_EP_SetType(usbx, ep->epNum, USBD_REG_EP_TYPE_CONTROL);
            break;
        
        case EP_TYPE_INTERRUPT:
            USBD_EP_SetType(usbx, ep->epNum, USBD_REG_EP_TYPE_INTERRUPT);
            break;
        
        case EP_TYPE_ISO:
            USBD_EP_SetType(usbx, ep->epNum, USBD_REG_EP_TYPE_ISO);
            break;
        
        default:
            break;
    }
    
    USBD_EP_SetAddr(usbx, ep->epNum, ep->epNum);
    
    if(ep->bufferStatus == USBD_EP_BUFFER_SINGLE)
    {
        if(ep->epDir == EP_DIR_IN)
        {
            USBD_EP_SetTxAddr(usbx, ep->epNum, ep->pmaAddr);
            USBD_EP_ResetTxToggle(usbx, ep->epNum);
            
            if(ep->epType != EP_TYPE_ISO)
            {
                USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_NAK);
            }
            else
            {
                USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
            }
        }
        else
        {
            USBD_EP_SetRxAddr(usbx, ep->epNum, ep->pmaAddr);
            
            USBD_EP_SetRxCnt(usbx, ep->epNum, ep->mps);
            USBD_EP_ResetRxToggle(usbx, ep->epNum);
            
            USBD_EP_SetRxStatus(usbx, ep->epNum, USBD_EP_STATUS_VALID);
        }
    }
    else
    {
        USBD_EP_SetKind(usbx, ep->epNum);
        
        USBD_EP_SetTxAddr(usbx, ep->epNum, ep->pmaAddr0);
        USBD_EP_SetRxAddr(usbx, ep->epNum, ep->pmaAddr1);
        
        if(ep->epDir == EP_DIR_IN)
        {
            USBD_EP_ResetRxToggle(usbx, ep->epNum);
            USBD_EP_ResetTxToggle(usbx, ep->epNum);
            
            if(ep->epType != EP_TYPE_ISO)
            {
                USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_NAK);
            }
            else
            {
                USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
            }
            
            USBD_EP_SetRxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
        }
        else
        {
            USBD_EP_ResetRxToggle(usbx, ep->epNum);
            USBD_EP_ResetTxToggle(usbx, ep->epNum);
            
            USBD_EP_SetRxStatus(usbx, ep->epNum, USBD_EP_STATUS_VALID);
            USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
        }
    }
}

/*!
 * @brief     Reset endpoint
 *
 * @param     usbx: USB peripheral
 *
 * @param     ep: Endpoint handler
 *
 * @retval    None
 */
void USBD_ResetEP(USBD_T *usbx, USBD_ENDPOINT_INFO_T *ep)
{
    if(ep->bufferStatus == USBD_EP_BUFFER_SINGLE)
    {
        if(ep->epDir == EP_DIR_IN)
        {
            USBD_EP_ResetTxToggle(usbx, ep->epNum);
            
            USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
        }
        else
        {
            USBD_EP_ResetRxToggle(usbx, ep->epNum);
            
            USBD_EP_SetRxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
        }
    }
    else
    {
        if(ep->epDir == EP_DIR_IN)
        {
            USBD_EP_ResetRxToggle(usbx, ep->epNum);
            USBD_EP_ResetTxToggle(usbx, ep->epNum);
            USBD_EP_ToggleRx(usbx, ep->epNum);
            
            USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
            USBD_EP_SetRxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
        }
        else
        {
            USBD_EP_ResetRxToggle(usbx, ep->epNum);
            USBD_EP_ResetTxToggle(usbx, ep->epNum);
            USBD_EP_ToggleTx(usbx, ep->epNum);
            
            USBD_EP_SetTxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
            USBD_EP_SetRxStatus(usbx, ep->epNum, USBD_EP_STATUS_DISABLE);
        }
    }
}

/*!
 * @brief     Set EP Tx and Rx status
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @param     txStatus: TX status
 *
 * @param     rxStatus: RX status
 *
 * @retval    None
 */
void USBD_EP_SetTxRxStatus(USBD_T *usbx, uint8_t epNum, USBD_EP_STATUS_T txStatus, USBD_EP_STATUS_T rxStatus)
{
    __IOM uint32_t reg;
    uint32_t tmp;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT | USBD_EP_BIT_RXSTS | USBD_EP_BIT_TXSTS);

    tmp = rxStatus << 12;
    reg ^= (tmp & USBD_EP_BIT_RXSTS);

    tmp = txStatus << 4;
    reg ^= (tmp & USBD_EP_BIT_TXSTS);

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Read a buffer of data to a selected endpoint
 *
 * @param     usbx: USB peripheral
 *
 * @param     pmaBufAddr: PMA buffer address
 *
 * @param     rBuf: Buffer pointer
 *
 * @param     rLen: Buffer length
 *
 * @retval    None
 */
void USBD_EP_ReadPacketData(USBD_T *usbx, uint16_t pmaBufAddr, uint8_t* rBuf, uint32_t rLen)
{
    __IO uint16_t* epAddr;
    uint32_t i, temp, cnt;

    cnt = rLen >> 1;

    //epAddr = (__IO uint16_t *)USBD_EP_ReadRxBufferPointer(usbx, epNum);
    epAddr = (__IO uint16_t *)(USBD_PMA_ADDR + ((uint32_t)pmaBufAddr * USBD_PMA_ACCESS));
    
    for (i = 0; i < cnt; i++)
    {
        temp = *epAddr++;
        *rBuf++ = temp & 0xFF;
        *rBuf++ = (temp >> 8) & 0xFF;
        
#if USBD_PMA_ACCESS > 1
        epAddr++;
#endif
    }

    if (rLen & 1)
    {
        temp = *epAddr;
        *rBuf = temp & 0xFF;
    }
}

/*!
 * @brief     Write a buffer of data to a selected endpoint
 *
 * @param     usbx: USB peripheral
 *
 * @param     pmaBufAddr: PMA buffer address
 *
 * @param     wBuf: Buffer pointer
 *
 * @param     wLen: Buffer length
 *
 * @retval    None
 */
void USBD_EP_WritePacketData(USBD_T *usbx, uint16_t pmaBufAddr, uint8_t* wBuf, uint32_t wLen)
{
    uint32_t i;

    __IO uint16_t* epAddr;
    uint32_t temp;

    wLen = (wLen + 1) >> 1;

    //epAddr = USBD_EP_ReadTxBufferPointer(usbx, epNum);

    epAddr = (__IO uint16_t *)(USBD_PMA_ADDR + ((uint32_t)pmaBufAddr * USBD_PMA_ACCESS));
    
    for (i = 0; i < wLen; i++)
    {
        temp = *wBuf++;
        temp = ((*wBuf++) << 8) | temp;

        *epAddr++ = temp;
        
#if USBD_PMA_ACCESS > 1
        epAddr++;
#endif
    }
}

/*!
 * @brief     Read EP status
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: EP number
 *
 * @retval    EP status
 */
uint16_t USBD_EP_ReadStatus(USBD_T *usbx, uint8_t epNum)
{
    return (uint16_t)usbx->EP[epNum].EP;
}

/*!
 * @brief     Read DOT field value in INTSTS rigister
 *
 * @param     usbx: USB peripheral
 *
 * @retval    DOT field value
 */
uint8_t USBD_EP_ReadDir(USBD_T *usbx)
{
    return (usbx->INTSTS_B.DOT);
}

/*!
 * @brief     Read EPID field value in INTSTS rigister
 *
 * @param     usbx: USB peripheral
 *
 * @retval    EPID field value
 */
uint8_t USBD_EP_ReadID(USBD_T *usbx)
{
    return (usbx->INTSTS_B.EPID);
}

/*!
 * @brief     Reset EP CTFR bit
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ResetRxFlag(USBD_T *usbx, uint8_t epNum)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg &= ~USBD_EP_BIT_CTFR;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Reset EP CTFT bit
 *
 * @param     usbx: USB peripheral
 *
 * @param     epNum: Endpoint number
 *
 * @retval    None
 */
void USBD_EP_ResetTxFlag(USBD_T *usbx, uint8_t epNum)
{
    __IOM uint32_t reg;

    reg = usbx->EP[epNum].EP;

    reg &= (uint32_t)(USBD_EP_MASK_DEFAULT);
    reg &= ~USBD_EP_BIT_CTFT;

    usbx->EP[epNum].EP = reg;
}

/*!
 * @brief     Set force reset
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_SetForceReset(USBD_T *usbx)
{
    usbx->CTRL_B.FORRST = BIT_SET;
}

/*!
 * @brief     Reset force reset
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_ResetForceReset(USBD_T *usbx)
{
    usbx->CTRL_B.FORRST = BIT_RESET;
}

/*!
 * @brief     Set force Wakeup Request
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_SetWakeupRequest(USBD_T *usbx)
{
    usbx->CTRL_B.WUPREQ = BIT_SET;
}

/*!
 * @brief     Reset force Wakeup Request
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_ResetWakeupRequest(USBD_T *usbx)
{
    usbx->CTRL_B.WUPREQ = BIT_RESET;
}

/*!
 * @brief     Set low power mode
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_SetLowerPowerMode(USBD_T *usbx)
{
    usbx->CTRL_B.LPWREN = BIT_SET;
}

/*!
 * @brief     Ret low power mode
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_ResetLowerPowerMode(USBD_T *usbx)
{
    usbx->CTRL_B.LPWREN = BIT_RESET;
}

/*!
 * @brief     Set force suspend
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_SetForceSuspend(USBD_T *usbx)
{
    usbx->CTRL_B.FORSUS = BIT_SET;
}

/*!
 * @brief     Reset force suspend
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_ResetForceSuspend(USBD_T *usbx)
{
    usbx->CTRL_B.FORSUS = BIT_RESET;
}

/*!
 * @brief     Enable USBD peripheral
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_Enable(USBD_T *usbx)
{
    usbx->ADDR_B.USBDEN = BIT_SET;
}

/*!
 * @brief     Disable USBD peripheral
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD_Disable(USBD_T *usbx)
{
    usbx->ADDR_B.USBDEN = BIT_RESET;
}

/*!
 * @brief     Enable USBD2 peripheral
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD2_Enable(USBD_T *usbx)
{
    usbx->SWITCH = BIT_SET;
}

/*!
 * @brief     Disable USBD2 peripheral
 *
 * @param     usbx: USB peripheral
 *
 * @retval    None
 */
void USBD2_Disable(USBD_T *usbx)
{
    usbx->SWITCH = BIT_RESET;
}

/*!
 * @brief     Set device address
 *
 * @param     usbx: USB peripheral
 *
 * @param     addr: Device address
 *
 * @retval    None
 */
void USBD_SetDeviceAddr(USBD_T *usbx, uint8_t address)
{
    usbx->ADDR_B.ADDR = address;
}

/*!
 * @brief     Enable interrupt
 *
 * @param     usbx: USB peripheral
 *
 * @param     interrupt: Interrupt source
 *
 * @retval    None
 */
void USBD_EnableInterrupt(USBD_T *usbx, uint32_t interrupt)
{
    USBD->CTRL |= interrupt;
}

/*!
 * @brief     Disable interrupt
 *
 * @param     usbx: USB peripheral
 *
 * @param     interrupt: Interrupt source
 *
 * @retval    None
 */
void USBD_DisableInterrupt(USBD_T *usbx, uint32_t interrupt)
{
    USBD->CTRL &= ~interrupt;
}

/*!
 * @brief     Read the specified interrupt flag status
 *
 * @param     usbx: USB peripheral
 *
 * @param     interrupt: Interrupt source
 *
 * @retval    Flag status.0 or not 0
 */
uint8_t USBD_ReadIntFlag(USBD_T *usbx, uint32_t interrupt)
{
    return (usbx->INTSTS & interrupt) ? SET : RESET;
}

/*!
 * @brief     Clear the specified interrupt flag status
 *
 * @param     usbx: USB peripheral
 *
 * @param     interrupt: Interrupt source
 *
 * @retval    None
 */
void USBD_ClearIntFlag(USBD_T *usbx, uint32_t interrupt)
{
    usbx->INTSTS &= ~interrupt;
}
