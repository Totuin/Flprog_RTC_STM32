#include "flprogRtcStm32.h"
#include "stm32yyxx_ll_rtc.h"
#include <string.h> /* memset(), memcpy() */
#if defined(HAL_RTC_MODULE_ENABLED) && !defined(HAL_RTC_MODULE_ONLY)
#if defined(STM32MP1xx)
#error "RTC shall not be handled by Arduino in STM32MP1xx."
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  /* Единственный RTC-контроллер STM32. Не выделяет динамическую память. */
  static RTC_HandleTypeDef FLPROG_RtcHandle = {0};
  static sourceClock_t flprogClkSrc = LSI_CLOCK;
  static uint8_t FLPROG_HSEDiv = 0;
#if !defined(STM32F1xx)
  static int8_t flprogPredivAsync = -1;
  static int16_t flprogPredivSync = -1;
#else
static uint32_t flprogPrediv = RTC_AUTO_1_SECOND;
#endif
  static flprog_hourFormat_t flprogInitFormat = HOUR_FORMAT_12;
  /* Возвращаем false при ошибке, чтобы не обращаться к неготовому HAL. */
  static bool FLPROG_RTC_initClock(sourceClock_t source);
#if !defined(STM32F1xx)
  static bool FLPROG_RTC_computePrediv(int8_t *asynch, int16_t *synch);
#endif

  static bool FLPROG_RTC_initClock(sourceClock_t source)
  {
    /* В разных HAL имеются дополнительные поля; незаполненные биты запрещены. */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (source == LSE_CLOCK)
    {
      enableClock(LSE_CLOCK);
      PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
      if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
      {
        Error_Handler();
        return false;
      }
      flprogClkSrc = LSE_CLOCK;
    }
    else if (source == HSE_CLOCK)
    {
      enableClock(HSE_CLOCK);
      FLPROG_HSEDiv = 0;
      PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
#if defined(STM32F1xx)
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV128;
      FLPROG_HSEDiv = 128;
#define HSEDIV_MAX_GUARD 129U
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV32) && !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV32;
    FLPROG_HSEDiv = 32;
#define HSEDIV_MAX_GUARD 33U
#elif !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
#define HSEDIV_MAX_GUARD 17U
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
#define HSEDIV_MAX_GUARD 64U
#else
#define HSEDIV_MAX 32
#define HSESHIFT 16
#define HSEDIV_MAX_GUARD 32U
#endif
    /* При переборе отсекаем отсутствие допустимого делителя. */
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
      /* Не использовать RTCClockSelection, если подходящий делитель не найден. */
      if ((FLPROG_HSEDiv == 0U) ||
          (FLPROG_HSEDiv >= HSEDIV_MAX_GUARD) ||
          ((HSE_VALUE / FLPROG_HSEDiv) > HSE_RTC_MAX))
      {
        Error_Handler();
        return false;
      }

      if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
      {
        Error_Handler();
        return false;
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
        return false;
      }
      flprogClkSrc = LSI_CLOCK;
    }
    else
    {
      Error_Handler();
      return false;
    }
    return true;
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
  if ((asynch >= 0) && ((uint32_t)asynch <= PREDIVA_MAX) &&
      (synch >= 0) && ((uint32_t)synch <= PREDIVS_MAX))
  {
    flprogPredivAsync = asynch;
    flprogPredivSync = synch;
  }
  else
  {
    if (!FLPROG_RTC_computePrediv(&flprogPredivAsync, &flprogPredivSync))
      Error_Handler();
  }
}
#endif
#if defined(STM32F1xx)
  void FLPROG_RTC_getPrediv(uint32_t *asynch)
  {
    if (asynch != NULL)
    {
      flprogPrediv = LL_RTC_GetDivider(RTC);
      *asynch = flprogPrediv;
    }
  }
#else
void FLPROG_RTC_getPrediv(int8_t *asynch, int16_t *synch)
{
  if ((flprogPredivAsync == -1) || (flprogPredivSync == -1))
  {
    if (!FLPROG_RTC_computePrediv(&flprogPredivAsync, &flprogPredivSync))
    {
      Error_Handler();
      return;
    }
  }
  if (asynch != NULL) *asynch = flprogPredivAsync;
  if (synch != NULL) *synch = flprogPredivSync;
}
#endif
#if !defined(STM32F1xx)
  static bool FLPROG_RTC_computePrediv(int8_t *asynch, int16_t *synch)
  {
    uint32_t clk = 0;
    uint32_t predivS = 0;
    int divider;
    if ((asynch == NULL) || (synch == NULL)) return false;
    if (flprogClkSrc == LSE_CLOCK) clk = LSE_VALUE;
    else if (flprogClkSrc == LSI_CLOCK) clk = LSI_VALUE;
    else if (flprogClkSrc == HSE_CLOCK)
    {
      if (FLPROG_HSEDiv == 0U) return false; /* Не делить на ноль. */
      clk = HSE_VALUE / FLPROG_HSEDiv;
    }
    if (clk == 0U) return false;

    /* Ищем точное получение 1 Гц, начиная с максимального делителя. */
    for (divider = (int)PREDIVA_MAX; divider >= 0; --divider)
    {
      const uint32_t asynchDiv = (uint32_t)(divider + 1);
      if (clk < asynchDiv) continue;
      if ((clk % asynchDiv) == 0U)
      {
        predivS = clk / asynchDiv - 1U;
        if (predivS <= PREDIVS_MAX)
        {
          *asynch = (int8_t)divider;
          *synch = (int16_t)predivS;
          return true;
        }
      }
    }
    /* Когда точной комбинации нет, используем исходный округлённый вариант. */
    if (clk < (PREDIVA_MAX + 1U)) return false;
    predivS = clk / (PREDIVA_MAX + 1U) - 1U;
    if (predivS > PREDIVS_MAX) return false;
    *asynch = (int8_t)PREDIVA_MAX;
    *synch = (int16_t)predivS;
    return true;
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
#endif
      /* Сначала выбираем источник: делители LSE и LSI различаются. */
      if (!FLPROG_RTC_initClock(source)) return false;
#if !defined(STM32F1xx)
      /* HAL хранит оба делителя в uint32_t: нельзя записывать через int8_t*. */
      FLPROG_RTC_getPrediv(&asynch, &sync);
      if ((asynch < 0) || (sync < 0)) return false;
      FLPROG_RtcHandle.Init.AsynchPrediv = (uint32_t)asynch;
      FLPROG_RtcHandle.Init.SynchPrediv = (uint32_t)sync;
#endif
      if (HAL_RTC_Init(&FLPROG_RtcHandle) != HAL_OK)
      {
        Error_Handler();
        return false;
      }
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
        /* При смене RTC clock сохраняем дату/время; на плате эту ветку
         * нужно отдельно проверить, так как backup domain зависит от HAL. */
        FLPROG_RTC_GetDate(&years, &month, &days, &weekDay);
        FLPROG_RTC_GetTime(&hours, &minutes, &seconds, &subSeconds, &period);
#if defined(STM32F1xx)
        FLPROG_RTC_getPrediv(&asynch);
#else
      FLPROG_RTC_getPrediv(&asynch, &sync);
#endif
        if (!FLPROG_RTC_initClock(source)) return false;
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
        if (!FLPROG_RTC_initClock(source)) return false;
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
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    UNUSED(subSeconds); /* HAL_SetTime не устанавливает доли секунды. */
    if (flprogInitFormat == HOUR_FORMAT_24)
    {
      period = HOUR_AM;
    }
    if (((flprogInitFormat == HOUR_FORMAT_24) ? IS_RTC_HOUR24(hours) : IS_RTC_HOUR12(hours)) &&
        IS_RTC_MINUTES(minutes) && IS_RTC_SECONDS(seconds))
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
      RTC_TimeStruct.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
      RTC_TimeStruct.StoreOperation = RTC_STOREOPERATION_RESET;
#else
    UNUSED(period);
#endif

      if (HAL_RTC_SetTime(&FLPROG_RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN) != HAL_OK)
        Error_Handler();
    }
  }

  void FLPROG_RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, flprog_hourAM_PM_t *period)
  {
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    if ((hours != NULL) && (minutes != NULL) && (seconds != NULL))
    {
#if defined(STM32F1xx)
      uint8_t current_date = FLPROG_RtcHandle.DateToUpdate.Date;
#endif

      if (HAL_RTC_GetTime(&FLPROG_RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN) != HAL_OK)
        return; /* Не выдаём неинициализированные данные при ошибке HAL. */
#if !defined(STM32F1xx)
      /* В RTC с shadow registers чтение даты разблокирует календарь.
       * Иначе HAL_RTC_GetTime() может постоянно возвращать старое время. */
      RTC_DateTypeDef dummyDate = {0};
      if (HAL_RTC_GetDate(&FLPROG_RtcHandle, &dummyDate, RTC_FORMAT_BIN) != HAL_OK)
        return;
#endif
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
        /* HAL возвращает фактический SecondFraction; исключаем underflow. */
        const uint32_t denom = RTC_TimeStruct.SecondFraction + 1U;
        *subSeconds = (denom > 0U && RTC_TimeStruct.SubSeconds <= RTC_TimeStruct.SecondFraction)
                        ? ((RTC_TimeStruct.SecondFraction - RTC_TimeStruct.SubSeconds) * 1000U) / denom
                        : 0U;
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
    RTC_DateTypeDef RTC_DateStruct = {0};

    if (IS_RTC_YEAR(year) && IS_RTC_MONTH(month) && IS_RTC_DATE(day) && IS_RTC_WEEKDAY(wday))
    {
      RTC_DateStruct.Year = year;
      RTC_DateStruct.Month = month;
      RTC_DateStruct.Date = day;
      RTC_DateStruct.WeekDay = wday;
      if (HAL_RTC_SetDate(&FLPROG_RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN) != HAL_OK)
      {
        Error_Handler();
        return;
      }
#if defined(STM32F1xx)
      FLPROG_RTC_StoreDate();
#endif
    }
  }

  void FLPROG_RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *wday)
  {
    RTC_DateTypeDef RTC_DateStruct = {0};

    if ((year != NULL) && (month != NULL) && (day != NULL) && (wday != NULL))
    {
      if (HAL_RTC_GetDate(&FLPROG_RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN) != HAL_OK)
        return;
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
