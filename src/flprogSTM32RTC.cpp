#include "flprogSTM32RTC.h"

FLProgSystemRTC::FLProgSystemRTC(int16_t gmt)
{
  setGmt(gmt);
  RTC_init(HOUR_FORMAT_24, LSE_CLOCK, false);
  readTime();
}

void FLProgSystemRTC::pool()
{
  if (flprog::isTimer(_startReadTime, _reqestPeriod))
  {
    readTime();
  }
  // else
  // {
  //   calculationTime();
  // }
}

void FLProgSystemRTC::privateSetTime()
{
  RTC_SetTime(now.getHour(), now.getMinute(), now.getSecond(), 0, HOUR_AM);
  RTC_SetDate(uint8_t(now.getYear() - 2000), now.getMonth(), now.getDate(), now.getDay());
}

void FLProgSystemRTC::readTime()
{
  uint8_t year, month, data, day, hour, minute, second;
  uint32_t subSec;
  hourAM_PM_t period;
  RTC_GetDate(&year, &month, &data, &day);
  RTC_GetTime(&hour, &minute, &second, &subSec, &period);
  now.setTime(second, minute, hour, data, month, year);
  _startReadTime = millis();
}
