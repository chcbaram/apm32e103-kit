#ifndef ICM42670_H_
#define ICM42670_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_ICM42670



typedef struct
{
  
  int16_t acc_x; 
  int16_t acc_y; 
  int16_t acc_z; 
  int16_t gyro_x; 
  int16_t gyro_y; 
  int16_t gyro_z; 

  int16_t temp;

  uint32_t timestamp;
  uint16_t acc_scale;
  uint16_t gyro_scale;
} icm42670_info_t;


bool icm42670Init(void);
bool icm42670GetInfo(icm42670_info_t *p_info);


#endif

#ifdef __cplusplus
 }
#endif

#endif 
