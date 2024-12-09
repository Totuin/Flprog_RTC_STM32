#include "flprogRtcStm32.h"
#include "stm32yyxx_ll_rtc.h"
#include <string.h>
#if defined(HAL_RTC_MODULE_ENABLED) && !defined(HAL_RTC_MODULE_ONLY)
#if defined(STM32MP1xx)
#error "RTC shall not be handled by Arduino in STM32MP1xx."
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  static RTC_HandleTypeDef FLPROG_RtcHandle = {0};
  static sourceClock_t flprogClkSrc = LSI_CLOCK;
  static uint8_t FLPROG_HSEDiv = 0;
#if !defined(STM32F1xx)
  static uint8_t flprogpPredivSync_bits = 0xFF;
  static int8_t flprogPredivAsync = -1;
  static int16_t flprogPredivSync = -1;
#else
static uint32_t flprogPrediv = RTC_AUTO_1_SECOND;
#endif
  static flprog_hourFormat_t flprogInitFormat = HOUR_FORMAT_12;
  static void FLPROG_RTC_initClock(sourceClock_t source);
#if !defined(STM32F1xx)
  static void FLPROG_RTC_computePrediv(int8_t *asynch, int16_t *synch);
#endif
  static inline int FLPROG_log2(int x)
  {
    return (x > 0) ? (sizeof(int) * 8 - __builtin_clz(x) - 1) : 0;
  }

  static void FLPROG_RTC_initClock(sourceClock_t source)
  {
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    if (source == LSE_CLOCK)
    {
      enableClock(LSE_CLOCK);
      PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
      if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
      {
        Error_Handler();
      }
      flprogClkSrc = LSE_CLOCK;
    }
    else if (source == HSE_CLOCK)
    {
      enableClock(HSE_CLOCK);
      PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
#if defined(STM32F1xx)
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV128;
      FLPROG_HSEDiv = 128;
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV32) && !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV32;
    FLPROG_HSEDiv = 32;
#elif !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    if ((HSE_VALUE / 2) <= HSE_RTC_MAX)
    {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV2;
      FLPROG_HSEDiv = 2;
    }
    else if ((HSE_VALUE / 4) <= HSE_RTC_MAX)
    {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV4;
      FLPROG_HSEDiv = 4;
    }
    else if ((HSE_VALUE / 8) <= HSE_RTC_MAX)
    {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV8;
      FLPROG_HSEDiv = 8;
    }
    else if ((HSE_VALUE / 16) <= HSE_RTC_MAX)
    {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV16;
      FLPROG_HSEDiv = 16;
    }
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV31)
#ifndef RCC_RTCCLKSOURCE_HSE_DIVX
#define RCC_RTCCLKSOURCE_HSE_DIVX 0x00000300U
#endif
#if defined(RCC_RTCCLKSOURCE_HSE_DIV63)
#define HSEDIV_MAX 64
#define HSESHIFT 12
#else
#define HSEDIV_MAX 32
#define HSESHIFT 16
#endif
    for (FLPROG_HSEDiv = 2; FLPROG_HSEDiv < HSEDIV_MAX; FLPROG_HSEDiv++)
    {
      if ((HSE_VALUE / FLPROG_HSEDiv) <= HSE_RTC_MAX)
      {
        PeriphClkInit.RTCClockSelection = (FLPROG_HSEDiv << HSESHIFT) | RCC_RTCCLKSOURCE_HSE_DIVX;
        break;
      }
    }
#else
#error "Could not define RTCClockSelection"
#endif
      if ((HSE_VALUE / FLPROG_HSEDiv) > HSE_RTC_MAX)
      {
        Error_Handler();
      }

      if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
      {
        Error_Handler();
      }
      flprogClkSrc = HSE_CLOCK;
    }
    else if (source == LSI_CLOCK)
    {
      enableClock(LSI_CLOCK);
      PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
      if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
      {
        Error_Handler();
      }
      flprogClkSrc = LSI_CLOCK;
    }
    else
    {
      Error_Handler();
    }
  }

#if defined(STM32F1xx)
  void FLPROG_RTC_setPrediv(uint32_t asynch)
  {
    flprogPrediv = asynch;
    LL_RTC_SetAsynchPrescaler(RTC, asynch);
  }
#else
void FLPROG_RTC_setPrediv(int8_t asynch, int16_t synch)
{
  if ((asynch >= -1) && ((uint32_t)asynch <= PREDIVA_MAX) &&
      (synch >= -1) && ((uint32_t)synch <= PREDIVS_MAX))
  {
    flprogPredivAsync = asynch;
    flprogPredivSync = synch;
  }
  else
  {
    FLPROG_RTC_computePrediv(&flprogPredivAsync, &flprogPredivSync);
  }
  flprogpPredivSync_bits = (uint8_t)FLPROG_log2(flprogPredivSync) + 1;
}
#endif
#if defined(STM32F1xx)
  void FLPROG_RTC_getPrediv(uint32_t *asynch)
  {
    flprogPrediv = LL_RTC_GetDivider(RTC);
    *asynch = flprogPrediv;
  }
#else
void FLPROG_RTC_getPrediv(int8_t *asynch, int16_t *synch)
{
  if ((flprogPredivAsync == -1) || (flprogPredivSync == -1))
  {
    FLPROG_RTC_computePrediv(&flprogPredivAsync, &flprogPredivSync);
  }
  if ((asynch != NULL) && (synch != NULL))
  {
    *asynch = flprogPredivAsync;
    *synch = flprogPredivSync;
  }
  flprogpPredivSync_bits = (uint8_t)FLPROG_log2(flprogPredivSync) + 1;
}
#endif
#if !defined(STM32F1xx)
  static void FLPROG_RTC_computePrediv(int8_t *asynch, int16_t *synch)
  {
    uint32_t predivS = PREDIVS_MAX + 1;
    uint32_t clk = 0;
    if ((asynch == NULL) || (synch == NULL))
    {
      return;
    }
    if (flprogClkSrc == LSE_CLOCK)
    {
      clk = LSE_VALUE;
    }
    else if (flprogClkSrc == LSI_CLOCK)
    {
      clk = LSI_VALUE;
    }
    else if (flprogClkSrc == HSE_CLOCK)
    {
      clk = HSE_VALUE / FLPROG_HSEDiv;
    }
    else
    {
      Error_Handler();
    }
    for (*asynch = PREDIVA_MAX; *asynch >= 0; (*asynch)--)
    {
      predivS = (clk / (*asynch + 1)) - 1;

      if (((predivS + 1) * (*asynch + 1)) == clk)
      {
        break;
      }
    }
    if ((predivS > PREDIVS_MAX) || (*asynch < 0))
    {
      *asynch = PREDIVA_MAX;
      predivS = (clk / (*asynch + 1)) - 1;
    }

    if (predivS > PREDIVS_MAX)
    {
      Error_Handler();
    }
    *synch = (int16_t)predivS;
  }
#endif

  bool FLPROG_RTC_init(flprog_hourFormat_t format, sourceClock_t source, bool reset)
  {
    bool reinit = false;
    flprog_hourAM_PM_t period = HOUR_AM;
    uint32_t subSeconds = 0;
    uint8_t seconds = 0, minutes = 0, hours = 0, weekDay = 0, days = 0, month = 0, years = 0;

#if defined(STM32F1xx)
    uint32_t asynch;
#else
  int8_t asynch;
  int16_t sync;
#endif

    flprogInitFormat = format;
    FLPROG_RtcHandle.Instance = RTC;
    enableBackupDomain();
    if (reset)
    {
      resetBackupDomain();
    }

#ifdef __HAL_RCC_RTCAPB_CLK_ENABLE
    __HAL_RCC_RTCAPB_CLK_ENABLE();
#endif
    __HAL_RCC_RTC_ENABLE();

#if defined(STM32F1xx)
    uint32_t BackupDate;
    BackupDate = getBackupRegister(RTC_BKP_DATE) << 16;
    BackupDate |= getBackupRegister(RTC_BKP_DATE + 1) & 0xFFFF;
    if ((BackupDate == 0) || reset)
    {
      FLPROG_RtcHandle.Init.AsynchPrediv = flprogPrediv;
      FLPROG_RtcHandle.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
#else
  if (!LL_RTC_IsActiveFlag_INITS(FLPROG_RtcHandle.Instance) || reset)
  {
    FLPROG_RtcHandle.Init.HourFormat = format == HOUR_FORMAT_12 ? RTC_HOURFORMAT_12 : RTC_HOURFORMAT_24;
    FLPROG_RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    FLPROG_RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    FLPROG_RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
#if defined(RTC_OUTPUT_PULLUP_NONE)
    FLPROG_RtcHandle.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
#endif
#if defined(RTC_OUTPUT_REMAP_NONE)
    FLPROG_RtcHandle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
#endif
#if defined(RTC_BINARY_NONE)
    FLPROG_RtcHandle.Init.BinMode = RTC_BINARY_NONE;
#endif
    FLPROG_RTC_getPrediv((int8_t *)&(FLPROG_RtcHandle.Init.AsynchPrediv), (int16_t *)&(FLPROG_RtcHandle.Init.SynchPrediv));
#endif
      FLPROG_RTC_initClock(source);

      HAL_RTC_Init(&FLPROG_RtcHandle);
      FLPROG_RTC_SetDate(1, 1, 1, 6);
      reinit = true;
    }
    else
    {
      uint32_t oldRtcClockSource = __HAL_RCC_GET_RTC_SOURCE();
      oldRtcClockSource = ((oldRtcClockSource == RCC_RTCCLKSOURCE_LSE) ? LSE_CLOCK : (oldRtcClockSource == RCC_RTCCLKSOURCE_LSI) ? LSI_CLOCK
                                                                                 :
#if defined(RCC_RTCCLKSOURCE_HSE_DIVX)
                                                                                 (oldRtcClockSource == RCC_RTCCLKSOURCE_HSE_DIVX) ? HSE_CLOCK
                                                                                                                                  :
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV32)
                                                                               (oldRtcClockSource == RCC_RTCCLKSOURCE_HSE_DIV32) ? HSE_CLOCK
                                                                                                                                 :
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV)
                                                                             (oldRtcClockSource == RCC_RTCCLKSOURCE_HSE_DIV) ? HSE_CLOCK
                                                                                                                             :
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV128)
                                                                             (oldRtcClockSource == RCC_RTCCLKSOURCE_HSE_DIV128) ? HSE_CLOCK
                                                                                                                                :
#endif

                                                                                                                                  0xFFFFFFFF);

#if defined(STM32F1xx)
      if ((FLPROG_RtcHandle.DateToUpdate.WeekDay == 0) && (FLPROG_RtcHandle.DateToUpdate.Month == 0) && (FLPROG_RtcHandle.DateToUpdate.Date == 0) && (FLPROG_RtcHandle.DateToUpdate.Year == 0))
      {
        memcpy(&FLPROG_RtcHandle.DateToUpdate, &BackupDate, 4);
      }
#endif
      if (source != oldRtcClockSource)
      {
        FLPROG_RTC_GetDate(&years, &month, &days, &weekDay);
        FLPROG_RTC_GetTime(&hours, &minutes, &seconds, &subSeconds, &period);
#if defined(STM32F1xx)
        FLPROG_RTC_getPrediv(&asynch);
#else
      FLPROG_RTC_getPrediv(&asynch, &sync);
#endif
        FLPROG_RTC_initClock(source);
        FLPROG_RTC_SetTime(hours, minutes, seconds, subSeconds, period);
        FLPROG_RTC_SetDate(years, month, days, weekDay);
#if defined(STM32F1xx)
        FLPROG_RTC_setPrediv(asynch);
#else
      FLPROG_RTC_setPrediv(asynch, sync);
#endif
      }
      else
      {
        FLPROG_RTC_initClock(source);
#if defined(STM32F1xx)
        memcpy(&FLPROG_RtcHandle.DateToUpdate, &BackupDate, 4);
        FLPROG_RTC_GetDate(&years, &month, &days, &weekDay);
        FLPROG_RTC_SetDate(FLPROG_RtcHandle.DateToUpdate.Year, FLPROG_RtcHandle.DateToUpdate.Month, FLPROG_RtcHandle.DateToUpdate.Date, FLPROG_RtcHandle.DateToUpdate.WeekDay);
#else
      FLPROG_RTC_getPrediv(NULL, NULL);
#endif
      }
    }

#if defined(RTC_CR_BYPSHAD)
    HAL_RTCEx_EnableBypassShadow(&FLPROG_RtcHandle);
#endif
    return reinit;
  }

  void FLPROG_RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, flprog_hourAM_PM_t period)
  {
    RTC_TimeTypeDef RTC_TimeStruct;
    UNUSED(subSeconds);
    if (flprogInitFormat == HOUR_FORMAT_24)
    {
      period = HOUR_AM;
    }
    if ((((flprogInitFormat == HOUR_FORMAT_24) && IS_RTC_HOUR24(hours)) || IS_RTC_HOUR12(hours)) && IS_RTC_MINUTES(minutes) && IS_RTC_SECONDS(seconds))
    {
      RTC_TimeStruct.Hours = hours;
      RTC_TimeStruct.Minutes = minutes;
      RTC_TimeStruct.Seconds = seconds;
#if !defined(STM32F1xx)
      if (period == HOUR_PM)
      {
        RTC_TimeStruct.TimeFormat = RTC_HOURFORMAT12_PM;
      }
      else
      {
        RTC_TimeStruct.TimeFormat = RTC_HOURFORMAT12_AM;
      }
      RTC_TimeStruct.DayLightSaving = RTC_STOREOPERATION_RESET;
      RTC_TimeStruct.StoreOperation = RTC_DAYLIGHTSAVING_NONE;
#else
    UNUSED(period);
#endif

      HAL_RTC_SetTime(&FLPROG_RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN);
    }
  }

  void FLPROG_RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, flprog_hourAM_PM_t *period)
  {
    RTC_TimeTypeDef RTC_TimeStruct;
    if ((hours != NULL) && (minutes != NULL) && (seconds != NULL))
    {
#if defined(STM32F1xx)
      uint8_t current_date = FLPROG_RtcHandle.DateToUpdate.Date;
#endif

      HAL_RTC_GetTime(&FLPROG_RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN);
      *hours = RTC_TimeStruct.Hours;
      *minutes = RTC_TimeStruct.Minutes;
      *seconds = RTC_TimeStruct.Seconds;
#if !defined(STM32F1xx)
      if (period != NULL)
      {
        if (RTC_TimeStruct.TimeFormat == RTC_HOURFORMAT12_PM)
        {
          *period = HOUR_PM;
        }
        else
        {
          *period = HOUR_AM;
        }
      }
#if defined(RTC_SSR_SS)
      if (subSeconds != NULL)
      {
        *subSeconds = ((flprogPredivSync - RTC_TimeStruct.SubSeconds) * 1000) / (flprogPredivSync + 1);
      }
#else
      UNUSED(subSeconds);
#endif
#else
    UNUSED(period);
    UNUSED(subSeconds);

    if (current_date != FLPROG_RtcHandle.DateToUpdate.Date)
    {
      FLPROG_RTC_StoreDate();
    }
#endif
    }
  }

  void FLPROG_RTC_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t wday)
  {
    RTC_DateTypeDef RTC_DateStruct;

    if (IS_RTC_YEAR(year) && IS_RTC_MONTH(month) && IS_RTC_DATE(day) && IS_RTC_WEEKDAY(wday))
    {
      RTC_DateStruct.Year = year;
      RTC_DateStruct.Month = month;
      RTC_DateStruct.Date = day;
      RTC_DateStruct.WeekDay = wday;
      HAL_RTC_SetDate(&FLPROG_RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN);
#if defined(STM32F1xx)
      FLPROG_RTC_StoreDate();
#endif
    }
  }

  void FLPROG_RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *wday)
  {
    RTC_DateTypeDef RTC_DateStruct;

    if ((year != NULL) && (month != NULL) && (day != NULL) && (wday != NULL))
    {
      HAL_RTC_GetDate(&FLPROG_RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN);
      *year = RTC_DateStruct.Year;
      *month = RTC_DateStruct.Month;
      *day = RTC_DateStruct.Date;
      *wday = RTC_DateStruct.WeekDay;
    }
  }
#if defined(STM32F1xx)
  void FLPROG_RTC_StoreDate(void)
  {
    uint32_t dateToStore;
    memcpy(&dateToStore, &FLPROG_RtcHandle.DateToUpdate, 4);
    setBackupRegister(RTC_BKP_DATE, dateToStore >> 16);
    setBackupRegister(RTC_BKP_DATE + 1, dateToStore & 0xffff);
  }
#endif

#ifdef __cplusplus
}
#endif

#endif
