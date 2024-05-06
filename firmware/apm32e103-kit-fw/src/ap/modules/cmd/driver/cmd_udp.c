#include "cmd_udp.h"




#define CMD_UDP_RX_LENGTH     8*1024
#define CMD_UPD_RX_USE_Q      1


typedef struct
{
  char     ip_addr[32];
  uint32_t port;
} cmd_udp_args_t;


static bool open_(void *args);
static bool close_(void *args);  
static uint32_t available(void *args);
static bool flush(void *args);
static uint8_t read(void *args);
static uint32_t write(void *args, uint8_t *p_data, uint32_t length);  

static bool is_init = false;
static bool is_open = false;

static uint8_t   socket_id = HW_WIZNET_SOCKET_CMD;
static uint8_t   rx_buf[CMD_UDP_RX_LENGTH];
static qbuffer_t rx_q;


static uint8_t  dest_ip[4];
static uint16_t dest_port;
static bool     dest_update = false;






bool cmdUdpInitDriver(cmd_driver_t *p_driver, const char *ip_addr, uint32_t port)
{
  cmd_udp_args_t *p_args = (cmd_udp_args_t *)p_driver->args;


  qbufferCreate(&rx_q, rx_buf, CMD_UDP_RX_LENGTH);

  p_args->port  = port;
  strncpy(p_args->ip_addr, ip_addr, 32);

  p_driver->open      = open_;
  p_driver->close     = close_;
  p_driver->available = available;
  p_driver->flush     = flush;
  p_driver->read      = read;
  p_driver->write     = write;

  if (wiznetIsInit())
    is_init = true;
  else
    is_init = false;

  return true;
}

bool open_(void *args)
{
  bool   ret = false;
  int8_t socket_ret;
  cmd_udp_args_t *p_args = (cmd_udp_args_t *)args;

  if (!is_init)
    return false;

  socket_ret = socket(socket_id, Sn_MR_UDP, p_args->port, 0x00);
  if (socket_ret == socket_id)
  {
    ret = true;
  }

  logPrintf("[%s] cmdUdpOpen()\n", ret ? "OK":"E_");

  is_open = ret;
  return ret;
}

bool close_(void *args)
{
  if (is_open == false) return true;

  is_open = false;

  return true;  
}

uint32_t available(void *args)
{
  #if CMD_UPD_RX_USE_Q
  uint32_t       ret = 0;
  uint32_t       buf_size = 0;
  int32_t        recv_len;
  static uint8_t buf[1024];


  if (!is_init)
    return 0;

  if (getSn_SR(socket_id) == SOCK_UDP)
  {
    buf_size = getSn_RX_RSR(socket_id);
    buf_size = constrain(buf_size, 0, 1024);

    if (buf_size > 0)
    {
      recv_len = recvfrom(socket_id, buf, buf_size, dest_ip,(uint16_t*)&dest_port);
      if (recv_len > 0)
      {
        dest_update = true;
        qbufferWrite(&rx_q, buf, recv_len);
      }
    }
  }

  ret = qbufferAvailable(&rx_q);
  #else
  uint32_t ret = 0;

  if (getSn_SR(socket_id) == SOCK_UDP)
  {
    ret = getSn_RX_RSR(socket_id);
  }
  #endif

  return ret;
}

bool flush(void *args)
{
  uint32_t pre_time;

  pre_time = millis();
  while(available(args) > 0 && millis()-pre_time < 200)
  {
    read(args);
  }
  qbufferFlush(&rx_q);
  return true;
}

uint8_t read(void *args)
{
  #if CMD_UPD_RX_USE_Q
  uint8_t  ret;

  qbufferRead(&rx_q, &ret, 1);
  #else
  uint8_t ret;
  int32_t recv_len;

  recv_len = recvfrom(socket_id, &ret, 1, dest_ip,(uint16_t*)&dest_port);
  if (recv_len > 0)
  {
    dest_update = true;
  }
  #endif

  return ret;
}

uint32_t write(void *args, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;
  uint32_t pre_time;
  int32_t  socket_ret;

  if (is_init == false) 
    return 0;

  if (is_open == true && dest_update == true)
  {
    uint32_t tx_index;

   
    tx_index = 0;
    pre_time = millis();
    while(millis()-pre_time < 500)
    {
      socket_ret = sendto(socket_id, &p_data[tx_index], length-tx_index, dest_ip, dest_port);
      if (tx_index == length)
      {
        ret = tx_index;
        break;
      }
      if (socket_ret < 0)
      {
        break;
      }
      tx_index += socket_ret;
    }
  }

  return ret;
}
