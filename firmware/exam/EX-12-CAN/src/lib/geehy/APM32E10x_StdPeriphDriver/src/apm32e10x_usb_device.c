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

/* Includes */
#include "apm32e10x_usb_device.h"
#include "bsp.h"

/** @addtogroup APM32F10x_StdPeriphDriver
  @{
*/

/** @addtogroup USB_Device_Driver USB Device Driver
  @{
*/

/** @defgroup USB_Device_Functions Functions
  @{
*/

/*!
 * @brief     USB device start
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_Start(USBD_HANDLE_T* usbdh)
{
    usbdh->usbGlobal->INTSTS = 0;
    
    USBD_EnableInterrupt(usbdh->usbGlobal, USBD_INT_CTR | \
                                           USBD_INT_WKUP | \
                                           USBD_INT_SUS | \
                                           USBD_INT_ERR | \
                                           USBD_INT_RST | \
                                           USBD_INT_SOF | \
                                           USBD_INT_ESOF);
}

/*!
 * @brief     USB device stop
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_Stop(USBD_HANDLE_T* usbdh)
{
    /* Disable USB all global interrupt */
    USBD_DisableInterrupt(usbdh->usbGlobal, USBD_INT_CTR | \
                                            USBD_INT_WKUP | \
                                            USBD_INT_SUS | \
                                            USBD_INT_ERR | \
                                            USBD_INT_RST | \
                                            USBD_INT_SOF | \
                                            USBD_INT_ESOF);
}

/*!
 * @brief     USB device stop
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_StopDevice(USBD_HANDLE_T* usbdh)
{
    USBD_SetForceReset(usbdh->usbGlobal);
    
    usbdh->usbGlobal->INTSTS = 0;
    
    usbdh->usbGlobal->CTRL_B.FORRST = BIT_SET;
    usbdh->usbGlobal->CTRL_B.PWRDOWN = BIT_SET;
}

/*!
 * @brief     USB device open EP
 *
 * @param     usbdh: USB device handler
 *
 * @param     epAddr: endpoint address
 *
 * @param     epType: endpoint type
 *
 * @param     epMps: endpoint maxinum of packet size
 *
 * @retval    None
 */
void USBD_EP_Open(USBD_HANDLE_T* usbdh, uint8_t epAddr, \
                  uint8_t epType, uint16_t epMps)
{
    uint8_t epAddrTemp = epAddr & 0x0F;

    if ((epAddr & 0x80) == 0x80)
    {
        usbdh->epIN[epAddrTemp].epDir = EP_DIR_IN;

        usbdh->epIN[epAddrTemp].epNum   = epAddrTemp;
        usbdh->epIN[epAddrTemp].epType  = epType;
        usbdh->epIN[epAddrTemp].mps     = epMps;

        usbdh->epIN[epAddrTemp].txFifoNum = usbdh->epIN[epAddrTemp].epNum;
    }
    else
    {
        usbdh->epOUT[epAddrTemp].epDir = EP_DIR_OUT;

        usbdh->epOUT[epAddrTemp].epNum   = epAddrTemp;
        usbdh->epOUT[epAddrTemp].epType  = epType;
        usbdh->epOUT[epAddrTemp].mps     = epMps;
    }

    /* Init data PID */
    if (epType == EP_TYPE_BULK)
    {
        usbdh->epIN[epAddrTemp].dataPID = 0;
    }

    if ((epAddr & 0x80) == 0x80)
    {
        USBD_ConfigEP(usbdh->usbGlobal, &usbdh->epIN[epAddrTemp]);
    }
    else
    {
        USBD_ConfigEP(usbdh->usbGlobal, &usbdh->epOUT[epAddrTemp]);
    }
}

/*!
 * @brief     USB device close EP
 *
 * @param     usbdh: USB device handler
 *
 * @param     epAddr: endpoint address
 *
 * @retval    None
 */
void USBD_EP_Close(USBD_HANDLE_T* usbdh, uint8_t epAddr)
{
    uint8_t epAddrTemp = epAddr & 0x0F;

    if ((epAddr & 0x80) == 0x80)
    {
        usbdh->epIN[epAddrTemp].epDir = EP_DIR_IN;
    }
    else
    {
        usbdh->epOUT[epAddrTemp].epDir = EP_DIR_OUT;
    }

    if ((epAddr & 0x80) == 0x80)
    {
        USBD_ResetEP(usbdh->usbGlobal, &usbdh->epIN[epAddrTemp]);
    }
    else
    {
        USBD_ResetEP(usbdh->usbGlobal, &usbdh->epOUT[epAddrTemp]);
    }
}

/*!
 * @brief     USB device set EP on stall status
 *
 * @param     usbdh: USB device handler
 *
 * @param     epAddr: endpoint address
 *
 * @retval    None
 */
void USBD_EP_ClearStall(USBD_HANDLE_T* usbdh, uint8_t epAddr)
{
    USBD_ENDPOINT_INFO_T *ep;
    
    if ((epAddr & 0x0F) > usbdh->usbCfg.devEndpointNum)
    {
        return;
    }

    if ((epAddr & 0x80) == 0x80)
    {
        ep = &usbdh->epIN[epAddr & 0x0F];
        ep->epDir = EP_DIR_IN;
    }
    else
    {
        ep = &usbdh->epOUT[epAddr & 0x0F];
        ep->epDir = EP_DIR_OUT;
    }

    ep->stallStatus = DISABLE;
    ep->epNum = epAddr & 0x0F;

    if(ep->bufferStatus == USBD_EP_BUFFER_SINGLE)
    {
        if (ep->epDir == EP_DIR_IN)
        {
            USBD_EP_ResetTxToggle(usbdh->usbGlobal, ep->epNum);
            
            if(ep->epType != EP_TYPE_ISO)
            {
                USBD_EP_SetTxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_NAK);
            }
        }
        else
        {
            USBD_EP_ResetRxToggle(usbdh->usbGlobal, ep->epNum);
            
            USBD_EP_SetRxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_VALID);
        }
    }
}

/*!
 * @brief     USB device set EP on stall status
 *
 * @param     usbdh: USB device handler
 *
 * @param     epAddr: endpoint address
 *
 * @retval    None
 */
void USBD_EP_Stall(USBD_HANDLE_T* usbdh, uint8_t epAddr)
{
    USBD_ENDPOINT_INFO_T *ep;
    
    if ((epAddr & 0x0F) > usbdh->usbCfg.devEndpointNum)
    {
        return;
    }
    
    if ((epAddr & 0x80) == 0x80)
    {
        ep = &usbdh->epIN[epAddr & 0x0F];
        ep->epDir = EP_DIR_IN;
    }
    else
    {
        ep = &usbdh->epOUT[epAddr & 0x0F];
        ep->epDir = EP_DIR_OUT;
    }

    ep->stallStatus = ENABLE;
    ep->epNum = epAddr & 0x0F;
    
    if (ep->epDir == EP_DIR_IN)
    {
        USBD_EP_SetTxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_STALL);
    }
    else
    {
        USBD_EP_SetRxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_STALL);
    }
}

/*!
 * @brief     USB device get EP stall status
 *
 * @param     usbdh: USB device handler
 *
 * @param     epAddr: endpoint address
 *
 * @retval    Stall status
 */
uint8_t USBD_EP_ReadStallStatus(USBD_HANDLE_T* usbdh, uint8_t epAddr)
{
    if ((epAddr & 0x80) == 0x80)
    {
        return (usbdh->epIN[epAddr & 0x7F].stallStatus);
    }
    else
    {
        return (usbdh->epOUT[epAddr & 0x7F].stallStatus);
    }
}

/*!
 * @brief     USB device EP start transfer
 *
 * @param     usbdh: USB device handler.
 *
 * @param     ep : endpoint handler
 *
 * @retval    None
 */
void USBD_EP_XferStart(USBD_HANDLE_T* usbdh, USBD_ENDPOINT_INFO_T* ep)
{
    uint32_t length;
    uint16_t epStatus;
    
    if(ep->epDir == EP_DIR_IN)
    {
        if(ep->bufLen > ep->mps)
        {
            length = ep->mps;
        }
        else
        {
            length = ep->bufLen;
        }
        
        if(ep->bufferStatus == USBD_EP_BUFFER_SINGLE)
        {
            USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr, ep->buffer, length);
            USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
        }
        else
        {
            /* double buffer */
            if(ep->epType == EP_TYPE_BULK)
            {
                if(ep->dbBufferLen > ep->mps)
                {
                    USBD_EP_SetKind(usbdh->usbGlobal, ep->epNum);
                    
                    ep->dbBufferLen -= length;
                    
                    epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, ep->epNum);
                    
                    if(epStatus & USBD_EP_BIT_TXDTOG)
                    {
                        /* Buffer1 */
                        if(ep->epDir == 0)
                        {
                            USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        else
                        {
                            USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        
                        USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr1, ep->buffer, length);
                        ep->buffer += length;
                        
                        if(ep->dbBufferLen > ep->mps)
                        {
                            ep->dbBufferLen -= length;
                        }
                        else
                        {
                            length = ep->dbBufferLen;
                            ep->dbBufferLen = 0;
                        }
                        
                        /* Buffer0 */
                        if(ep->epDir == 0)
                        {
                            USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        else
                        {
                            USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        
                        USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr0, ep->buffer, length);
                    }
                    else
                    {
                        /* Buffer0 */
                        if(ep->epDir == 0)
                        {
                            USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        else
                        {
                            USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        
                        USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr0, ep->buffer, length);
                        ep->buffer += length;
                        
                        if(ep->dbBufferLen > ep->mps)
                        {
                            ep->dbBufferLen -= length;
                        }
                        else
                        {
                            length = ep->dbBufferLen;
                            ep->dbBufferLen = 0;
                        }
                        
                        /* Buffer1 */
                        if(ep->epDir == 0)
                        {
                            USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        else
                        {
                            USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        
                        USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr1, ep->buffer, length);
                    }
                }
                else
                {
                    USBD_EP_ResetKind(usbdh->usbGlobal, ep->epNum);
                    
                    length = ep->dbBufferLen;
                    
                    USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                    
                    USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr0, ep->buffer, length);
                }
            }
            else
            {
                USBD_EP_SetKind(usbdh->usbGlobal, ep->epNum);
                
                ep->dbBufferLen -= length;
                
                epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, ep->epNum);
                
                if(epStatus & USBD_EP_BIT_TXDTOG)
                {
                    /* Buffer1 */
                    if(ep->epDir == 0)
                    {
                        USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    else
                    {
                        USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    
                    USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr1, ep->buffer, length);
                    ep->buffer += length;
                    
                    if(ep->dbBufferLen > ep->mps)
                    {
                        ep->dbBufferLen -= length;
                    }
                    else
                    {
                        length = ep->dbBufferLen;
                        ep->dbBufferLen = 0;
                    }
                    
                    if(length)
                    {
                        /* Buffer0 */
                        if(ep->epDir == 0)
                        {
                            USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        else
                        {
                            USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        
                        USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr0, ep->buffer, length);
                    }
                }
                else
                {
                    /* Buffer0 */
                    if(ep->epDir == 0)
                    {
                        USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    else
                    {
                        USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    
                    USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr0, ep->buffer, length);
                    ep->buffer += length;
                    
                    if(ep->dbBufferLen > ep->mps)
                    {
                        ep->dbBufferLen -= length;
                    }
                    else
                    {
                        length = ep->dbBufferLen;
                        ep->dbBufferLen = 0;
                    }
                    
                    if(length)
                    {
                        /* Buffer1 */
                        if(ep->epDir == 0)
                        {
                            USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        else
                        {
                            USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, length);
                        }
                        
                        USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr1, ep->buffer, length);
                    }
                }
            }
        }
        
        USBD_EP_SetTxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_VALID);
    }
    else
    {
        if(ep->bufferStatus == USBD_EP_BUFFER_SINGLE)
        {
            if(ep->bufLen > ep->mps)
            {
                length = ep->mps;
                ep->bufLen -= length;
            }
            else
            {
                length = ep->bufLen;
                ep->bufLen = 0;
            }
            
            USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
        }
        else
        {
            switch(ep->epType)
            {
                case EP_TYPE_BULK:
                    /* Buffer0 */
                    if(ep->epDir == 0)
                    {
                        USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, ep->mps);
                    }
                    else
                    {
                        USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, ep->mps);
                    }
                    
                    /* Buffer1 */
                    if(ep->epDir == 0)
                    {
                        USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, ep->mps);
                    }
                    else
                    {
                        USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, ep->mps);
                    }
                    
                    if(ep->bufCount)
                    {
                        epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, ep->epNum);
                        
                        if(((epStatus & USBD_EP_BIT_RXDTOG) && ((epStatus & USBD_EP_BIT_TXDTOG))) || \
                           (((epStatus & USBD_EP_BIT_RXDTOG) == 0) && ((epStatus & USBD_EP_BIT_TXDTOG) == 0)))
                        {
                            USBD_EP_ToggleTx(usbdh->usbGlobal,ep->epNum);
                        }
                    }
                    break;
                
                case EP_TYPE_ISO:
                    if(ep->bufLen > ep->mps)
                    {
                        length = ep->mps;
                        ep->bufLen -= length;
                    }
                    else
                    {
                        length = ep->bufLen;
                        ep->bufLen = 0;
                    }
                    
                    /* Buffer0 */
                    if(ep->epDir == 0)
                    {
                        USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    else
                    {
                        USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    
                    /* Buffer1 */
                    if(ep->epDir == 0)
                    {
                        USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    else
                    {
                        USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, length);
                    }
                    break;
                
                default:
                    return;
            }
        }
        
        USBD_EP_SetRxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_VALID);
    }
}

/*!
 * @brief     USB device EP receive handler
 *
 * @param     usbdh: USB device handler.
 *
 * @param     epAddr : endpoint address
 *
 * @param     buffer : data buffer
 *
 * @param     length : length of data
 *
 * @retval    None
 */
void USBD_EP_Receive(USBD_HANDLE_T* usbdh, uint8_t epAddr, \
                     uint8_t* buffer, uint32_t length)
{
    uint8_t epAddrTemp = epAddr & 0x0F;

    usbdh->epOUT[epAddrTemp].epNum = epAddr & 0x0F;
    usbdh->epOUT[epAddrTemp].epDir = EP_DIR_OUT;

    usbdh->epOUT[epAddrTemp].buffer = buffer;
    usbdh->epOUT[epAddrTemp].bufCount = 0;
    usbdh->epOUT[epAddrTemp].bufLen = length;

    if (epAddr & 0x0F)
    {
        USBD_EP_XferStart(usbdh, &usbdh->epOUT[epAddrTemp]);
    }
    else
    {
        /* EP0 */
        USBD_EP_XferStart(usbdh, &usbdh->epOUT[epAddrTemp]);
    }
}

/*!
 * @brief     USB device EP transfer handler
 *
 * @param     usbdh: USB device handler.
 *
 * @param     epAddr : endpoint address
 *
 * @param     buffer : data buffer
 *
 * @param     length : length of data
 *
 * @retval    None
 */
void USBD_EP_Transfer(USBD_HANDLE_T* usbdh, uint8_t epAddr, \
                      uint8_t* buffer, uint32_t length)
{
    uint8_t epAddrTemp = epAddr & 0x0F;

    usbdh->epIN[epAddrTemp].epNum = epAddr & 0x0F;
    usbdh->epIN[epAddrTemp].epDir = EP_DIR_IN;

    usbdh->epIN[epAddrTemp].buffer = buffer;
    usbdh->epIN[epAddrTemp].bufCount = 0;
    usbdh->epIN[epAddrTemp].bufLen = length;
    
#if defined (USB_DEVICE)
    usbdh->epIN[epAddrTemp].dbBufferFill = ENABLE;
    usbdh->epIN[epAddrTemp].dbBufferLen = length;
#endif

    if (epAddr & 0x0F)
    {
        USBD_EP_XferStart(usbdh, &usbdh->epIN[epAddrTemp]);
    }
    else
    {
        /* EP0 */
        USBD_EP_XferStart(usbdh, &usbdh->epIN[epAddrTemp]);
    }
}

/*!
 * @brief     USB device read EP last receive data size
 *
 * @param     usbdh: USB device handler.
 *
 * @param     epAddr: endpoint address
 *
 * @retval    size of last receive data
 */
uint32_t USBD_EP_ReadRxDataLen(USBD_HANDLE_T* usbdh, uint8_t epAddr)
{
    return usbdh->epOUT[epAddr & 0x0F].bufCount;
}

/*!
 * @brief     USB device flush EP handler
 *
 * @param     usbdh: USB device handler.
 *
 * @param     epAddr : endpoint address
 *
 * @retval    usb device status
 */
void USBD_EP_Flush(USBD_HANDLE_T* usbdh, uint8_t epAddr)
{
    if (epAddr & 0x80)
    {

    }
    else
    {

    }
}

/*!
 * @brief     Handle USB device double buffer BULK IN transfer
 *
 * @param     usbdh: USB device handler
 *
 * @param     ep: ep handler
 *
 * @param     epStatus: ep register value
 *
 * @retval    None
 */
static void USBD_EP_DB_Transmit(USBD_HANDLE_T* usbdh, USBD_ENDPOINT_INFO_T* ep, uint16_t epStatus)
{
    uint32_t length;
    uint16_t cnt;
    
    /* Buffer0 */
    if(epStatus & USBD_EP_BIT_TXDTOG)
    {
        cnt = USBD_EP_ReadTxCnt(usbdh->usbGlobal, ep->epNum);
        
        if(ep->bufLen > cnt)
        {
            ep->bufLen -= cnt;
        }
        else
        {
            ep->bufLen = 0;
        }
        
        if(ep->bufLen == 0)
        {
            /* Buffer0 */
            if(ep->epDir == 0)
            {
                USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            else
            {
                USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            
            /* Buffer1 */
            if(ep->epDir == 0)
            {
                USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            else
            {
                USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            
            USBD_DataInStageCallback(usbdh, ep->epNum);
            
            if(epStatus & USBD_EP_BIT_RXDTOG)
            {
                USBD_EP_ToggleRx(usbdh->usbGlobal, ep->epNum);
            }
        }
        else
        {
            if(epStatus & USBD_EP_BIT_RXDTOG)
            {
                USBD_EP_ToggleRx(usbdh->usbGlobal, ep->epNum);
            }
            
            if(ep->dbBufferFill == ENABLE)
            {
                ep->buffer += cnt;
                ep->bufCount += cnt;
                
                if(ep->dbBufferLen >= ep->mps)
                {
                    length = ep->mps;
                    ep->dbBufferLen -= length;
                }
                else if(ep->dbBufferLen == 0)
                {
                    length = cnt;
                    ep->dbBufferFill = DISABLE;
                }
                else
                {
                    length = ep->dbBufferLen;
                    ep->dbBufferLen = 0;
                    ep->dbBufferFill = DISABLE;
                }
                
                /* Buffer0 */
                if(ep->epDir == 0)
                {
                    USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, length);
                }
                else
                {
                    USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, length);
                }
                
                /* Write buffer to PMA */
                USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr0, ep->buffer, length);
            }
        }
    }
    /* Buffer1 */
    else
    {
        /* Buffer1 */
        cnt = USBD_EP_ReadRxCnt(usbdh->usbGlobal, ep->epNum);
        
        if(ep->bufLen >= cnt)
        {
            ep->bufLen -= cnt;
        }
        else
        {
            ep->bufLen = 0;
        }
        
        if(ep->bufLen == 0)
        {
            /* Buffer0 */
            if(ep->epDir == 0)
            {
                USBD_EP_SetBuffer0RxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            else
            {
                USBD_EP_SetTxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            
            /* Buffer1 */
            if(ep->epDir == 0)
            {
                USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            else
            {
                USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, 0);
            }
            
            USBD_DataInStageCallback(usbdh, ep->epNum);
            
            if((epStatus & USBD_EP_BIT_RXDTOG) == 0)
            {
                USBD_EP_ToggleRx(usbdh->usbGlobal, ep->epNum);
            }
        }
        else
        {
            if((epStatus & USBD_EP_BIT_RXDTOG) == 0)
            {
                USBD_EP_ToggleRx(usbdh->usbGlobal, ep->epNum);
            }
            
            if(ep->dbBufferFill == ENABLE)
            {
                ep->buffer += cnt;
                ep->bufCount += cnt;
                
                if(ep->dbBufferLen >= ep->mps)
                {
                    length = ep->mps;
                    ep->dbBufferLen -= length;
                }
                else if(ep->dbBufferLen == 0)
                {
                    length = cnt;
                    ep->dbBufferFill = DISABLE;
                }
                else
                {
                    length = ep->dbBufferLen;
                    ep->dbBufferLen = 0;
                    ep->dbBufferFill = DISABLE;
                }
                
                /* Buffer1 */
                if(ep->epDir == 0)
                {
                    USBD_EP_SetRxCnt(usbdh->usbGlobal, ep->epNum, length);
                }
                else
                {
                    USBD_EP_SetBuffer0TxCnt(usbdh->usbGlobal, ep->epNum, length);
                }
                
                /* Write buffer to PMA */
                USBD_EP_WritePacketData(usbdh->usbGlobal, ep->pmaAddr1, ep->buffer, length);
            }
        }
    }
    
    /* Enable IN EP */
    USBD_EP_SetTxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_VALID);
}

/*!
 * @brief     Handle USB device double buffer BULK OUT transfer
 *
 * @param     usbdh: USB device handler
 *
 * @param     ep: ep handler
 *
 * @param     epStatus: ep register value
 *
 * @retval    None
 */
static uint16_t USBD_EP_DB_Receive(USBD_HANDLE_T* usbdh, USBD_ENDPOINT_INFO_T* ep, uint16_t epStatus)
{
    uint16_t cnt;
    
    /* Buffer0 */
    if(epStatus & USBD_EP_BIT_RXDTOG)
    {
        /* Buffer0 */
        cnt = USBD_EP_ReadTxCnt(usbdh->usbGlobal, ep->epNum);
        
        if(ep->bufLen >= cnt)
        {
            ep->bufLen -= cnt;
        }
        else
        {
            ep->bufLen = 0;
        }
        
        if(ep->bufLen == 0)
        {
            USBD_EP_SetRxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_NAK);
        }
        
        if(epStatus & USBD_EP_BIT_TXDTOG)
        {
            USBD_EP_ToggleTx(usbdh->usbGlobal, ep->epNum);
        }
        
        if(cnt)
        {
            USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                   ep->pmaAddr0, \
                                   ep->buffer, \
                                   cnt);
        }
    }
    /* Buffer1 */
    else
    {
        /* Buffer1 */
        cnt = USBD_EP_ReadRxCnt(usbdh->usbGlobal, ep->epNum);
        
        if(ep->bufLen >= cnt)
        {
            ep->bufLen -= cnt;
        }
        else
        {
            ep->bufLen = 0;
        }
        
        if(ep->bufLen == 0)
        {
            USBD_EP_SetRxStatus(usbdh->usbGlobal, ep->epNum, USBD_EP_STATUS_NAK);
        }
        
        if((epStatus & USBD_EP_BIT_TXDTOG) == 0)
        {
            USBD_EP_ToggleTx(usbdh->usbGlobal, ep->epNum);
        }
        
        if(cnt)
        {
            USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                   ep->pmaAddr1, \
                                   ep->buffer, \
                                   cnt);
        }
    }
    
    return cnt;
}

/*!
 * @brief     Handle USB device correct transfer interrupt
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
static void USBD_EP_CTRHandler(USBD_HANDLE_T* usbdh)
{
    USBD_ENDPOINT_INFO_T* ep;
    uint8_t epNum;
    uint8_t epDir;
    uint16_t epStatus;
    
    uint16_t bufCnt;
    uint16_t txBufCnt;
    
    while(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_CTR) == SET)
    {
        epNum = USBD_EP_ReadID(usbdh->usbGlobal);
        epDir = USBD_EP_ReadDir(usbdh->usbGlobal);
        
        /* EP0 */
        if(epNum == USBD_EP_0)
        {
            /* EP IN */
            if(epDir == 0)
            {
                USBD_EP_ResetTxFlag(usbdh->usbGlobal, USBD_EP_0);
                ep = &usbdh->epIN[USBD_EP_0];
                
                ep->bufCount = USBD_EP_ReadTxCnt(usbdh->usbGlobal, epNum);
                ep->buffer += ep->bufCount;
                
                /* IN stage */
                USBD_DataInStageCallback(usbdh, USBD_EP_0);
                
                if((ep->bufLen == 0) && (usbdh->address > 0))
                {
                    USBD_SetDeviceAddr(usbdh->usbGlobal, usbdh->address);
                    USBD_Enable(usbdh->usbGlobal);
                    usbdh->address = 0;
                }
            }
            else
            {
                ep = &usbdh->epOUT[USBD_EP_0];
                epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, USBD_EP_0);
                
                /* SETUP */
                if(epStatus & USBD_EP_BIT_SETUP)
                {
                    ep->bufCount = USBD_EP_ReadRxCnt(usbdh->usbGlobal, ep->epNum);
                    USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                           ep->pmaAddr, \
                                           (uint8_t *)usbdh->setup, \
                                           ep->bufCount);
                    
                    USBD_EP_ResetRxFlag(usbdh->usbGlobal, USBD_EP_0);
                    
                    USBD_SetupStageCallback(usbdh);
                }
                /* OUT or SETUP */
                else if(epStatus & USBD_EP_BIT_CTFR)
                {
                    USBD_EP_ResetRxFlag(usbdh->usbGlobal, USBD_EP_0);
                    
                    ep->bufCount = USBD_EP_ReadRxCnt(usbdh->usbGlobal, ep->epNum);
                    
                    if((ep->bufCount !=0) && (ep->buffer != 0))
                    {
                        USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                               ep->pmaAddr, \
                                               ep->buffer, \
                                               ep->bufCount);
                        
                        ep->buffer += ep->bufCount;
                        
                        USBD_DataOutStageCallback(usbdh, USBD_EP_0);
                    }
                    
                    epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, USBD_EP_0);
                    
                    if((epStatus & USBD_EP_BIT_SETUP) == 0)
                    {
                        USBD_EP_SetRxCnt(usbdh->usbGlobal, USBD_EP_0, ep->mps);
                        USBD_EP_SetRxStatus(usbdh->usbGlobal, USBD_EP_0, USBD_EP_STATUS_VALID);
                    }
                }
            }
        }
        else
        {
            epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, epNum);
            
            if(epStatus & USBD_EP_BIT_CTFR)
            {
                USBD_EP_ResetRxFlag(usbdh->usbGlobal, epNum);
                
                ep = &usbdh->epOUT[epNum];
                
                /* Single Buffer */
                if(ep->bufferStatus == USBD_EP_BUFFER_SINGLE)
                {
                    bufCnt = (uint16_t)USBD_EP_ReadRxCnt(usbdh->usbGlobal, epNum);
                    
                    if(bufCnt)
                    {
                        USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                               ep->pmaAddr, \
                                               ep->buffer, \
                                               bufCnt);
                    }
                }
                else
                {
                    /* DB bulk OUT */
                    if(ep->epType == EP_TYPE_BULK)
                    {
                        bufCnt = USBD_EP_DB_Receive(usbdh, ep, epStatus);
                    }
                    /* DB iso OUT */
                    else
                    {
                        USBD_EP_ToggleTx(usbdh->usbGlobal, ep->epNum);
                        
                        epStatus = USBD_EP_ReadStatus(usbdh->usbGlobal, epNum);
                        
                        if(epStatus & USBD_EP_BIT_RXDTOG)
                        {
                            /* Buffer0 */
                            bufCnt = (uint16_t)USBD_EP_ReadTxCnt(usbdh->usbGlobal, ep->epNum);
                            
                            if(bufCnt)
                            {
                                USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                                       ep->pmaAddr0, \
                                                       ep->buffer, \
                                                       bufCnt);
                            }
                        }
                        else
                        {
                            /* Buffer1 */
                            bufCnt = (uint16_t)USBD_EP_ReadRxCnt(usbdh->usbGlobal, ep->epNum);
                            
                            if(bufCnt)
                            {
                                USBD_EP_ReadPacketData(usbdh->usbGlobal, \
                                                       ep->pmaAddr1, \
                                                       ep->buffer, \
                                                       bufCnt);
                            }
                        }
                    }
                }
                
                /* Multi packets */
                ep->bufCount += bufCnt;
                ep->buffer += bufCnt;
                
                if((ep->bufLen == 0) || (bufCnt < ep->mps))
                {
                    USBD_DataOutStageCallback(usbdh, epNum);
                }
                else
                {
                    USBD_EP_XferStart(usbdh, ep);
                }
            }
            else if(epStatus & USBD_EP_BIT_CTFT)
            {
                USBD_EP_ResetTxFlag(usbdh->usbGlobal, epNum);
                
                ep = &usbdh->epIN[epNum];
                
                if((ep->epType == EP_TYPE_BULK) || \
                   (ep->epType == EP_TYPE_CONTROL) || \
                   ((ep->epType == EP_TYPE_INTERRUPT) && (epStatus & USBD_EP_BIT_KIND) == 0))
                {
                    txBufCnt = (uint16_t)USBD_EP_ReadTxCnt(usbdh->usbGlobal, ep->epNum);
                    
                    if(ep->bufLen > txBufCnt)
                    {
                        ep->bufLen -= txBufCnt;
                    }
                    else
                    {
                        ep->bufLen = 0;
                    }
                    
                    if(ep->bufLen == 0)
                    {
                        USBD_DataInStageCallback(usbdh, ep->epNum);
                    }
                    else
                    {
                        ep->buffer += txBufCnt;
                        ep->bufCount += txBufCnt;
                        USBD_EP_XferStart(usbdh, ep);
                    }
                }
                else
                {
                    USBD_EP_DB_Transmit(usbdh, ep, epStatus);
                }
            }
        }
    }
}

/*!
 * @brief     Handle USB device suspend status
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
static void USBD_SuspendHandler(USBD_HANDLE_T* usbdh)
{
    uint8_t i;
    uint16_t backupEP[8];
    
    for(i = 0; i < 8; i++)
    {
        backupEP[i] = (uint16_t)usbdh->usbGlobal->EP[i].EP;
    }
    
    USBD_SetForceReset(usbdh->usbGlobal);
    USBD_ResetForceReset(usbdh->usbGlobal);
    
    while (USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_RST) == RESET);
    
    USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_RST);
    
    for(i = 0; i < 8; i++)
    {
        usbdh->usbGlobal->EP[i].EP = backupEP[i];
    }
    
    USBD_SetForceSuspend(usbdh->usbGlobal);
    
    USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_SUS);
    
    /* Enter low power mode */
    USBD_SetLowerPowerMode(usbdh->usbGlobal);
}

/*!
 * @brief     USB device set device address
 *
 * @param     usbdh: USB device handler
 *
 * @param     address: address
 *
 * @retval    None
 */
void USBD_SetDevAddress(USBD_HANDLE_T* usbdh, uint8_t address)
{
    usbdh->address = address;

    if(address == 0)
    {
        USBD_SetDeviceAddr(usbdh->usbGlobal, address);
        USBD_Enable(usbdh->usbGlobal);
    }
}

/*!
 * @brief     USB device configure PMA
 *
 * @param     usbdh: USB device handler
 *
 * @param     epAddr: endpoint address
 *
 * @param     bufferStatus: endpoint kind
 *
 * @param     pmaAddr: PMA address
 *
 * @retval    None
 */
void USBD_ConfigPMA(USBD_HANDLE_T* usbdh, uint16_t epAddr, uint16_t bufferStatus, uint32_t pmaAddr)
{
    USBD_ENDPOINT_INFO_T *ep;
    
    if((epAddr & 0x80) == 0x80)
    {
        ep = &usbdh->epIN[epAddr & 0x07];
    }
    else
    {
        ep = &usbdh->epOUT[epAddr];
    }
    
    if(bufferStatus == USBD_EP_BUFFER_SINGLE)
    {
        ep->bufferStatus = USBD_EP_BUFFER_SINGLE;
        ep->pmaAddr = (uint16_t)pmaAddr;
    }
    else
    {
        ep->bufferStatus = USBD_EP_BUFFER_DOUBLE;
        ep->pmaAddr0 = (uint16_t)(pmaAddr & 0xFFFF);
        ep->pmaAddr1 = (uint16_t)((pmaAddr & 0xFFFF0000) >> 16);
    }
}

/*!
 * @brief     Config the USB device peripheral according to the specified parameters
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_Config(USBD_HANDLE_T* usbdh)
{
    uint8_t i;
    
    /* Init control endpoint structure */
    for (i = 0; i < usbdh->usbCfg.devEndpointNum; i++)
    {
        /* OUT control endpoint */
        usbdh->epOUT[i].epNum       = i;
        usbdh->epOUT[i].epDir       = EP_DIR_OUT;

        usbdh->epOUT[i].epType      = EP_TYPE_CONTROL;
        usbdh->epOUT[i].mps         = 0;
        usbdh->epOUT[i].buffer      = 0;
        usbdh->epOUT[i].bufLen      = 0;

        /* IN control endpoint */
        usbdh->epIN[i].epNum        = i;
        usbdh->epIN[i].epDir        = EP_DIR_IN;
        usbdh->epIN[i].txFifoNum    = i;

        usbdh->epIN[i].epType       = EP_TYPE_CONTROL;
        usbdh->epIN[i].mps          = 0;
        usbdh->epIN[i].buffer       = 0;
        usbdh->epIN[i].bufLen       = 0;
    }

    /* Init address */
    usbdh->address = 0;
    
    USBD_SetForceSuspend(usbdh->usbGlobal);
    
    usbdh->usbGlobal->CTRL = 0;
    
    usbdh->usbGlobal->INTSTS = 0;
    
    usbdh->usbGlobal->BUFFTB = USBD_BUFFTB_ADDR;
}

/*!
 * @brief     Handle USB device wakeup interrupt
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_WakeupIsrHandler(USBD_HANDLE_T* usbdh)
{
    EINT->IPEND = USBD_WAKEUP_EINT_LINE;
}

/*!
 * @brief     Handle USB device global interrupt
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
void USBD_IsrHandler(USBD_HANDLE_T* usbdh)
{
    /* Handle Correct Transfer */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_CTR))
    {
        USBD_EP_CTRHandler(usbdh);
    }
    
    /* Handle USB Reset interrupt */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_RST))
    {
        USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_RST);
        
        USBD_EnumDoneCallback(usbdh);
        
        USBD_SetDevAddress(usbdh, 0x00);
    }
    
    /* Handle Packet Memory Overflow */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_PMAOU))
    {
        USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_PMAOU);
    }
    
    /* Handle Failure Of Transfer */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_ERR))
    {
        USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_ERR);
    }
    
    /* Handle Wakeup Request */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_WKUP))
    {
        
        USBD_ResetLowerPowerMode(usbdh->usbGlobal);
        USBD_ResetForceSuspend(usbdh->usbGlobal);
        
        USBD_ResumeCallback(usbdh);
        
        USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_WKUP);
    }
    
    /* Handle Suspend Mode Request */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_SUS))
    {
        USBD_SuspendHandler(usbdh);
        
        USBD_SuspendCallback(usbdh);
    }
    
    /* Handle Start Of Frame */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_SOF))
    {
        USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_SOF);
        
        USBD_SOFCallback(usbdh);
    }
    
    /* Handle Expected Start of Frame */
    if(USBD_ReadIntFlag(usbdh->usbGlobal, USBD_INT_ESOF))
    {
        USBD_ClearIntFlag(usbdh->usbGlobal, USBD_INT_ESOF);
    }
}

/*!
 * @brief     USB device resume callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
__weak void USBD_ResumeCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/*!
 * @brief     USB device suspend callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
__weak void USBD_SuspendCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/*!
 * @brief     USB device enum done callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
__weak void USBD_EnumDoneCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/*!
 * @brief     USB device SETUP stage callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
__weak void USBD_SetupStageCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/*!
 * @brief     USB device data IN stage callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
__weak void USBD_DataInStageCallback(USBD_HANDLE_T* usbdh, uint8_t epNum)
{
    /* callback interface */
}

/*!
 * @brief     USB device data OUT stage callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
__weak void USBD_DataOutStageCallback(USBD_HANDLE_T* usbdh, uint8_t epNum)
{
    /* callback interface */
}

/*!
 * @brief       USB device SOF event callback function
 *
 * @param       usbhh: USB host handler.
 *
 * @retval      None
 */
__weak void USBD_SOFCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/*!
 * @brief     USB device ISO IN in complete callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
__weak void USBD_IsoInInCompleteCallback(USBD_HANDLE_T* usbdh, uint8_t epNum)
{
    /* callback interface */
}

/*!
 * @brief     USB device ISO OUT in complete callback
 *
 * @param     usbdh: USB device handler
 *
 * @param     epNum: endpoint number
 *
 * @retval    None
 */
__weak void USBD_IsoOutInCompleteCallback(USBD_HANDLE_T* usbdh, uint8_t epNum)
{
    /* callback interface */
}

/*!
 * @brief     USB device connect callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
__weak void USBD_ConnectCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/*!
 * @brief     USB device disconnect callback
 *
 * @param     usbdh: USB device handler
 *
 * @retval    None
 */
__weak void USBD_DisconnectCallback(USBD_HANDLE_T* usbdh)
{
    /* callback interface */
}

/**@} end of group USB_Device_Functions*/
/**@} end of group USB_Device_Driver*/
/**@} end of group APM32F10x_StdPeriphDriver*/
