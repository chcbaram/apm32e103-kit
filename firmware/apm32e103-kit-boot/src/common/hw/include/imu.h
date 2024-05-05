#ifndef IMU_H_
#define IMU_H_



#ifdef __cplusplus
 extern "C" {
#endif


#include "hw_def.h"

#ifdef _USE_HW_IMU

#include "bsp.h"


typedef struct
{
  float quat[4];

  float ax, ay, az;
  float gx, gy, gz;

  float pitch, roll, yaw;
  float a_res;
  float g_res;

  int16_t a_raw[3];
  int16_t g_raw[3];
} imu_info_t;

bool imuInit(void);
bool imuIsInit(void);
bool imuBegin(void);
bool imuUpdate(void);
bool imuGetInfo(imu_info_t *p_info);


#ifdef __cplusplus
}
#endif

#endif

#endif
