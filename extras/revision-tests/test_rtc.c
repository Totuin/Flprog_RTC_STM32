#include "flprogRtcStm32.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int mock_active=0, mock_errors=0, mock_gettime_calls=0, mock_getdate_calls=0, mock_init_calls=0, mock_init_fail=0, mock_gettime_fail=0, mock_getdate_fail=0, mock_settime_calls=0, mock_setdate_calls=0;
uint32_t mock_clock_source=RCC_RTCCLKSOURCE_LSI, mock_init_async=999, mock_init_sync=999;
RTC_TimeTypeDef mock_time = {10,11,12,0,0,0,127,255};
RTC_DateTypeDef mock_date = {26,11,12,4};
RCC_PeriphCLKInitTypeDef mock_clock_config;
static uint32_t backup_regs[20]={0};
void Error_Handler(void){mock_errors++;}
int HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef* r){assert(r->ExtraField==0);mock_clock_config=*r;mock_clock_source=r->RTCClockSelection;return HAL_OK;}
void enableClock(sourceClock_t s){(void)s;}
void enableBackupDomain(void){}
void resetBackupDomain(void){memset(backup_regs,0,sizeof(backup_regs));mock_active=0;}
uint32_t getBackupRegister(uint32_t r){return backup_regs[r];}
void setBackupRegister(uint32_t r,uint32_t v){backup_regs[r]=v;}
int HAL_RTC_Init(RTC_HandleTypeDef* h){mock_init_calls++;mock_init_async=h->Init.AsynchPrediv;mock_init_sync=h->Init.SynchPrediv;if(mock_init_fail)return HAL_ERROR;mock_active=1;return HAL_OK;}
int HAL_RTC_SetTime(RTC_HandleTypeDef* h,RTC_TimeTypeDef* t,uint32_t f){(void)h;(void)f; mock_settime_calls++; mock_time=*t;return HAL_OK;}
int HAL_RTC_GetTime(RTC_HandleTypeDef* h,RTC_TimeTypeDef* t,uint32_t f){(void)h;(void)f;mock_gettime_calls++;if(mock_gettime_fail)return HAL_ERROR;*t=mock_time;return HAL_OK;}
int HAL_RTC_SetDate(RTC_HandleTypeDef* h,RTC_DateTypeDef* d,uint32_t f){(void)f;mock_setdate_calls++;mock_date=*d;h->DateToUpdate=*d;return HAL_OK;}
int HAL_RTC_GetDate(RTC_HandleTypeDef* h,RTC_DateTypeDef* d,uint32_t f){(void)h;(void)f;mock_getdate_calls++;if(mock_getdate_fail)return HAL_ERROR;*d=mock_date;return HAL_OK;}
int HAL_RTCEx_EnableBypassShadow(RTC_HandleTypeDef* h){(void)h;return HAL_OK;}
int LL_RTC_IsActiveFlag_INITS(void* r){(void)r;return mock_active;}
uint32_t LL_RTC_GetDivider(void* r){(void)r;return 32767;}
void LL_RTC_SetAsynchPrescaler(void* r,uint32_t n){(void)r;(void)n;}
int main(void){
  assert(FLPROG_RTC_init(HOUR_FORMAT_24,LSE_CLOCK,true));
  assert(mock_errors==0 && mock_init_calls==1);
#ifndef STM32F1xx
  assert(mock_init_async == 127u && mock_init_sync == 255u);
  int8_t a=0;int16_t s=0;FLPROG_RTC_getPrediv(&a,&s);assert(a==127 && s==255);
  FLPROG_RTC_getPrediv(NULL,&s);assert(s==255);
#endif
  FLPROG_RTC_SetTime(12,30,45,0,HOUR_PM);
  assert(mock_settime_calls==1 && mock_time.Hours==12 && mock_time.Minutes==30);
#ifndef STM32F1xx
  assert(mock_time.DayLightSaving==RTC_DAYLIGHTSAVING_NONE && mock_time.StoreOperation==RTC_STOREOPERATION_RESET);
#endif
  const int oldSet=mock_settime_calls;
  FLPROG_RTC_SetTime(25,0,0,0,HOUR_AM);
  assert(mock_settime_calls==oldSet);
  mock_time=(RTC_TimeTypeDef){10,11,12,RTC_HOURFORMAT12_PM,0,0,127,255};
  uint8_t h=255,m=255,sec=255; uint32_t sub=9999; flprog_hourAM_PM_t period=HOUR_AM;
#ifndef STM32F1xx
  const int before_date=mock_getdate_calls;
#endif
  FLPROG_RTC_GetTime(&h,&m,&sec,&sub,&period);
  assert(h==10 && m==11 && sec==12);
#ifndef STM32F1xx
  assert(sub==500 && mock_getdate_calls==before_date+1);
  mock_getdate_fail=1;
  h=200;FLPROG_RTC_GetTime(&h,&m,&sec,&sub,&period);assert(h==200);mock_getdate_fail=0;
#endif
  mock_gettime_fail=1;h=201; FLPROG_RTC_GetTime(&h,&m,&sec,&sub,&period);assert(h==201);mock_gettime_fail=0;
  uint8_t y=255,mo=255,d=255,w=255;
  mock_getdate_fail=1;FLPROG_RTC_GetDate(&y,&mo,&d,&w); assert(y==255);mock_getdate_fail=0;
  FLPROG_RTC_SetDate(26,11,12,4); FLPROG_RTC_GetDate(&y,&mo,&d,&w);assert(y==26 && mo==11 && d==12 && w==4);
  /* При повторном запуске без reset сохранённый RTC не сбрасывается. */
  assert(!FLPROG_RTC_init(HOUR_FORMAT_24,LSE_CLOCK,false));
  assert(mock_init_calls==1);
#ifndef STM32F1xx
  mock_time=(RTC_TimeTypeDef){12,13,14,0,0,0,999,255};
  h=0;sub=900; FLPROG_RTC_GetTime(&h,&m,&sec,&sub,&period);
  assert(h==12 && sub==0);
#endif
  assert(!FLPROG_RTC_init(HOUR_FORMAT_24,(sourceClock_t)99,true));
  assert(mock_errors>=1);
  puts("RTC_STM32_MOCK_PASS");return 0;
}
