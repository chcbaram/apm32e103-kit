#ifndef MADGWICK_H_
#define MADGWICK_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "hw_def.h"


typedef struct
{
  float quat[4];

  float deg_roll;
  float deg_pitch;
  float deg_yaw;

  float rad_roll;
  float rad_pitch;
  float rad_yaw;
} madgwick_info_t;



bool madgwickInit(void);
void madgwickSetFreq(float freq_hz);
void madgwickSetFreqTime(float freq_time);
void madgwickUpdate(float gx, float gy, float gz, float ax, float ay, float az);
bool madgwickGetInfo(madgwick_info_t *p_info);

#ifdef __cplusplus
}
#endif

#endif