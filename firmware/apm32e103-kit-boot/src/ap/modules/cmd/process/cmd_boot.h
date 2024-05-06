#ifndef CMD_BOOT_H_
#define CMD_BOOT_H_


#include "ap_def.h"


typedef struct
{
  bool     is_begin;
  char     fw_name[64];
  uint32_t fw_size;
  uint32_t fw_receive_size;
} cmd_boot_info_t;

bool cmdBootInit(void);
bool cmdBootIsBusy(void);
bool cmdBootProcess(cmd_t *p_cmd);
void cmdBootGetInfo(cmd_boot_info_t *p_info);


#endif