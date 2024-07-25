#pragma once
#include "Arduino.h"
#include "flprogUtilites.h"
#include "flprog_RTC.h"

// #if defined(STM32_CORE_VERSION) && (STM32_CORE_VERSION < 0x02000000)
// #error "This library is not compatible with core version used. Please update the core."
// #endif
#include "rtc.h"
// #ifndef HAL_RTC_MODULE_ENABLED
// #error "RTC configuration is missing. Check flag HAL_RTC_MODULE_ENABLED in variants/board_name/stm32yzxx_hal_conf.h"
// #endif
//  #include <time.h>

class FLProgSystemRTC : public FLProgRTCBase
{
public:
    FLProgSystemRTC(int16_t gmt=0);
    virtual void pool();
    void setReqestPeriod(uint32_t reqestPeriod) { _reqestPeriod = reqestPeriod; };

protected:
    virtual void privateSetTime();
    void readTime();
    bool _isInit = false;
    uint32_t _startReadTime = 0;
    uint32_t _reqestPeriod = 2000;
};
