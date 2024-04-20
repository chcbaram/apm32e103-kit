#ifndef _USBD_CDC_VCP_H_
#define _USBD_CDC_VCP_H_

#include "usbd_cdc.h"



#define CDC_DATA_HS_MAX_PACKET_SIZE 512U /* Endpoint IN & OUT Packet size */
#define CDC_DATA_FS_MAX_PACKET_SIZE 64U  /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE         8U   /* Control Endpoint Packet size */



extern USBD_CDC_INTERFACE_T USBD_CDC_INTERFACE_FS;




bool     cdcIfInit(void);
uint32_t cdcIfAvailable(void);
uint8_t  cdcIfRead(void);
uint32_t cdcIfGetBaud(void);
uint32_t cdcIfWrite(uint8_t *p_data, uint32_t length);
bool     cdcIfIsConnected(void);
uint8_t  cdcIfGetType(void);


#endif
