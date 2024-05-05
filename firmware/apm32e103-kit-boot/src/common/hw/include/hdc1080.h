#ifndef HDC1080_H_
#define HDC1080_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"

#ifdef _USE_HW_HDC1080



typedef struct
{
  int16_t temp;
  int16_t humidity;

  int16_t temp_x100;
  int16_t humidity_x100;
} hdc1080_info_t;


bool hdc1080Init(void);
bool hdc1080IsInit(void);
bool hdc1080GetInfo(hdc1080_info_t *p_info);

#endif

#ifdef __cplusplus
}
#endif

#endif
