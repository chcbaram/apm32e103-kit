#ifndef HAL_I2C_H
#define HAL_I2C_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "hal.h"


#define I2C_MEMADD_SIZE_8BIT            (0x00000001U)
#define I2C_MEMADD_SIZE_16BIT           (0x00000002U)



HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_T *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_T *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                   uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_T *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                   uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_T *hi2c, uint16_t DevAddress, uint8_t *pData,
                                         uint16_t Size, uint32_t Timeout);

#ifdef __cplusplus
}
#endif

#endif 
