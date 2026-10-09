#include "stm32f10x.h"                  // Device header

#ifndef __KEY_H
#define __KEY_H


void KEY_Init(void);
uint8_t kEY_GetNum(void);
void Key_Tick(void);
void Key3_Tick(void);

#endif
