#include "stm32f10x.h"                  // Device header
#ifndef __MPU6050_H
#define __MPU6050_H


void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_readReg(uint8_t RegAddress);
void MPU6050_Init(void);
void MPU6050_GetData(int16_t *AccX, int16_t *ACCY, int16_t *AccZ,
                       int16_t *GYOX, int16_t *GYOY, int16_t *GYOZ);


#endif // !__MPU6050_H