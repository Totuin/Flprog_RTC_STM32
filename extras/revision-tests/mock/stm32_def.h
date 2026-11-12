#ifndef TEST_STM32_DEF_H
#define TEST_STM32_DEF_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define HAL_RTC_MODULE_ENABLED 1
#define HAL_OK 0
#define HAL_ERROR 1
#define UNUSED(x) ((void)(x))
#define RTC ((void*)0x40002800u)
#define RTC_FORMAT_BIN 0
#define RTC_OUTPUTSOURCE_NONE 0
#define RTC_OUTPUT_DISABLE 0
#define RTC_OUTPUT_POLARITY_HIGH 0
#define RTC_OUTPUT_TYPE_OPENDRAIN 0
#define RTC_OUTPUT_PULLUP_NONE 0
#define RTC_OUTPUT_REMAP_NONE 0
#define RTC_BINARY_NONE 0
#define RTC_HOURFORMAT_12 1
#define RTC_HOURFORMAT_24 0
#define RTC_HOURFORMAT12_PM 1
#define RTC_HOURFORMAT12_AM 0
#define RTC_DAYLIGHTSAVING_NONE 0
#define RTC_STOREOPERATION_RESET 0
#define RTC_ALARM_A 1
#define RTC_ALARM_B 2
#define RTC_CR_BYPSHAD 1
#define RTC_SSR_SS 1
#define RTC_PRER_PREDIV_S 0x00007FFFu
#define RTC_PRER_PREDIV_S_Pos 0u
#define RTC_PRER_PREDIV_A 0x007F0000u
#define RTC_PRER_PREDIV_A_Pos 16u
#define LSE_VALUE 32768u
#define LSI_VALUE 32000u
#ifndef HSE_VALUE
#define HSE_VALUE 8000000u
#endif
#define RCC_PERIPHCLK_RTC 1u
#define RCC_RTCCLKSOURCE_LSE 1u
#define RCC_RTCCLKSOURCE_LSI 2u
#define RCC_RTCCLKSOURCE_HSE_DIV128 3u
#if defined(MOCK_HSE_DIV31)
#define RCC_RTCCLKSOURCE_HSE_DIV31 10u
#else
#define RCC_RTCCLKSOURCE_HSE_DIV32 3u
#endif
#define RTC_AUTO_1_SECOND 32767u
#define RTC_BKP_DATE 6u
#define __HAL_RCC_RTC_ENABLE() ((void)0)
#define __HAL_RCC_RTCAPB_CLK_ENABLE() ((void)0)
#define IS_RTC_HOUR24(x) ((x)<=23)
#define IS_RTC_HOUR12(x) ((x)>=1 && (x)<=12)
#define IS_RTC_MINUTES(x) ((x)<=59)
#define IS_RTC_SECONDS(x) ((x)<=59)
#define IS_RTC_YEAR(x) ((x)<=99)
#define IS_RTC_MONTH(x) ((x)>=1 && (x)<=12)
#define IS_RTC_DATE(x) ((x)>=1 && (x)<=31)
#define IS_RTC_WEEKDAY(x) ((x)>=1 && (x)<=7)
#define RTC_WEEKDAY_MONDAY 1
#define RTC_WEEKDAY_TUESDAY 2
#define RTC_WEEKDAY_WEDNESDAY 3
#define RTC_WEEKDAY_THURSDAY 4
#define RTC_WEEKDAY_FRIDAY 5
#define RTC_WEEKDAY_SATURDAY 6
#define RTC_WEEKDAY_SUNDAY 7

typedef struct {uint8_t Year,Month,Date,WeekDay;} RTC_DateTypeDef;
typedef struct {uint8_t Hours,Minutes,Seconds,TimeFormat; uint32_t DayLightSaving,StoreOperation,SubSeconds,SecondFraction;} RTC_TimeTypeDef;
typedef struct {uint32_t AsynchPrediv,SynchPrediv,HourFormat,OutPut,OutPutPolarity,OutPutType,OutPutPullUp,OutPutRemap,BinMode;} RTC_InitTypeDef;
typedef struct {void* Instance; RTC_InitTypeDef Init; RTC_DateTypeDef DateToUpdate;} RTC_HandleTypeDef;
typedef struct {uint32_t PeriphClockSelection,RTCClockSelection,ExtraField;} RCC_PeriphCLKInitTypeDef;

typedef enum {LSI_CLOCK=1,LSE_CLOCK=2,HSE_CLOCK=3} sourceClock_t;
extern int mock_active, mock_errors, mock_gettime_calls, mock_getdate_calls, mock_init_calls, mock_init_fail, mock_gettime_fail, mock_getdate_fail, mock_settime_calls, mock_setdate_calls;
extern uint32_t mock_clock_source, mock_init_async, mock_init_sync;
extern RTC_TimeTypeDef mock_time;
extern RTC_DateTypeDef mock_date;
extern RCC_PeriphCLKInitTypeDef mock_clock_config;
void Error_Handler(void);
int HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef*);
int HAL_RTC_Init(RTC_HandleTypeDef*);
int HAL_RTC_SetTime(RTC_HandleTypeDef*,RTC_TimeTypeDef*,uint32_t);
int HAL_RTC_GetTime(RTC_HandleTypeDef*,RTC_TimeTypeDef*,uint32_t);
int HAL_RTC_SetDate(RTC_HandleTypeDef*,RTC_DateTypeDef*,uint32_t);
int HAL_RTC_GetDate(RTC_HandleTypeDef*,RTC_DateTypeDef*,uint32_t);
int HAL_RTCEx_EnableBypassShadow(RTC_HandleTypeDef*);
int LL_RTC_IsActiveFlag_INITS(void*);
uint32_t LL_RTC_GetDivider(void*);
void LL_RTC_SetAsynchPrescaler(void*,uint32_t);
#define __HAL_RCC_GET_RTC_SOURCE() mock_clock_source
#endif
