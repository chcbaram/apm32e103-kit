#include "hal_i2c.h"



#define I2C_TIMEOUT_ADDR    (10000U)       /*!< 10 s  */
#define I2C_TIMEOUT_BUSY    (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_DIR     (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_RXNE    (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_STOPF   (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_TC      (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_TCR     (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_TXIS    (25U)          /*!< 25 ms */
#define I2C_TIMEOUT_FLAG    (25U)          /*!< 25 ms */





HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_T *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)
{
  bool ret = false;
  uint32_t pre_time;


  if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_BUSBSY))
  {
    return HAL_BUSY;
  }

  do
  {
    /* Generate Start */
    I2C_EnableGenerateStart(hi2c);

    pre_time = millis();
    while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_MODE_SELECT))  //EV5
    {
      if (millis()-pre_time >= Timeout)
      {
        return HAL_ERROR;
      }
    }

    /* Send address for write */
    I2C_Tx7BitAddress(hi2c, DevAddress, I2C_DIRECTION_TX);

    bool is_ready = true;
    while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))  //EV6
    {
      if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_AE))
      {
        I2C_ClearStatusFlag(hi2c, I2C_FLAG_AE);
        is_ready = false;
        break;
      }            
      if (millis()-pre_time >= Timeout)
      {
        is_ready = false;
        break;
      }
    }

    I2C_EnableGenerateStop(hi2c);
    pre_time = millis();
    while (I2C_ReadStatusFlag(hi2c, I2C_FLAG_BUSBSY))
    {
      if (millis()-pre_time >= Timeout)
      {
        is_ready = false; 
        break;
      }
    }    

    ret = is_ready;
  } while (0);

  if (ret)
    return HAL_OK;
  else
    return HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_BUSY(I2C_T *hi2c)
{
  uint32_t pre_time;


  pre_time = millis();
  while (I2C_ReadStatusFlag(hi2c, I2C_FLAG_BUSBSY))
  {
    if (millis()-pre_time >= I2C_TIMEOUT_BUSY)
      return HAL_ERROR;
  }    

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_START(I2C_T *hi2c)
{
  uint32_t pre_time;


  pre_time = millis();
  I2C_EnableAcknowledge(hi2c);
  I2C_EnableGenerateStart(hi2c);
  while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_MODE_SELECT))  
  {
    if (millis()-pre_time >= I2C_TIMEOUT_FLAG)
      return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_STOP(I2C_T *hi2c)
{
  uint32_t pre_time;


  pre_time = millis();  
  I2C_EnableGenerateStop(hi2c);
  while (I2C_ReadStatusFlag(hi2c, I2C_FLAG_BUSBSY))
  {
    if (millis()-pre_time >= I2C_TIMEOUT_STOPF)
    {
      return HAL_ERROR;
    }
  }    

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_ADDR_TX(I2C_T *hi2c, uint16_t dev_address)
{
  uint32_t pre_time;


  pre_time = millis();
  I2C_EnableAcknowledge(hi2c);
  I2C_Tx7BitAddress(hi2c, dev_address, I2C_DIRECTION_TX);
  while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))  
  {
    if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_AE))
    {
      I2C_ClearStatusFlag(hi2c, I2C_FLAG_AE);
      return HAL_ERROR;
    }            
    if (millis()-pre_time >= I2C_TIMEOUT_DIR)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_ADDR_RX(I2C_T *hi2c, uint16_t dev_address)
{
  uint32_t pre_time;


  pre_time = millis();
  I2C_EnableAcknowledge(hi2c);
  I2C_Tx7BitAddress(hi2c, dev_address, I2C_DIRECTION_RX);
  while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))  
  {
    if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_AE))
    {
      I2C_ClearStatusFlag(hi2c, I2C_FLAG_AE);
      return HAL_ERROR;
    }            
    if (millis()-pre_time >= I2C_TIMEOUT_DIR)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_DATA_TX(I2C_T *hi2c, uint8_t tx_data, bool ack, uint32_t pre_time, uint32_t timeout)
{
  if (ack)
    I2C_EnableAcknowledge(hi2c);
  else
    I2C_DisableAcknowledge(hi2c);

  I2C_TxData(hi2c, tx_data);

  while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_BYTE_TRANSMITTING))  
  {
    if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_AE))
    {
      I2C_ClearStatusFlag(hi2c, I2C_FLAG_AE);
      return HAL_ERROR;
    }            
    if (millis()-pre_time >= timeout)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_DATA_RX(I2C_T *hi2c, uint8_t *p_data, bool ack, uint32_t pre_time, uint32_t timeout)
{

  if (ack)
    I2C_EnableAcknowledge(hi2c);
  else
    I2C_DisableAcknowledge(hi2c);

  while (!I2C_ReadEventStatus(hi2c, I2C_EVENT_MASTER_BYTE_RECEIVED))  
  {
    if (millis()-pre_time >= timeout)
    {
      return HAL_TIMEOUT;
    }
  }

  *p_data = I2C_RxData(hi2c);

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_T *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                   uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  if ((pData == NULL) || (Size == 0U))
  {
    return  HAL_ERROR;
  }

  // BUSY
  //
  if (HAL_I2C_BUSY(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // START
  //
  if (HAL_I2C_START(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // ADDR
  //
  if (HAL_I2C_ADDR_TX(hi2c, DevAddress) != HAL_OK)
  {
    HAL_I2C_STOP(hi2c);
    return HAL_ERROR;
  }

  // MemAddress
  //
  uint32_t pre_time;

  pre_time = millis();
  if (MemAddSize == I2C_MEMADD_SIZE_8BIT)
  {
    if (HAL_I2C_DATA_TX(hi2c, MemAddress, true, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }
  }
  else
  {
    if (HAL_I2C_DATA_TX(hi2c, MemAddress>>8, true, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }
    if (HAL_I2C_DATA_TX(hi2c, MemAddress>>0, true, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }        
  }

  // RE-START
  //
  if (HAL_I2C_START(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // ADDR
  //
  if (HAL_I2C_ADDR_RX(hi2c, DevAddress) != HAL_OK)
  {
    HAL_I2C_STOP(hi2c);
    return HAL_ERROR;
  }

  for (int i=0; i<Size; i++)
  {
    bool ack = true;

    if ((i+1) == Size)
      ack = false;

    if (HAL_I2C_DATA_RX(hi2c, &pData[i], ack, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }
  }

  // STOP
  //
  if (HAL_I2C_STOP(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_T *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                   uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  if ((pData == NULL) || (Size == 0U))
  {
    return  HAL_ERROR;
  }

  // BUSY
  //
  if (HAL_I2C_BUSY(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // START
  //
  if (HAL_I2C_START(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // ADDR
  //
  if (HAL_I2C_ADDR_TX(hi2c, DevAddress) != HAL_OK)
  {
    HAL_I2C_STOP(hi2c);
    return HAL_ERROR;
  }

  // MemAddress
  //
  uint32_t pre_time;

  pre_time = millis();
  if (MemAddSize == I2C_MEMADD_SIZE_8BIT)
  {
    if (HAL_I2C_DATA_TX(hi2c, MemAddress, true, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }
  }
  else
  {
    if (HAL_I2C_DATA_TX(hi2c, MemAddress>>8, true, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }
    if (HAL_I2C_DATA_TX(hi2c, MemAddress>>0, true, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }        
  }

  for (int i=0; i<Size; i++)
  {
    bool ack = true;

    if ((i+1) == Size)
      ack = false;

    if (HAL_I2C_DATA_TX(hi2c, pData[i], ack, pre_time, Timeout) != HAL_OK)
    {
      HAL_I2C_STOP(hi2c);
      return HAL_ERROR;
    }
  }

  // STOP
  //
  if (HAL_I2C_STOP(hi2c) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}
