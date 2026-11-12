#include "stm32_def.h"
void enableBackupDomain(void);
void resetBackupDomain(void);
uint32_t getBackupRegister(uint32_t reg);
void setBackupRegister(uint32_t reg,uint32_t value);
