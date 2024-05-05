#include "imu.h"



#ifdef _USE_HW_IMU
#include "cli.h"
#include "imu/icm42670.h"
#include "imu/madgwick.h"



#if CLI_USE(HW_IMU)
static void cliCmd(cli_args_t *args);
#endif
static void imuComputeIMU( void );
static bool imuUpdateInfo(imu_info_t *p_info);

static uint32_t        update_hz = 100;
static uint32_t        update_us;
static bool            is_init   = false;
static imu_info_t      imu_info;
static madgwick_info_t filter_info;


bool imuInit(void)
{
  bool ret = true;


  imu_info.a_res = 2.0 / 32768.0;    // 2g
  imu_info.g_res = 2000.0 / 32768.0; // 2000dps
  update_us      = 1000000 / update_hz;

  is_init = imuBegin();
  ret = is_init;

  logPrintf("[%s] imuInit()\n", ret ? "OK":"E_");

#if CLI_USE(HW_IMU)
  cliAdd("imu", cliCmd);
#endif
  return ret;
}

bool imuIsInit(void)
{
  return is_init;
}

bool imuBegin(void)
{
  bool ret = true;

  ret &= icm42670Init();
  ret &= madgwickInit();

  madgwickSetFreq(update_hz);
  
  return ret;
}

bool imuGetInfo(imu_info_t *p_info)
{
  if (!is_init)
    return false;

  *p_info = imu_info;

  return true;
}

bool imuUpdate(void)
{
  bool ret;

  ret = imuUpdateInfo(&imu_info);
  return ret;
}

bool imuUpdateInfo(imu_info_t *p_info)
{
  bool ret = false;
  static uint32_t tTime;


  if( (micros()-tTime) >= update_us )
  {
    tTime = micros();

    imuComputeIMU();

    *p_info = imu_info;

    ret = true;
  }

  return ret;
}


#define FILTER_NUM    3

void imuComputeIMU( void )
{
  static uint32_t prev_process_time = 0;
  static uint32_t cur_process_time = 0;
  static uint32_t process_time = 0;
  icm42670_info_t sensor_info;
  int16_t gyro_offset = 15;


  if (!icm42670GetInfo(&sensor_info))
  {
    return;
  }

  if (micros() - prev_process_time > 100*1000)
  {
    prev_process_time = micros();
  }

  if (sensor_info.gyro_x > -gyro_offset && sensor_info.gyro_x < gyro_offset)
    sensor_info.gyro_x = 0;
  if (sensor_info.gyro_y > -gyro_offset && sensor_info.gyro_y < gyro_offset)
    sensor_info.gyro_y = 0;
  if (sensor_info.gyro_z > -gyro_offset && sensor_info.gyro_z < gyro_offset)
    sensor_info.gyro_z = 0;

  imu_info.ax = (float)sensor_info.acc_x*imu_info.a_res;
  imu_info.ay = (float)sensor_info.acc_y*imu_info.a_res;
  imu_info.az = (float)sensor_info.acc_z*imu_info.a_res;

  imu_info.gx = (float)sensor_info.gyro_x*imu_info.g_res;
  imu_info.gy = (float)sensor_info.gyro_y*imu_info.g_res;
  imu_info.gz = (float)sensor_info.gyro_z*imu_info.g_res;

  imu_info.a_raw[0] = sensor_info.acc_x;
  imu_info.a_raw[1] = sensor_info.acc_y;
  imu_info.a_raw[2] = sensor_info.acc_z;

  imu_info.g_raw[0] = sensor_info.gyro_x;
  imu_info.g_raw[1] = sensor_info.gyro_y;
  imu_info.g_raw[2] = sensor_info.gyro_z;

  cur_process_time  = micros();
  process_time      = cur_process_time-prev_process_time;
  prev_process_time = cur_process_time;

  madgwickSetFreqTime(process_time/1000000.0f);
  madgwickUpdate(imu_info.gx, imu_info.gy, imu_info.gz, imu_info.ax, imu_info.ay, imu_info.az);
  madgwickGetInfo(&filter_info);

  imu_info.roll  = filter_info.deg_roll;
  imu_info.pitch = filter_info.deg_pitch;
  imu_info.yaw   = filter_info.deg_yaw - 180;
}


#if CLI_USE(HW_IMU)
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("imu init : %d\n", is_init);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "acc"))
  {
    while (cliKeepLoop())
    {        
      if (imuUpdateInfo(&imu_info))
      {
        int x, y, z;

        x = imu_info.a_raw[0];
        y = imu_info.a_raw[1];
        z = imu_info.a_raw[2];
     
        cliPrintf("ax: %-6d ay: %-6d az: %-6d\n ", x, y, z);
      }
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "gyro"))
  {
    while (cliKeepLoop())
    {        
      if (imuUpdateInfo(&imu_info))
      {
        int x, y, z;

        x = imu_info.g_raw[0];
        y = imu_info.g_raw[1];
        z = imu_info.g_raw[2];
     
        cliPrintf("gx: %-6d gy: %-6d gz: %-6d\n ", x, y, z);
      }
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show"))
  {
    uint32_t data_count = 0;
    uint32_t data_rate = 0;
    uint32_t pre_count = 0;
    uint32_t pre_time = millis();


    pre_time = millis();
    while (cliKeepLoop())
    {        
      if (imuUpdateInfo(&imu_info))
      {
        int r, p, y;

        r = imu_info.roll * 1;
        p = imu_info.pitch * 1;
        y = imu_info.yaw * 1;

        data_count++;

        cliPrintf("%d\t Roll: %d\t Pitch: %d\t Yaw: %d\n ", data_rate, r, p, y);
      }

      if (millis() - pre_time >= 1000)
      {
        pre_time  = millis();
        data_rate = data_count - pre_count;
        pre_count = data_count;
      }
    }
    
    ret = true;
  }


  if (ret == false)
  {
    cliPrintf( "imu info\n");
    cliPrintf( "imu acc\n");
    cliPrintf( "imu gyro\n");
    cliPrintf( "imu show\n");
    cliPrintf( "imu graph\n");
  }
}

#endif





#endif
