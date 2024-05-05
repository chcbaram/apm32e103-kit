#include "can.h"


#ifdef _USE_HW_CAN
#include "qbuffer.h"
#include "cli.h"


#define CAN_RECOVERY_FAIL_CNT_MAX     6



typedef struct
{
  CAN_T         *h_can;
  CAN_Config_T  *p_cfg;
} can_hw_t;


typedef struct
{
  uint32_t            prescaler;
  CAN_SJW_T           sjw;
  CAN_TIME_SEGMENT1_T tseg1;
  CAN_TIME_SEGMENT2_T tseg2;
} can_baud_cfg_t;

const can_baud_cfg_t can_baud_cfg_60m_normal[] =
    {
        {40, CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 100K, 87.5%
        {30, CAN_SJW_4, CAN_TIME_SEGMENT1_13, CAN_TIME_SEGMENT2_2}, // 125K, 87.5%
        {15, CAN_SJW_4, CAN_TIME_SEGMENT1_13, CAN_TIME_SEGMENT2_2}, // 250K, 87.5%
        {8,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 500K, 87.5%
        {4,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 1M,   87.5%
    };

const can_baud_cfg_t can_baud_cfg_60m_data[] =
    {
        {40, CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 100K, 60%
        {32, CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 125K, 60%
        {16, CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 250K, 60%
        {8,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 500K, 60%
        {4,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 1M,   60%
        {2,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 2M    60%
        {1,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 4M    60%
        {1,  CAN_SJW_4, CAN_TIME_SEGMENT1_12, CAN_TIME_SEGMENT2_2}, // 5M    62.5%
    };

const can_baud_cfg_t *p_baud_normal = can_baud_cfg_60m_normal;
const can_baud_cfg_t *p_baud_data   = can_baud_cfg_60m_data;


const uint32_t dlc_len_tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

const uint32_t dlc_tbl[] =
    {
      0,
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      8,
      8,
      8,
      8,
      8,
      8
    };

static const CAN_MODE_T mode_tbl[] =
    {
        CAN_MODE_NORMAL,
        CAN_MODE_SILENT,
        CAN_MODE_LOOPBACK
    };


typedef struct
{
  bool is_init;
  bool is_open;

  uint32_t err_code;
  uint8_t  state;
  uint32_t recovery_cnt;

  uint32_t q_rx_full_cnt;
  uint32_t q_tx_full_cnt;
  uint32_t fifo_full_cnt;
  uint32_t fifo_lost_cnt;

  CAN_FILTER_FIFO_T fifo_idx;
  CAN_INT_T  enable_int;
  CanMode_t  mode;
  CanFrame_t frame;
  CanBaud_t  baud;
  CanBaud_t  baud_data;

  uint32_t rx_cnt;
  uint32_t tx_cnt;
  uint32_t pre_time;

  can_hw_t  *p_hw;
  bool (*handler)(uint8_t ch, CanEvent_t evt, can_msg_t *arg);

  qbuffer_t q_msg;
  can_msg_t can_msg[CAN_MSG_RX_BUF_MAX];
} can_tbl_t;

static can_tbl_t can_tbl[CAN_MAX_CH];

static volatile uint32_t err_int_cnt = 0;
static CAN_Config_T can1_cfg;

const static can_hw_t can_hw_tbl[CAN_MAX_CH] = 
  {
    {CAN1, &can1_cfg},
  };


#ifdef _USE_HW_CLI
static void cliCan(cli_args_t *args);
#endif

static void canErrUpdate(uint8_t ch);
static bool canInitHw(uint8_t ch);



bool canInit(void)
{
  bool ret = true;

  uint8_t i;


  for(i = 0; i < CAN_MAX_CH; i++)
  {
    can_tbl[i].is_init  = true;
    can_tbl[i].is_open  = false;
    can_tbl[i].err_code = CAN_ERR_NONE;
    can_tbl[i].state    = 0;
    can_tbl[i].recovery_cnt = 0;

    can_tbl[i].q_rx_full_cnt = 0;
    can_tbl[i].q_tx_full_cnt = 0;
    can_tbl[i].fifo_full_cnt = 0;
    can_tbl[i].fifo_lost_cnt = 0;
    can_tbl[i].p_hw          = (can_hw_t *)&can_hw_tbl[i];

    qbufferCreateBySize(&can_tbl[i].q_msg, (uint8_t *)&can_tbl[i].can_msg[0], sizeof(can_msg_t), CAN_MSG_RX_BUF_MAX);
  }



 logPrintf("[OK] canInit()\n");

#ifdef _USE_HW_CLI
  cliAdd("can", cliCan);
#endif
  return ret;
}

bool canInitHw(uint8_t ch)
{
  bool ret = true;
  GPIO_Config_T configStruct;


  switch(ch)
  {
    case _DEF_CAN1:
      /* Enable related Clock */
      RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_CAN1);
      RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);

      GPIO_ConfigPinRemap(GPIO_REMAP1_CAN1);
      configStruct.pin   = GPIO_PIN_9; // CAN1 TX remap 1
      configStruct.mode  = GPIO_MODE_AF_PP;
      configStruct.speed = GPIO_SPEED_50MHz;
      GPIO_Config(GPIOB, &configStruct);

      configStruct.pin  = GPIO_PIN_8;  // CAN1 RX remap 1
      configStruct.mode = GPIO_MODE_IN_PU;
      GPIO_Config(GPIOB, &configStruct);
      break;

    default:
      ret = false;
      break;
  }

  return ret;
}

bool canLock(void)
{
  bool ret = true;
  return ret;
}

bool canUnLock(void)
{
  return true;
}

bool canOpen(uint8_t ch, CanMode_t mode, CanFrame_t frame, CanBaud_t baud, CanBaud_t baud_data)
{
  bool ret = true;
  can_hw_t  *p_hw;


  if (ch >= CAN_MAX_CH) return false;


  p_hw = can_tbl[ch].p_hw;


  switch(ch)
  {
    case _DEF_CAN1:

      CAN_Reset(p_hw->h_can);

      canInitHw(ch);

      CAN_ConfigStructInit(p_hw->p_cfg);
      p_hw->p_cfg->autoBusOffManage = DISABLE;
      p_hw->p_cfg->autoWakeUpMode   = DISABLE;
      p_hw->p_cfg->nonAutoRetran    = ENABLE;
      p_hw->p_cfg->txFIFOPriority   = ENABLE;
      p_hw->p_cfg->mode             = mode_tbl[mode];
      p_hw->p_cfg->syncJumpWidth    = p_baud_normal[baud].sjw;
      p_hw->p_cfg->timeSegment1     = p_baud_normal[baud].tseg1;
      p_hw->p_cfg->timeSegment2     = p_baud_normal[baud].tseg2;
      p_hw->p_cfg->prescaler        = p_baud_normal[baud].prescaler;

      CAN_Config(p_hw->h_can, p_hw->p_cfg);

      can_tbl[ch].mode       = mode;
      can_tbl[ch].frame      = frame;
      can_tbl[ch].baud       = baud;
      can_tbl[ch].baud_data  = baud_data;
      can_tbl[ch].fifo_idx   = CAN_FILTER_FIFO_0;
      can_tbl[ch].enable_int = CAN_INT_F0MP
                             | CAN_INT_F0OVR
                             | CAN_INT_ERRW
                             | CAN_INT_ERRP
                             | CAN_INT_BOF
                             | CAN_INT_ERR
                             | CAN_INT_LEC;

      can_tbl[ch].err_code     = CAN_ERR_NONE;
      can_tbl[ch].recovery_cnt = 0;
      ret = true;
      break;
  }

  can_tbl[ch].is_open = false;

  if (ret != true)
  {
    return false;
  }

  canConfigFilter(ch, 0, CAN_STD, CAN_ID_MASK, 0x0000, 0x0000);  

  NVIC_EnableIRQRequest(USBD1_LP_CAN1_RX0_IRQn, 0, 0);
  CAN_EnableInterrupt(p_hw->h_can, can_tbl[ch].enable_int);

  can_tbl[ch].is_open = true;

  return ret;
}

bool canIsOpen(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return false;

  return can_tbl[ch].is_open;
}

void canClose(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return;

  if (can_tbl[ch].is_open)
  {
    can_tbl[ch].is_open = false;
    CAN_DisableInterrupt(can_tbl[ch].p_hw->h_can, 0xFFFFFFFF);
  }
  return;
}

bool canGetInfo(uint8_t ch, can_info_t *p_info)
{
  if(ch >= CAN_MAX_CH) return false;

  p_info->baud = can_tbl[ch].baud;
  p_info->baud_data = can_tbl[ch].baud_data;
  p_info->frame = can_tbl[ch].frame;
  p_info->mode = can_tbl[ch].mode;

  return true;
}

CanDlc_t canGetDlc(uint8_t length)
{
  CanDlc_t ret;

  if (length >= 64) 
    ret = CAN_DLC_64;
  else if (length >= 48)
    ret = CAN_DLC_48;
  else if (length >= 32)  
    ret = CAN_DLC_32;
  else if (length >= 24)  
    ret = CAN_DLC_24;
  else if (length >= 20)  
    ret = CAN_DLC_20;
  else if (length >= 16)  
    ret = CAN_DLC_16;
  else if (length >= 12)  
    ret = CAN_DLC_12;
  else if (length >= 8)  
    ret = CAN_DLC_8;
  else
    ret = (CanDlc_t)length;

  return ret;
}

uint8_t canGetLen(CanDlc_t dlc)
{
  return dlc_len_tbl[(int)dlc];
}

bool canConfigFilter(uint8_t ch, 
                     uint8_t index, 
                     CanIdType_t id_type, 
                     CanFilterType_t ft_type, 
                     uint32_t id, 
                     uint32_t id_mask)
{
  bool ret = false;
  CAN_FilterConfig_T sFilterConfig;

  if (ch >= CAN_MAX_CH) return false;
  // assert(index <= 13);
  // assert(ft_type <= CAN_ID_LIST);


  sFilterConfig.filterNumber = index;
  sFilterConfig.filterScale = CAN_FILTER_SCALE_32BIT;
  sFilterConfig.filterFIFO = can_tbl[ch].fifo_idx;
  sFilterConfig.filterActivation =  ENABLE;


  if (id_type == CAN_STD)
  {
    sFilterConfig.filterIdHigh     = ((     id <<  5) & 0xFFFF);
    sFilterConfig.filterIdLow      = 0;
    sFilterConfig.filterMaskIdHigh = ((id_mask <<  5) & 0xFFFF);
    sFilterConfig.filterMaskIdLow  = 0;
  }
  else
  {
    sFilterConfig.filterIdHigh     = ((     id >> 13) & 0xFFFF);
    sFilterConfig.filterIdLow      = ((     id <<  3) & 0xFFF8);
    sFilterConfig.filterMaskIdHigh = ((id_mask >> 13) & 0xFFFF);
    sFilterConfig.filterMaskIdLow  = ((id_mask <<  3) & 0xFFF8);
  }

  if (ft_type == CAN_ID_MASK)
    sFilterConfig.filterMode = CAN_FILTER_MODE_IDMASK;
  else
    sFilterConfig.filterMode = CAN_FILTER_MODE_IDLIST;


  CAN_ConfigFilter(can_tbl[ch].p_hw->h_can, &sFilterConfig);
  ret = true;


  return ret;
}

uint32_t canMsgAvailable(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return qbufferAvailable(&can_tbl[ch].q_msg);
}

bool canMsgInit(can_msg_t *p_msg, CanFrame_t frame, CanIdType_t  id_type, CanDlc_t dlc)
{
  p_msg->frame   = frame;
  p_msg->id_type = id_type;
  p_msg->dlc     = dlc;
  p_msg->length  = dlc_len_tbl[dlc];
  return true;
}

bool canMsgWrite(uint8_t ch, can_msg_t *p_msg, uint32_t timeout)
{
  CAN_T  *p_can;
  CAN_TxMessage_T tx_header;
  uint32_t pre_time;
  bool ret = true;


  if(ch > CAN_MAX_CH) return false;

  if (can_tbl[ch].err_code & CAN_ERR_BUS_OFF) return false;


  p_can = can_tbl[ch].p_hw->h_can;

  switch(p_msg->id_type)
  {
    case CAN_STD :
      tx_header.stdID  = p_msg->id & 0x7FF;
      tx_header.extID  = p_msg->id & 0x1FFFFFFF;
      tx_header.typeID = CAN_TYPEID_STD;
      break;

    case CAN_EXT :
      tx_header.stdID  = p_msg->id & 0x7FF;
      tx_header.extID  = p_msg->id & 0x1FFFFFFF;
      tx_header.typeID = CAN_TYPEID_EXT;
      break;
  }  
  tx_header.dataLengthCode = dlc_tbl[p_msg->dlc];
  tx_header.remoteTxReq = CAN_RTXR_DATA;

  memcpy(tx_header.data, p_msg->data, tx_header.dataLengthCode);

  pre_time = millis();
  /* Wait transmission complete */
  while (CAN_TxMessage(p_can, &tx_header) == 3)
  {
    if (millis() - pre_time >= timeout)
    {
      ret = false;
      break;
    }
#ifdef _USE_HW_RTOS
    osThreadYield();
#endif
  }

  return ret;
}

bool canMsgRead(uint8_t ch, can_msg_t *p_msg)
{
  bool ret = true;


  if(ch > CAN_MAX_CH) return 0;

  ret = qbufferRead(&can_tbl[ch].q_msg, (uint8_t *)p_msg, 1);

  return ret;
}

uint16_t canGetRxErrCount(uint8_t ch)
{
  uint16_t ret = 0;

  if(ch > CAN_MAX_CH) return 0;

  ret = (can_tbl[ch].p_hw->h_can->ERRSTS >> 24) & 0xFF;

  return ret;
}

uint16_t canGetTxErrCount(uint8_t ch)
{
  uint16_t ret = 0;

  if(ch > CAN_MAX_CH) return 0;

  ret = (can_tbl[ch].p_hw->h_can->ERRSTS >> 16) & 0xFF;

  return ret;
}

uint32_t canGetError(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return can_tbl[ch].err_code;
}

uint32_t canGetRxCount(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return can_tbl[ch].rx_cnt;
}

uint32_t canGetTxCount(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return can_tbl[ch].tx_cnt;
}

uint32_t canGetState(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return 0;

  return 0;
}

void canAttachRxInterrupt(uint8_t ch, bool (*handler)(uint8_t ch, CanEvent_t evt, can_msg_t *arg))
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].handler = handler;
}

void canDetachRxInterrupt(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].handler = NULL;
}

void canRecovery(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;
  if (can_tbl[ch].is_open != true) return;
  
  can_tbl[ch].err_code = CAN_ERR_NONE;
  can_tbl[ch].p_hw->h_can->ERRSTS = 0;
}

bool canIsRecoveryFail(uint8_t ch)
{
  return can_tbl[ch].recovery_cnt == CAN_RECOVERY_FAIL_CNT_MAX ? true:false;
}

bool canUpdate(void)
{
  enum
  {
    CAN_STATE_IDLE,
    CAN_STATE_WAIT
  };
  bool ret = false;
  can_tbl_t *p_can;


  for (int i=0; i<CAN_MAX_CH; i++)
  {
    p_can = &can_tbl[i];


    canErrUpdate(i);

    switch(p_can->state)
    {
      case CAN_STATE_IDLE:
        if (p_can->err_code & CAN_ERR_PASSIVE)
        {
          canRecovery(i);
          can_tbl[i].recovery_cnt++;
          can_tbl[i].pre_time = millis();
          p_can->state = CAN_STATE_WAIT;
          ret = true;
        }
        break;

      case CAN_STATE_WAIT:
        if ((p_can->err_code & CAN_ERR_PASSIVE) == 0)
        {
          p_can->state = CAN_STATE_IDLE;
        }
        if (millis()-can_tbl[i].pre_time >= 1000)
        {
          can_tbl[i].pre_time = millis();
          if (can_tbl[i].recovery_cnt < CAN_RECOVERY_FAIL_CNT_MAX)
          {
            canRecovery(i);
            can_tbl[i].recovery_cnt++;
          }
        }
        if (can_tbl[i].recovery_cnt == 0)
        {
          p_can->state = CAN_STATE_IDLE;
        }
        break;
    }
  }

  return ret;
}

void canRxFifoCallback(uint8_t ch)
{
  can_msg_t *rx_buf;
  CAN_RxMessage_T rx_header;


  rx_buf  = (can_msg_t *)qbufferPeekWrite(&can_tbl[ch].q_msg);

  CAN_RxMessage(can_tbl[ch].p_hw->h_can, can_tbl[ch].fifo_idx, &rx_header);
  {
    if(rx_header.typeID == CAN_TYPEID_STD)
    {
      rx_buf->id      = rx_header.stdID;
      rx_buf->id_type = CAN_STD;
    }
    else
    {
      rx_buf->id      = rx_header.extID;
      rx_buf->id_type = CAN_EXT;
    }
    rx_buf->length = rx_header.dataLengthCode;
    rx_buf->dlc = canGetDlc(rx_buf->length);
    rx_buf->frame = CAN_CLASSIC;
    rx_buf->timestamp = millis();

    memcpy(rx_buf->data, rx_header.data, rx_header.dataLengthCode);

    can_tbl[ch].rx_cnt++;

    if (qbufferWrite(&can_tbl[ch].q_msg, NULL, 1) != true)
    {
      can_tbl[ch].q_rx_full_cnt++;
    }

    if( can_tbl[ch].handler != NULL )
    {
      if ((*can_tbl[ch].handler)(ch, CAN_EVT_MSG, (void *)rx_buf) == true)
      {
        qbufferRead(&can_tbl[ch].q_msg, NULL, 1);
      }
    }
  }
}

void canErrClear(uint8_t ch)
{
  if(ch > CAN_MAX_CH) return;

  can_tbl[ch].err_code = CAN_ERR_NONE;
}

void canErrPrint(uint8_t ch)
{
  uint32_t err_code;


  if(ch > CAN_MAX_CH) return;

  err_code = can_tbl[ch].err_code;

  if (err_code & CAN_ERR_PASSIVE)   logPrintf("  ERR : CAN_ERR_PASSIVE\n");
  if (err_code & CAN_ERR_WARNING)   logPrintf("  ERR : CAN_ERR_WARNING\n");
  if (err_code & CAN_ERR_BUS_OFF)   logPrintf("  ERR : CAN_ERR_BUS_OFF\n");
  if (err_code & CAN_ERR_BUS_FAULT) logPrintf("  ERR : CAN_ERR_BUS_FAULT\n");
  if (err_code & CAN_ERR_ETC)       logPrintf("  ERR : CAN_ERR_BUS_ETC\n");
  if (err_code & CAN_ERR_MSG)       logPrintf("  ERR : CAN_ERR_BUS_MSG\n");

  logPrintf("  ESR : 0x%X\n", can_tbl[ch].p_hw->h_can->ERRSTS );
}

void canErrUpdate(uint8_t ch)
{
  CanEvent_t can_evt = CAN_EVT_NONE;

  /*
   * @param     flag: specifies the CAN flag.
   *                  This parameter can be one of the following values:
   *                  @arg CAN_FLAG_ERRW   : Error Warning Flag
   *                  @arg CAN_FLAG_ERRP   : Error Passive Flag
   *                  @arg CAN_FLAG_BOF    : Bus-Off Flag
   *                  @arg CAN_FLAG_LERRC  : Last error record code Flag
   *                  @arg CAN_FLAG_WUPI   : Wake up Flag
   *                  @arg CAN_FLAG_SLEEP  : Sleep acknowledge Flag
   *                  @arg CAN_FLAG_F0MP   : FIFO 0 Message Pending Flag
   *                  @arg CAN_FLAG_F0FULL : FIFO 0 Full Flag
   *                  @arg CAN_FLAG_F0OVR  : FIFO 0 Overrun Flag
   *                  @arg CAN_FLAG_F1MP   : FIFO 1 Message Pending Flag
   *                  @arg CAN_FLAG_F1FULL : FIFO 1 Full Flag
   *                  @arg CAN_FLAG_F1OVR  : FIFO 1 Overrun Flag
   *                  @arg CAN_FLAG_REQC0  : Request MailBox0 Flag
   *                  @arg CAN_FLAG_REQC1  : Request MailBox1 Flag
   *                  @arg CAN_FLAG_REQC2  : Request MailBox2 Flag
   *
   * @retval    flag staus:  RESET or SET
   *
   */

  if (CAN_ReadStatusFlag(can_tbl[ch].p_hw->h_can, CAN_FLAG_ERRP))
  {
    can_tbl[ch].err_code |= CAN_ERR_PASSIVE;
    can_evt = CAN_EVT_ERR_PASSIVE;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_PASSIVE;
  }

  if (CAN_ReadStatusFlag(can_tbl[ch].p_hw->h_can, CAN_FLAG_ERRW))
  {
    can_tbl[ch].err_code |= CAN_ERR_WARNING;
    can_evt = CAN_EVT_ERR_WARNING;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_WARNING;
  }

  if (CAN_ReadStatusFlag(can_tbl[ch].p_hw->h_can, CAN_FLAG_BOF))
  {
    can_tbl[ch].err_code |= CAN_ERR_BUS_OFF;
    can_evt = CAN_EVT_ERR_BUS_OFF;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_BUS_OFF;
  }

  if (CAN_ReadStatusFlag(can_tbl[ch].p_hw->h_can, CAN_FLAG_LERRC))
  {
    can_tbl[ch].err_code |= CAN_ERR_MSG;
    can_evt = CAN_EVT_ERR_MSG;
  }
  else
  {
    can_tbl[ch].err_code &= ~CAN_ERR_MSG;
  }


  if( can_tbl[ch].handler != NULL)
  {
    if (can_evt != CAN_EVT_NONE)
    {
      (*can_tbl[ch].handler)(ch, can_evt, NULL);
    }
  }   
}

void canInfoPrint(uint8_t ch)
{
  can_tbl_t *p_can = &can_tbl[ch];

  #ifdef _USE_HW_CLI
  #define canPrintf   cliPrintf
  #else
  #define canPrintf   logPrintf
  #endif

  canPrintf("ch            : ");
  switch(ch)
  {
    case _DEF_CAN1:
      canPrintf("_DEF_CAN1\n");
      break;
    case _DEF_CAN2:
      canPrintf("_DEF_CAN2\n");
      break;
  }

  canPrintf("is_open       : ");
  if (p_can->is_open)
    canPrintf("true\n");
  else
    canPrintf("false\n");

  canPrintf("baud          : ");
  switch(p_can->baud)
  {
    case CAN_100K:
      canPrintf("100K\n");
      break;
    case CAN_125K:
      canPrintf("125K\n");
      break;
    case CAN_250K:
      canPrintf("250K\n");
      break;
    case CAN_500K:
      canPrintf("500K\n");
      break;
    case CAN_1M:
      canPrintf("1M\n");
      break;
    default:
      break;
  }

  canPrintf("baud data     : ");
  switch(p_can->baud_data)
  {
    case CAN_100K:
      canPrintf("100K\n");
      break;
    case CAN_125K:
      canPrintf("125K\n");
      break;
    case CAN_250K:
      canPrintf("250K\n");
      break;
    case CAN_500K:
      canPrintf("500K\n");
      break;
    case CAN_1M:
      canPrintf("1M\n");
      break;

    case CAN_2M:
      canPrintf("2M\n");
      break;  
    case CAN_4M:
      canPrintf("4M\n");
      break;          
    case CAN_5M:
      canPrintf("5M\n");
      break;      
  }

  canPrintf("mode          : ");
  switch(p_can->mode)
  {
    case CAN_NORMAL:
      canPrintf("NORMAL\n");
      break;
    case CAN_MONITOR:
      canPrintf("MONITOR\n");
      break;
    case CAN_LOOPBACK:
      canPrintf("LOOPBACK\n");
      break;
  }

  canPrintf("frame         : ");
  switch(p_can->frame)
  {
    case CAN_CLASSIC:
      canPrintf("CAN_CLASSIC\n");
      break;
    case CAN_FD_NO_BRS:
      canPrintf("CAN_FD_NO_BRS\n");
      break;
    case CAN_FD_BRS:
      canPrintf("CAN_FD_BRS\n");
      break;      
  }
}



void canErrorCallback(uint8_t ch)
{
  err_int_cnt++;

  canErrUpdate(ch);
}


void USBD1_LP_CAN1_RX0_IRQHandler(void)
{
  uint8_t ch = _DEF_CAN1;


  if (CAN_PendingMessage(can_tbl[ch].p_hw->h_can, can_tbl[ch].fifo_idx))
  {
    canRxFifoCallback(ch);
  }
  canErrorCallback(ch);
}




#ifdef _USE_HW_CLI
void cliCan(cli_args_t *args)
{
  bool ret = false;


   canLock();

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<CAN_MAX_CH; i++)
    {
      if (can_tbl[i].is_open == true)
      {
        canInfoPrint(i);
        cliPrintf("is_open       : %d\n", can_tbl[i].is_open);

        cliPrintf("q_rx_full_cnt : %d\n", can_tbl[i].q_rx_full_cnt);
        cliPrintf("q_tx_full_cnt : %d\n", can_tbl[i].q_tx_full_cnt);
        cliPrintf("fifo_full_cnt : %d\n", can_tbl[i].fifo_full_cnt);
        cliPrintf("fifo_lost_cnt : %d\n", can_tbl[i].fifo_lost_cnt);
        cliPrintf("rx error cnt  : %d\n", canGetRxErrCount(i));
        cliPrintf("tx error cnt  : %d\n", canGetTxErrCount(i));     
        cliPrintf("recovery cnt  : %d\n", can_tbl[i].recovery_cnt); 
        cliPrintf("err code      : 0x%X\n", can_tbl[i].err_code);
        canErrPrint(i);
        cliPrintf("\n");
      }
      else
      {
        cliPrintf("%d not open\n", i);
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "open"))
  {
    cliPrintf("ch    : 0~%d\n\n", CAN_MAX_CH - 1);
    cliPrintf("mode  : CAN_NORMAL\n");
    cliPrintf("        CAN_MONITOR\n");
    cliPrintf("        CAN_LOOPBACK\n\n");
    cliPrintf("frame : CAN_CLASSIC\n");
    cliPrintf("        CAN_FD_NO_BRS\n");
    cliPrintf("        CAN_FD_BRS\n\n");
    cliPrintf("baud  : CAN_100K\n");
    cliPrintf("        CAN_125K\n");
    cliPrintf("        CAN_250K\n");
    cliPrintf("        CAN_500K\n");
    cliPrintf("        CAN_1M\n");
    cliPrintf("        CAN_2M\n");
    cliPrintf("        CAN_4M\n");
    cliPrintf("        CAN_5M\n");
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "open") && args->isStr(1, "test"))
  {
    bool can_ret;

    can_ret = canOpen(_DEF_CAN1, CAN_NORMAL, CAN_CLASSIC, CAN_1M, CAN_1M); 
    cliPrintf("canOpen() : %s\n", can_ret ? "True":"False");
    canInfoPrint(_DEF_CAN1);
    ret = true;
  }

  if (args->argc == 6 && args->isStr(0, "open"))
  {
    uint8_t ch;
    CanMode_t mode = CAN_NORMAL;
    CanFrame_t frame = CAN_CLASSIC;
    CanBaud_t baud = CAN_500K;
    CanBaud_t baud_data = CAN_500K;
    const char *mode_str[]  = {"CLAN_NORMAL", "CAN_MONITOR", "CAN_LOOPBACK"};
    const char *frame_str[] = {"CAN_CLASSIC", "CAN_FD_NO_BRS", "CAN_FD_BRS"};
    const char *baud_str[]  = {"CAN_100K", "CAN_125K", "CAN_250K", "CAN_500K", "CAN_1M", "CAN_2M", "CAN_4M", "CAN_5M"};

    ch = constrain(args->getData(1), 0, CAN_MAX_CH - 1); 

    for (int i=0; i<3; i++)
    {
      if (args->isStr(2, mode_str[i]))
      {
        mode = i;
        break;
      }
    }
    for (int i=0; i<3; i++)
    {
      if (args->isStr(3, frame_str[i]))
      {
        frame = i;
        break;
      }
    }
    for (int i=0; i<8; i++)
    {
      if (args->isStr(4, baud_str[i]))
      {
        baud = i;
        break;
      }
    }
    for (int i=0; i<8; i++)
    {
      if (args->isStr(5, baud_str[i]))
      {
        baud_data = i;
        break;
      }
    }

    bool can_ret;

    can_ret = canOpen(ch, mode, frame, baud, baud_data); 
    cliPrintf("canOpen() : %s\n", can_ret ? "True":"False");
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read_test"))
  {
    uint32_t index = 0;
    uint8_t ch;

    ch = constrain(args->getData(1), 0, CAN_MAX_CH - 1); 


    while(cliKeepLoop())
    {
      if (canMsgAvailable(ch))
      {
        can_msg_t msg;

        canMsgRead(ch, &msg);

        index %= 1000;
        cliPrintf("ch %d %03d(R) <- id ",ch, index++);
        if (msg.frame != CAN_CLASSIC)
        {
          cliPrintf("fd ");
        }
        else
        {
          cliPrintf("   ");
        }        
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "send_test"))
  {
    uint32_t pre_time;
    uint32_t index = 0;
    uint32_t err_code;
    uint8_t ch;
    CanFrame_t frame;
    uint32_t rx_err_cnt = 0;
    uint32_t tx_err_cnt = 0;

    ch = constrain(args->getData(1), 0, CAN_MAX_CH - 1); 

    if (args->isStr(2, "can"))
      frame = CAN_CLASSIC;
    else
      frame = CAN_FD_BRS;

    err_code = can_tbl[_DEF_CAN1].err_code;

    pre_time = millis();
    while(cliKeepLoop())
    {
      can_msg_t msg;

      if (millis()-pre_time >= 500)
      {
        pre_time = millis();

        msg.frame   = frame;
        msg.id_type = CAN_EXT;
        msg.dlc     = CAN_DLC_2;
        msg.id      = 0x314;
        msg.length  = 2;
        msg.data[0] = 1;
        msg.data[1] = 2;
        if (canMsgWrite(ch, &msg, 10) == true)
        {
          index %= 1000;
          cliPrintf("ch %d %03d(T) -> id ", ch, index++);
          if (msg.frame != CAN_CLASSIC)
          {
            cliPrintf("fd ");
          }
          else
          {
            cliPrintf("   ");
          }

          if (msg.id_type == CAN_STD)
          {
            cliPrintf("std ");
          }
          else
          {
            cliPrintf("ext ");
          }
          cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
          for (int i=0; i<msg.length; i++)
          {
            cliPrintf("0x%02X ", msg.data[i]);
          }
          cliPrintf("\n");
        }

        if (rx_err_cnt != canGetRxErrCount(ch) || tx_err_cnt != canGetTxErrCount(ch))
        {
          cliPrintf("ch %d ErrCnt : %d, %d\n", ch, canGetRxErrCount(ch), canGetTxErrCount(ch));
          rx_err_cnt = canGetRxErrCount(ch);
          tx_err_cnt = canGetTxErrCount(ch);
        }

        if (err_int_cnt > 0)
        {
          cliPrintf("ch %d Cnt : %d\n", ch, err_int_cnt);
          err_int_cnt = 0;
        }
      }

      if (can_tbl[ch].err_code != err_code)
      {
        cliPrintf("ch %d ErrCode : 0x%X\n", ch, can_tbl[ch].err_code);
        canErrPrint(ch);
        err_code = can_tbl[ch].err_code;
      }

      if (canUpdate())
      {
        cliPrintf("ch %d BusOff Recovery\n", ch);
      }


      if (canMsgAvailable(ch))
      {
        canMsgRead(ch, &msg);

        index %= 1000;
        cliPrintf("ch %d %03d(R) <- id ", ch, index++);
        if (msg.frame != CAN_CLASSIC)
        {
          cliPrintf("fd ");
        }
        else
        {
          cliPrintf("   ");
        }        
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "recovery"))
  {
    uint8_t ch;

    ch = constrain(args->getData(1), 0, CAN_MAX_CH - 1); 

    canRecovery(ch);
    ret = true;
  }

  canUnLock();

  if (ret == false)
  {
    cliPrintf("can info\n");
    cliPrintf("can open\n");
    cliPrintf("can open ch[0~%d] mode frame baud fd_baud\n", CAN_MAX_CH-1);    
    cliPrintf("can open test\n");
    cliPrintf("can read_test ch[0~%d]\n", CAN_MAX_CH-1);
    cliPrintf("can send_test ch[0~%d] can:fd\n", CAN_MAX_CH-1);
    cliPrintf("can recovery ch[0~%d]\n", CAN_MAX_CH-1);
  }
}
#endif

#endif