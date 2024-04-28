#include "usb.h"
#include "usbd_cdc_if.h"
#include "qbuffer.h"


#define USBD_CDC_TX_BUF_LEN         1024
#define USBD_CDC_RX_BUF_LEN         1024




typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
} USBD_CDC_LineCodingTypeDef;

typedef  struct  usb_setup_req
{
  uint8_t   bmRequest;
  uint8_t   bRequest;
  uint16_t  wValue;
  uint16_t  wIndex;
  uint16_t  wLength;
} USBD_SetupReqTypedef;


const char *JUMP_BOOT_STR = "BOOT 5555AAAA";

static qbuffer_t q_rx;
static qbuffer_t q_tx;

static uint8_t q_rx_buf[2048];
static uint8_t q_tx_buf[2048];

static bool is_opened = false;
static bool is_rx_full = false;
static bool is_tx_req = false;
static uint8_t cdc_type = 0;


USBD_CDC_LineCodingTypeDef LineCoding =
    {
        115200,
        0x00,
        0x00,
        0x08
    };

uint8_t CDC_Reset_Status = 0;
uint8_t cdcTxBuffer[USBD_CDC_TX_BUF_LEN];
uint8_t cdcRxBuffer[USBD_CDC_RX_BUF_LEN];


extern USBD_INFO_T gUsbDeviceFS;



static USBD_STA_T USBD_FS_CDC_ItfInit(void);
static USBD_STA_T USBD_FS_CDC_ItfDeInit(void);
static USBD_STA_T USBD_FS_CDC_ItfCtrl(uint8_t command, uint8_t *buffer, uint16_t length);
static USBD_STA_T USBD_FS_CDC_ItfSend(uint8_t *buffer, uint16_t length);
static USBD_STA_T USBD_FS_CDC_ItfSendEnd(uint8_t epNum, uint8_t *buffer, uint32_t *length);
static USBD_STA_T USBD_FS_CDC_ItfReceive(uint8_t *buffer, uint32_t *length);




/* USB FS CDC interface handler */
USBD_CDC_INTERFACE_T USBD_CDC_INTERFACE_FS = {
  "CDC Interface FS",
  USBD_FS_CDC_ItfInit,
  USBD_FS_CDC_ItfDeInit,
  USBD_FS_CDC_ItfCtrl,
  USBD_FS_CDC_ItfSend,
  USBD_FS_CDC_ItfSendEnd,
  USBD_FS_CDC_ItfReceive,
};

USBD_STA_T USBD_FS_CDC_ItfInit(void)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_CDC_ConfigRxBuffer(&gUsbDeviceFS, cdcRxBuffer);
  USBD_CDC_ConfigTxBuffer(&gUsbDeviceFS, cdcTxBuffer, 0);

  return usbStatus;
}

USBD_STA_T USBD_FS_CDC_ItfDeInit(void)
{
  USBD_STA_T usbStatus = USBD_OK;

  return usbStatus;
}

USBD_STA_T USBD_FS_CDC_ItfCtrl(uint8_t command, uint8_t *buffer, uint16_t length)
{
  USBD_STA_T usbStatus = USBD_OK;
  uint8_t* pbuf = buffer;
  uint32_t bitrate;
  USBD_SetupReqTypedef *req = (USBD_SetupReqTypedef *)buffer;


  switch (command)
  {
    case USBD_CDC_SEND_ENCAPSULATED_COMMAND:

      break;
    case USBD_CDC_GET_ENCAPSULATED_RESPONSE:

      break;
    case USBD_CDC_SET_COMM_FEATURE:

      break;
    case USBD_CDC_GET_COMM_FEATURE:

      break;
    case USBD_CDC_CLEAR_COMM_FEATURE:

      break;

    case USBD_CDC_SET_LINE_CODING:
      bitrate   = (uint32_t)(pbuf[0]);
      bitrate  |= (uint32_t)(pbuf[1]<<8);
      bitrate  |= (uint32_t)(pbuf[2]<<16);
      bitrate  |= (uint32_t)(pbuf[3]<<24);
      LineCoding.format    = pbuf[4];
      LineCoding.paritytype= pbuf[5];
      LineCoding.datatype  = pbuf[6];
      LineCoding.bitrate   = bitrate - (bitrate%10);

      if( LineCoding.bitrate == 1200 )
      {
        CDC_Reset_Status = 1;
      }
      if (LineCoding.bitrate == 115200)
        cdc_type = 1;
      else
        cdc_type = 0;
      break;

    case USBD_CDC_GET_LINE_CODING:
      bitrate = LineCoding.bitrate | cdc_type;

      pbuf[0] = (uint8_t)(bitrate);
      pbuf[1] = (uint8_t)(bitrate>>8);
      pbuf[2] = (uint8_t)(bitrate>>16);
      pbuf[3] = (uint8_t)(bitrate>>24);
      pbuf[4] = LineCoding.format;
      pbuf[5] = LineCoding.paritytype;
      pbuf[6] = LineCoding.datatype;
      break;

    case USBD_CDC_SET_CONTROL_LINE_STATE:
      // TODO : 나중에 다른 터미널에서 문제 없는지 확인 필요
      //is_opened = req->wValue&0x01;  // 0 bit:DTR, 1 bit:RTS
      if (req->wValue & 0x01)
        is_opened = true;
      else
        is_opened = false;
        
      //logPrintf("CDC_SET_CONTROL_LINE_STATE %X\n", req->wValue);
      // if (cdc_type == 0 && LineCoding.bitrate > 57600)
      // {
      //   esp32RequestBoot(req->wValue);
      // }
      break;

    case USBD_CDC_SEND_BREAK:

      break;
    default:

      break;
  }
  return usbStatus;
}

USBD_STA_T USBD_FS_CDC_ItfSend(uint8_t *buffer, uint16_t length)
{
  USBD_STA_T usbStatus = USBD_OK;

  USBD_CDC_INFO_T *usbDevCDC = (USBD_CDC_INFO_T *)gUsbDeviceFS.devClass[gUsbDeviceFS.classID]->classData;

  if (usbDevCDC->cdcTx.state != USBD_CDC_XFER_IDLE)
  {
    return USBD_BUSY;
  }

  USBD_CDC_ConfigTxBuffer(&gUsbDeviceFS, buffer, length);

  usbStatus = USBD_CDC_TxPacket(&gUsbDeviceFS);

  return usbStatus;
}

USBD_STA_T USBD_FS_CDC_ItfSendEnd(uint8_t epNum, uint8_t *buffer, uint32_t *length)
{
  USBD_STA_T usbStatus = USBD_OK;

  is_tx_req = false;

  return usbStatus;
}

USBD_STA_T USBD_FS_CDC_ItfReceive(uint8_t *buffer, uint32_t *length)
{
  qbufferWrite(&q_rx, buffer, *length);

  if( CDC_Reset_Status == 1 )
  {
    CDC_Reset_Status = 0;

    if( *length >= 13 )
    {
      for(int i=0; i<13; i++ )
      {
        if( JUMP_BOOT_STR[i] != buffer[i] ) break;
      }
    }
  }

  uint32_t buf_len;

  buf_len = (q_rx.len - qbufferAvailable(&q_rx)) - 1;

  if (buf_len >= CDC_DATA_FS_MAX_PACKET_SIZE)
  {
    USBD_CDC_ConfigRxBuffer(&gUsbDeviceFS, &buffer[0]);
    USBD_CDC_RxPacket(&gUsbDeviceFS);
  }
  else
  {
    is_rx_full = true;
  }

  return USBD_OK;
}


bool cdcIfInit(void)
{
  is_opened = false;
  qbufferCreate(&q_rx, q_rx_buf, 2048);
  qbufferCreate(&q_tx, q_tx_buf, 2048);

  return true;
}

uint32_t cdcIfAvailable(void)
{
  return qbufferAvailable(&q_rx);
}

uint8_t cdcIfRead(void)
{
  uint8_t ret = 0;

  qbufferRead(&q_rx, &ret, 1);

  return ret;
}

uint32_t cdcIfWrite(uint8_t *p_data, uint32_t length)
{
  uint32_t pre_time;
  uint32_t tx_len;
  uint32_t buf_len;
  uint32_t sent_len;


  if (cdcIfIsConnected() != true) return 0;


  sent_len = 0;

  pre_time = millis();
  while(sent_len < length)
  {
    buf_len = (q_tx.len - qbufferAvailable(&q_tx)) - 1;
    tx_len = length - sent_len;

    if (tx_len > buf_len)
    {
      tx_len = buf_len;
    }

    if (tx_len > 0)
    {
      qbufferWrite(&q_tx, p_data, tx_len);
      p_data += tx_len;
      sent_len += tx_len;
    }
    else
    {
      delay(1);
    }
    
    if (cdcIfIsConnected() != true)
    {
      break;
    }

    if (millis()-pre_time >= 100)
    {
      break;
    }
  }

  return sent_len;
}

uint32_t cdcIfGetBaud(void)
{
  return LineCoding.bitrate;
}

bool cdcIfIsConnected(void)
{
  bool ret = true;

  if (gUsbDeviceFS.devClass == NULL)
  {
    return false;
  }
  if (is_opened == false)
  {
    ret = false;
  }
  if (gUsbDeviceFS.devState != USBD_DEV_CONFIGURE)
  {
    return false;
  }

  is_opened = ret;

  return ret;
}

uint8_t cdcIfGetType(void)
{
  return cdc_type;
}

uint8_t CDC_SoF_ISR(USBD_INFO_T *usbInfo)
{

  //-- RX
  //
  if (is_rx_full)
  {
    uint32_t buf_len;

    buf_len = (q_rx.len - qbufferAvailable(&q_rx)) - 1;

    if (buf_len >= CDC_DATA_FS_MAX_PACKET_SIZE)
    {
      USBD_CDC_ConfigRxBuffer(usbInfo, &cdcRxBuffer[0]);
      USBD_CDC_RxPacket(usbInfo);
      is_rx_full = false;
    }
  }


  //-- TX
  //
  uint32_t tx_len;
  tx_len = qbufferAvailable(&q_tx);
  if (tx_len > USBD_CDC_TX_BUF_LEN)
  {
    tx_len = USBD_CDC_TX_BUF_LEN;
  }

  if (tx_len%CDC_DATA_FS_MAX_PACKET_SIZE == 0)
  {
    if (tx_len > 0)
    {
      tx_len = tx_len - 1;
    }
  }

  if (tx_len > 0)
  {
    USBD_CDC_INFO_T *usbDevCDC = (USBD_CDC_INFO_T *)usbInfo->devClass[usbInfo->classID]->classData;

    if (usbDevCDC != NULL)
    {
      if (usbDevCDC->cdcTx.state == USBD_CDC_XFER_IDLE)
      {
        is_tx_req = true;
        qbufferRead(&q_tx, cdcTxBuffer, tx_len);

        USBD_CDC_ConfigTxBuffer(usbInfo, cdcTxBuffer, tx_len);
        USBD_CDC_TxPacket(usbInfo);
      }
    }
  }

  return 0;
}

