#include "flprogRtcStm32.h"
#include <type_traits>
static_assert(std::is_same<decltype(&FLPROG_RTC_init), bool(*)(flprog_hourFormat_t,sourceClock_t,bool)>::value, "RTC init signature changed");
static_assert(std::is_same<decltype(&FLPROG_RTC_GetTime),void(*)(uint8_t*,uint8_t*,uint8_t*,uint32_t*,flprog_hourAM_PM_t*)>::value, "GetTime signature changed");
int main() {return 0;}
