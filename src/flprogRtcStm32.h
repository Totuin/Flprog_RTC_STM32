#ifndef __RTC_H
#define __RTC_H

#include <stdbool.h>
#include "stm32_def.h"
#include "backup.h"
#include "clock.h"

#if defined(HAL_RTC_MODULE_ENABLED) && !defined(HAL_RTC_MODULE_ONLY)

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  {
    HOUR_FORMAT_12,
    HOUR_FORMAT_24
  } flprog_hourFormat_t;

  typedef enum
  {
    HOUR_AM,
    HOUR_PM
  } flprog_hourAM_PM_t;

  typedef enum
  {
    OFF_MSK = 0,
    SS_MSK = 1,
    MM_MSK = 2,
    HH_MSK = 4,
    D_MSK = 8,
    M_MSK = 16,
    Y_MSK = 32
  } flprog_alarmMask_t;

  typedef enum
  {
    ALARM_A = RTC_ALARM_A,
#ifdef RTC_ALARM_B
    ALARM_B = RTC_ALARM_B
#endif
  } flprog_alarm_t;

  typedef void (*flprog_voidCallbackPtr)(void *);

#if defined(STM32F1xx)
#if !defined(RTC_BKP_DATE)
#define RTC_BKP_DATE LL_RTC_BKP_DR6
#endif
#endif

#ifndef RTC_IRQ_PRIO
#define RTC_IRQ_PRIO 2
#endif
#ifndef RTC_IRQ_SUBPRIO
#define RTC_IRQ_SUBPRIO 0
#endif

#define HSE_RTC_MAX 1000000U

#if !defined(STM32F1xx)
#if !defined(RTC_PRER_PREDIV_S) || !defined(RTC_PRER_PREDIV_S)
#error "Unknown Family - unknown synchronous prescaler"
#endif
#define PREDIVA_MAX (RTC_PRER_PREDIV_A >> RTC_PRER_PREDIV_A_Pos)
#define PREDIVS_MAX (RTC_PRER_PREDIV_S >> RTC_PRER_PREDIV_S_Pos)
#else
#define PREDIVA_MAX 0xFFFFFU
#endif
#if defined(STM32C0xx) || defined(STM32F0xx) || defined(STM32L0xx) || defined(STM32L5xx) || defined(STM32U5xx)
#define RTC_Alarm_IRQn RTC_IRQn
#define FLPROG_RTC_Alarm_IRQHandler RTC_IRQHandler
#endif
#if defined(STM32G0xx)
#define RTC_Alarm_IRQn RTC_TAMP_IRQn
#define FLPROG_RTC_Alarm_IRQHandler RTC_TAMP_IRQHandler
#endif

#if defined(STM32F1xx) || (defined(STM32F0xx) && defined(RTC_CR_WUTE)) || defined(STM32L0xx) || defined(STM32L5xx) || defined(STM32U5xx)
#define ONESECOND_IRQn RTC_IRQn
#elif defined(STM32MP1xx)
#define ONESECOND_IRQn RTC_WKUP_ALARM_IRQn
#elif defined(STM32G0xx)
#define ONESECOND_IRQn RTC_TAMP_IRQn
#elif defined(CORE_CM0PLUS) && (defined(STM32WL54xx) || defined(STM32WL55xx) || defined(STM32WL5Mxx))
#define ONESECOND_IRQn RTC_LSECSS_IRQn
#elif defined(RTC_CR_WUTE)
#define ONESECOND_IRQn RTC_WKUP_IRQn
#else

#endif

#if defined(STM32F1xx) && !defined(IS_RTC_WEEKDAY)

#define IS_RTC_WEEKDAY(WEEKDAY) (((WEEKDAY) == RTC_WEEKDAY_MONDAY) || ((WEEKDAY) == RTC_WEEKDAY_TUESDAY) || ((WEEKDAY) == RTC_WEEKDAY_WEDNESDAY) || ((WEEKDAY) == RTC_WEEKDAY_THURSDAY) || ((WEEKDAY) == RTC_WEEKDAY_FRIDAY) || ((WEEKDAY) == RTC_WEEKDAY_SATURDAY) || ((WEEKDAY) == RTC_WEEKDAY_SUNDAY))

#define IS_RTC_HOUR12(HOUR) IS_RTC_HOUR24(HOUR)
#endif

#if defined(STM32F1xx)
  void FLPROG_RTC_getPrediv(uint32_t *asynch);
  void FLPROG_RTC_setPrediv(uint32_t asynch);
#else
void FLPROG_RTC_getPrediv(int8_t *asynch, int16_t *synch);
void FLPROG_RTC_setPrediv(int8_t asynch, int16_t synch);
#endif

  bool FLPROG_RTC_init(flprog_hourFormat_t format, sourceClock_t source, bool reset);

  void FLPROG_RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, flprog_hourAM_PM_t period);
  void FLPROG_RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, flprog_hourAM_PM_t *period);

  void FLPROG_RTC_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t wday);
  void FLPROG_RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *wday);

  void FLPROG_RTC_StartAlarm(flprog_alarm_t name, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, flprog_hourAM_PM_t period, uint8_t mask);
  bool FLPROG_RTC_IsAlarmSet(flprog_alarm_t name);
  void FLPROG_RTC_GetAlarm(flprog_alarm_t name, uint8_t *day, uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, flprog_hourAM_PM_t *period, uint8_t *mask);

#if defined(STM32F1xx)
  void FLPROG_RTC_StoreDate(void);
#endif
#ifdef __cplusplus
};
#endif

#endif
#endif
