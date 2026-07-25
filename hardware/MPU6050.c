#include "stm32f10x.h"                  // Device header
#include "MyI2C.h"
#include "MPU6050_Reg.h"
#define MPU6050_Address 0xD0


void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_Address);
    MyI2C_ReceiveACK();
    MyI2C_SendByte(RegAddress);
    MyI2C_ReceiveACK();
    MyI2C_SendByte(Data);
    MyI2C_ReceiveACK();
    MyI2C_Stop();

}

uint8_t MPU6050_readReg(uint8_t RegAddress)
{
    uint8_t Data;
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_Address);
    MyI2C_ReceiveACK();
    MyI2C_SendByte(RegAddress);
    MyI2C_ReceiveACK();
    
    MyI2C_Start();
    MyI2C_SendByte(MPU6050_Address|0x01);
    MyI2C_ReceiveACK();
    Data = MyI2C_ReceiveByte();
    MyI2C_SendACK(1);
    MyI2C_Stop();

    return Data;

}

uint8_t MPU6050_Init(void)
{
    uint8_t retry;

    MyI2C_Init();
    Delay_ms(300);

    /* 上电后WHO_AM_I可能第一次读取不稳定，最多重试5次 */
    for (retry = 0; retry < 5; retry++)
    {
        if (MPU6050_readReg(0x75) == 0x68)
        {
            break;
        }

        Delay_ms(50);
    }

    if (retry >= 5)
    {
        return 0;
    }

    /* 软件复位，复位后PWR_MGMT_1会重新变成0x40 */
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x80);
    Delay_ms(100);

    /* 唤醒并回读验证 */
    for (retry = 0; retry < 3; retry++)
    {
        MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);
        Delay_ms(20);

        if ((MPU6050_readReg(MPU6050_PWR_MGMT_1) & 0x40) == 0)
        {
            break;
        }
    }

    if (retry >= 3)
    {
        return 0;
    }

    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x00);
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x00);

    Delay_ms(100);

    return 1;
}

void MPU6050_GetData(int16_t *AccX, int16_t *ACCY, int16_t *AccZ,
                       int16_t *GYOX, int16_t *GYOY, int16_t *GYOZ )
{
   uint8_t Data_H, Data_L;

   Data_H=MPU6050_readReg(MPU6050_ACCEL_XOUT_H);
   Data_L=MPU6050_readReg(MPU6050_ACCEL_XOUT_L);
   *AccX = (Data_H <<8) | Data_L;

   Data_H=MPU6050_readReg(MPU6050_ACCEL_YOUT_H);
   Data_L=MPU6050_readReg(MPU6050_ACCEL_YOUT_L);
   *ACCY = (Data_H <<8) | Data_L;

   Data_H=MPU6050_readReg(MPU6050_ACCEL_ZOUT_H);
   Data_L=MPU6050_readReg(MPU6050_ACCEL_ZOUT_L);
   *AccZ = (Data_H <<8) | Data_L;

   Data_H=MPU6050_readReg(MPU6050_GYRO_XOUT_H);
   Data_L=MPU6050_readReg(MPU6050_GYRO_XOUT_L);
   *GYOX = (Data_H <<8) | Data_L;

   Data_H=MPU6050_readReg(MPU6050_GYRO_YOUT_H);
   Data_L=MPU6050_readReg(MPU6050_GYRO_YOUT_L);
   *GYOY = (Data_H <<8) | Data_L;

   Data_H=MPU6050_readReg(MPU6050_GYRO_ZOUT_H);
   Data_L=MPU6050_readReg(MPU6050_GYRO_ZOUT_L);
   *GYOZ = (Data_H <<8) | Data_L;

}