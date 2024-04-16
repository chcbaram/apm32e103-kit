#ifndef HAL_H_
#define HAL_H_

#ifdef __cplusplus
 extern "C" {
#endif


#include "hw_def.h"



typedef enum
{
  HAL_OK       = 0x00,
  HAL_ERROR    = 0x01,
  HAL_BUSY     = 0x02,
  HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;



#ifdef __cplusplus
}
#endif

#endif 