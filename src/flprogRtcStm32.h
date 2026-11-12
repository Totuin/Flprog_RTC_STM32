#ifndef FLPROG_RTC_STM32_H
#define FLPROG_RTC_STM32_H

/* STM32 RTC adapter for FLProg. HAL / STM32duino code is a dependency and is not modified. */
#include <stdbool.h>
#include "stm32_def.h"
#include "backup.h"
#include "clock.h"

#if defined(HAL_RTC_MODULE_ENABLED) && !defined(HAL_RTC_MODULE_ONLY)

#ifdef __cplusplus
extern "C"
{
#endif

  /* Формат часов, используемый при инициализации аппаратного RTC. */
  typedef enum
  {
    HOUR_FORMAT_12,
    HOUR_FORMAT_24
  } flprog_hourFormat_t;

  /* Половина суток при 12-часовом формате. */
  typedef enum
  {
    HOUR_AM,
    HOUR_PM
  } flprog_hourAM_PM_t;

  /* Битовая маска полей будильника (зарезервирована для интеграции). */
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

  /* Аппаратные каналы будильника, наличие ALARM_B зависит от STM32. */
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
#if !defined(RTC_PRER_PREDIV_A) || !defined(RTC_PRER_PREDIV_S)
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

/* Предделители RTC: для STM32F1 один коэффициент, для остальных два.
 * Значения -1 для новых STM32 означают автоматический подбор.
 * Настройка должна выполняться перед FLPROG_RTC_init(). */
#if defined(STM32F1xx)
  void FLPROG_RTC_getPrediv(uint32_t *asynch);
  void FLPROG_RTC_setPrediv(uint32_t asynch);
#else
void FLPROG_RTC_getPrediv(int8_t *asynch, int16_t *synch);
void FLPROG_RTC_setPrediv(int8_t asynch, int16_t synch);
#endif

  /* Инициализация RTC; reset=true сбрасывает backup domain и прежнее время!
   * Возвращает true, если календарь был заново инициализирован. */
  bool FLPROG_RTC_init(flprog_hourFormat_t format, sourceClock_t source, bool reset);

  /* Записать / прочитать время. Все числа в двоичном формате (не BCD).
   * subSeconds при записи пока не используется; при чтении — миллисекунды.
   * При ошибке HAL выходные указатели не меняются. */
  void FLPROG_RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, flprog_hourAM_PM_t period);
  void FLPROG_RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, flprog_hourAM_PM_t *period);

  /* Записать / прочитать дату: year — две последние цифры года,
   * wday — код дня недели, принятый в HAL. При ошибке чтения выходы не меняются. */
  void FLPROG_RTC_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t wday);
  void FLPROG_RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *wday);

#if defined(STM32F1xx)
  /* STM32F1: сохраняет дату в резервные регистры. */
  void FLPROG_RTC_StoreDate(void);
#endif
#ifdef __cplusplus
};
#endif

#endif
#endif
