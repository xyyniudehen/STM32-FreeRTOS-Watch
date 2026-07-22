#include "stm32f10x.h"
#include "Alarm.h"
#include "OLED.h"
#include "KEY.h"
#include "MyRTC.h"
#include "Delay.h"
#include <time.h>
#include "Power.h"


uint8_t Alarm_Enable = 0;

static uint8_t Alarm_MenuFlag = 1;
static uint8_t Alarm_LastMin = 255;


uint8_t Alarm_Hour = 7;
uint8_t Alarm_Min = 30;

void Show_Alarm_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);

    OLED_Printf(0, 16, OLED_8X16, "时:%02d", Alarm_Hour);
    OLED_Printf(0, 32, OLED_8X16, "分:%02d", Alarm_Min);

    if (Alarm_Enable)
    {
        OLED_ShowString(0, 48, "开 ", OLED_8X16);
    }
    else
    {
        OLED_ShowString(0, 48, "关", OLED_8X16);
    }
}


extern uint8_t KeyNum;
int Alarm_SetHour(void)
{
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Alarm_Hour++;
        if (Alarm_Hour>=24){Alarm_Hour=0;}
    }
    else if (KeyNum==2) //数值-1
    {
        Alarm_Hour--;
        if (Alarm_Hour<1){Alarm_Hour=24;}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        return 0;
    }
    Show_Alarm_UI();
    OLED_ReverseArea(40, 16, 16, 16);
    OLED_Update();
  }
}

int Alarm_SetMin(void)
{
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Alarm_Min++;
        if (Alarm_Min>=60){Alarm_Min=0;}
    }
    else if (KeyNum==2) //数值-1
    {
        Alarm_Min--;
        if (Alarm_Min<1){Alarm_Min=59;}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        return 0;
    }
    Show_Alarm_UI();
    OLED_ReverseArea(40, 32, 16, 16);
    OLED_Update();
  }
}

volatile uint8_t Alarm_RingFlag = 0;

int SetAlarmTime(void)
{
        while(1)
    {
        KeyNum=kEY_GetNum();
        Power_UIActivityTask(KeyNum);
        uint8_t Alarm_RingFlag_temp=0;
    if (KeyNum==1) //上一项
    {
        Alarm_MenuFlag--;
        if (Alarm_MenuFlag<=0){Alarm_MenuFlag=4;}
    }
    else if ( KeyNum==2) //下一项
    {
        Alarm_MenuFlag++;
        if (Alarm_MenuFlag>=5){Alarm_MenuFlag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        Alarm_SetHardwareAlarm();
        Alarm_RingFlag_temp=Alarm_MenuFlag;
    }
        if (Alarm_RingFlag_temp==1){return 0;}
        else if(Alarm_RingFlag_temp==2){Alarm_SetHour();}    //小时
        else if(Alarm_RingFlag_temp==3){Alarm_SetMin();}    //分钟
        else if(Alarm_RingFlag_temp==4){Alarm_Enable = !Alarm_Enable;Alarm_SetHardwareAlarm();}    //开关

    switch(Alarm_MenuFlag)
    {
        case 1:
        OLED_Clear();
        Show_Alarm_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;
        
        case 2:
        OLED_Clear();
        Show_Alarm_UI();
        OLED_ReverseArea(0, 16, 16, 16);
        OLED_Update();
        break;
        
        case 3:
        OLED_Clear();
        Show_Alarm_UI();
        OLED_ReverseArea(0, 32, 16, 16);
        OLED_Update();
        break;

        case 4:
        OLED_Clear();
        Show_Alarm_UI();
        OLED_ReverseArea(0, 48, 16, 16);
        OLED_Update();
        break;
    }
}

}

void Alarm_Check(void)
{
    MyRTC_ReadTime();

    if (Alarm_Enable == 1 &&
        MyRTC_Time[3] == Alarm_Hour &&
        MyRTC_Time[4] == Alarm_Min &&
        MyRTC_Time[5] == 0 &&
        Alarm_LastMin != MyRTC_Time[4])
    {
        Alarm_RingFlag = 1;
        Alarm_LastMin = MyRTC_Time[4];          //
    }

    if (MyRTC_Time[4] != Alarm_LastMin)         //通过分钟变化保证同一分钟内只触发一次闹钟
    {
        Alarm_LastMin = 255;
    }
}

void Alarm_RingPage(void)
{
    Power_Lock(POWER_LOCK_ALARM);
    while (1)
    {
        KeyNum = kEY_GetNum();

        if (KeyNum == 3)
        {
            Alarm_RingFlag = 0;
            OLED_Clear();
            OLED_Update();
            Power_Unlock(POWER_LOCK_ALARM);
            return;
        }

        OLED_Clear();
        OLED_ShowString(32, 16, "ALARM", OLED_8X16);
        OLED_ShowString(16, 40, "KEY3 STOP", OLED_8X16);
        OLED_Update();

        Delay_ms(100);
    }
}

//---------------------------------硬件闹钟设置-----------------------------------------//

void Alarm_HardwareInit(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    EXTI_ClearITPendingBit(EXTI_Line17);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&NVIC_InitStructure);

    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_WaitForLastTask();
    

}

void Alarm_SetHardwareAlarm(void)
{
    time_t alarm_cnt;
    uint32_t now_cnt;
    struct tm alarm_time;

    if (Alarm_Enable ==0)
    {
        RTC_ITConfig(RTC_IT_ALR, DISABLE);
        RTC_WaitForLastTask();

        RTC_ClearITPendingBit(RTC_IT_ALR);
        RTC_WaitForLastTask();

        EXTI_ClearITPendingBit(EXTI_Line17);
        return;
    }

    MyRTC_ReadTime();

    alarm_time.tm_year = MyRTC_Time[0] - 1900;
    alarm_time.tm_mon = MyRTC_Time[1] - 1;
    alarm_time.tm_mday = MyRTC_Time[2];
    alarm_time.tm_hour = Alarm_Hour;
    alarm_time.tm_min = Alarm_Min;
    alarm_time.tm_sec = 0;
    
    alarm_cnt = mktime(&alarm_time) - 8*60*60;

    now_cnt = RTC_GetCounter();

    if ((uint32_t)alarm_cnt<=now_cnt)
    {
         alarm_cnt  += 24 *60*60;
    }

    RTC_ITConfig(RTC_IT_ALR, DISABLE);
    RTC_WaitForLastTask();

    RTC_SetAlarm((uint32_t)alarm_cnt);
    RTC_WaitForLastTask();

    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_WaitForLastTask();

    EXTI_ClearITPendingBit(EXTI_Line17);

    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    RTC_WaitForLastTask();


}

void Alarm_Task(void)
{
    //Alarm_Check();
    if (Alarm_RingFlag)
    {
        Alarm_SetHardwareAlarm();
        Alarm_RingPage();

    }

}


void RTCAlarm_IRQHandler(void)
{
    if (RTC_GetITStatus(RTC_IT_ALR)  != RESET)
    {
        Alarm_RingFlag =1;

        RTC_ClearITPendingBit(RTC_IT_ALR);
        RTC_WaitForLastTask();
        
        EXTI_ClearITPendingBit(EXTI_Line17);

    }

}

