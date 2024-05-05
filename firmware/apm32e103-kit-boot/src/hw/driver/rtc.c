#include "rtc.h"



#ifdef _USE_HW_RTC
#include "cli.h"
#include <time.h>


#if CLI_USE(HW_RTC)
static void cliRtc(cli_args_t *args);
#endif

static bool is_init = false;





bool rtcInit(void)
{
  bool ret = false;


  RCM_EnableAPB1PeriphClock((RCM_APB1_PERIPH_T)(RCM_APB1_PERIPH_PMU | RCM_APB1_PERIPH_BAKR));
  PMU_EnableBackupAccess();
 
  RCM_ConfigLSE(RCM_LSE_OPEN);
  delay(10);

  RCM_ConfigRTCCLK(RCM_RTCCLK_LSE);
  RCM_EnableRTCCLK();

  RTC_WaitForSynchro();
  RTC_ConfigPrescaler(32767);
  RTC_WaitForLastTask();

  delay(10);
  ret = RCM_ReadStatusFlag(RCM_FLAG_LSERDY);

  logPrintf("[%s] rtcInit()\n", ret ? "OK":"NG");
  is_init = ret;

#if CLI_USE(HW_RTC)
  cliAdd("rtc", cliRtc);
#endif
  return ret;
}

bool rtcIsInit(void)
{
  return is_init;
}

bool rtcGetInfo(rtc_info_t *rtc_info)
{
  time_t    cur_time;
  struct tm timeinfo;

  cur_time = RTC_ReadCounter();

  gmtime_r(&cur_time, &timeinfo);

  rtc_info->time.hours   = timeinfo.tm_hour;
  rtc_info->time.minutes = timeinfo.tm_min;
  rtc_info->time.seconds = timeinfo.tm_sec;

  rtc_info->date.year  = (timeinfo.tm_year + 1900) % 100;
  rtc_info->date.month = timeinfo.tm_mon + 1;
  rtc_info->date.day   = timeinfo.tm_mday;
  rtc_info->date.week  = timeinfo.tm_wday;

  return true;
}

bool rtcGetTime(rtc_time_t *rtc_time)
{
  rtc_info_t rtc_info;


  rtcGetInfo(&rtc_info);


  rtc_time->hours   = rtc_info.time.hours;
  rtc_time->minutes = rtc_info.time.minutes;
  rtc_time->seconds = rtc_info.time.seconds;

  return true;
}

bool rtcGetDate(rtc_date_t *rtc_date)
{
  rtc_info_t rtc_info;


  rtcGetInfo(&rtc_info);

  rtc_date->year  = rtc_info.date.year;
  rtc_date->month = rtc_info.date.month;
  rtc_date->day   = rtc_info.date.day;
  rtc_date->week  = rtc_info.date.week;

  return true;
}

bool rtcSetTime(rtc_time_t *rtc_time)
{
  rtc_info_t rtc_info;


  rtcGetInfo(&rtc_info);

  rtc_info.time.hours   = rtc_time->hours;
  rtc_info.time.minutes = rtc_time->minutes;
  rtc_info.time.seconds = rtc_time->seconds;

  struct tm timeinfo;
  time_t    cur_time;

  memset(&timeinfo, 0, sizeof(timeinfo));
  timeinfo.tm_year  = (rtc_info.date.year + 2000) - 1900;
  timeinfo.tm_mon   = rtc_info.date.month - 1;
  timeinfo.tm_mday  = rtc_info.date.day;
  timeinfo.tm_hour  = rtc_info.time.hours;
  timeinfo.tm_min   = rtc_info.time.minutes;
  timeinfo.tm_sec   = rtc_info.time.seconds;

  cur_time = mktime(&timeinfo);

  RTC_ConfigCounter((uint32_t)cur_time);
  delay(10);

  return true;
}

bool rtcSetDate(rtc_date_t *rtc_date)
{
  rtc_info_t rtc_info;


  rtcGetInfo(&rtc_info);

  rtc_info.date.year  = rtc_date->year;
  rtc_info.date.month = rtc_date->month;
  rtc_info.date.day   = rtc_date->day;


  struct tm timeinfo;
  time_t    cur_time;

  memset(&timeinfo, 0, sizeof(timeinfo));
  timeinfo.tm_year  = (rtc_info.date.year + 2000) - 1900;
  timeinfo.tm_mon   = rtc_info.date.month - 1;
  timeinfo.tm_mday  = rtc_info.date.day;
  timeinfo.tm_hour  = rtc_info.time.hours;
  timeinfo.tm_min   = rtc_info.time.minutes;
  timeinfo.tm_sec   = rtc_info.time.seconds;

  cur_time = mktime(&timeinfo);

  RTC_ConfigCounter((uint32_t)cur_time);
  delay(10);

  return true;
}

bool rtcSetReg(uint32_t index, uint32_t data)
{
  BAKPR_ConfigBackupRegister((BAKPR_DATA_T)index, data);
  return true;
}

bool rtcGetReg(uint32_t index, uint32_t *p_data)
{
  *p_data = BAKPR_ReadBackupRegister((BAKPR_DATA_T)index);
  return true;
}


#if CLI_USE(HW_RTC)
void cliRtc(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("is_init : %d\n", is_init);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "get") && args->isStr(1, "info"))
  {
    rtc_info_t rtc_info;
    const char *week_str[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    while(cliKeepLoop())
    {
      rtcGetInfo(&rtc_info);

      cliPrintf("Y:%02d M:%02d D:%02d, H:%02d M:%02d S:%02d %s\n",
                rtc_info.date.year,
                rtc_info.date.month,
                rtc_info.date.day,
                rtc_info.time.hours,
                rtc_info.time.minutes,
                rtc_info.time.seconds,
                week_str[rtc_info.date.week]);
      delay(1000);
    }
    ret = true;
  }

  if (args->argc == 5 && args->isStr(0, "set") && args->isStr(1, "time"))
  {
    rtc_time_t rtc_time;

    rtc_time.hours = args->getData(2);
    rtc_time.minutes = args->getData(3);
    rtc_time.seconds = args->getData(4);

    rtcSetTime(&rtc_time);
    cliPrintf("H:%02d M:%02d S:%02d\n",
              rtc_time.hours,
              rtc_time.minutes,
              rtc_time.seconds);
    ret = true;
  }

  if (args->argc == 5 && args->isStr(0, "set") && args->isStr(1, "date"))
  {
    rtc_date_t rtc_date;

    rtc_date.year = args->getData(2);
    rtc_date.month = args->getData(3);
    rtc_date.day = args->getData(4);

    rtcSetDate(&rtc_date);
    cliPrintf("Y:%02d M:%02d D:%02d\n",
              rtc_date.year,
              rtc_date.month,
              rtc_date.day);
    ret = true;
  }


  if (ret == false)
  {
    cliPrintf("rtc info\n");
    cliPrintf("rtc get info\n");
    cliPrintf("rtc set time [h] [m] [s]\n");
    cliPrintf("rtc set date [y] [m] [d]\n");
  }
}
#endif

#endif