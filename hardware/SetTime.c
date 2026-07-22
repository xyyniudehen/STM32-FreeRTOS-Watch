#include "stm32f10x.h"                  // Device header
#include "MyRTC.h"
#include "OLED.h"
#include "KEY.h"
#include "Power.h"

void ShowTime_FirstUI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_Printf(0, 16, OLED_8X16, "年:%4d", MyRTC_Time[0]);
    OLED_Printf(0, 32, OLED_8X16, "月:%2d", MyRTC_Time[1]);
    OLED_Printf(0, 48, OLED_8X16, "日:%2d", MyRTC_Time[2]);
}

void ShowTime_SecondUI(void)
{
    OLED_Printf(0, 0, OLED_8X16, "时:%2d", MyRTC_Time[3]);
    OLED_Printf(0, 16, OLED_8X16, "分:%2d", MyRTC_Time[4]);
    OLED_Printf(0, 32, OLED_8X16, "秒:%2d", MyRTC_Time[5]);

}

void Change_RTC_Time(uint8_t i, uint8_t flag)
{
    if (flag==1){MyRTC_Time[i]++;}
    else{MyRTC_Time[i]--;}
}

extern uint8_t KeyNum;

int SetYear(void)
{
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Change_RTC_Time(0,1);
    }
    else if (KeyNum==2) //数值-1
    {
        Change_RTC_Time(0,0);  
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        return 0;
    }
    ShowTime_FirstUI();
    OLED_ReverseArea(24, 16, 32, 16);
    OLED_Update();

}
}

int SetMonth(void)
{
    while(1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Change_RTC_Time(1,1);
        if (MyRTC_Time[1]>=13){MyRTC_Time[1]=1;MyRTC_SetTime();}
    }
    else if (KeyNum==2) //数值-1
    {
        Change_RTC_Time(1,0);  
        if (MyRTC_Time[1]<1){MyRTC_Time[1]=12;MyRTC_SetTime();}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {

        return 0;
    }
    ShowTime_FirstUI();
    OLED_ReverseArea(24, 32, 16, 16);
    OLED_Update();
}
}

int SetDay(void)
{
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Change_RTC_Time(2,1);
        if (MyRTC_Time[2]>=32){MyRTC_Time[2]=1;MyRTC_SetTime();}
    }
    else if (KeyNum==2) //数值-1
    {
        Change_RTC_Time(2,0);  
        if (MyRTC_Time[2]<1){MyRTC_Time[2]=31;MyRTC_SetTime();}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        return 0;
    }
    ShowTime_FirstUI();
    OLED_ReverseArea(24, 48, 16, 16);
    OLED_Update();
  }
}


int SetHour(void)
{
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Change_RTC_Time(3,1);
        if (MyRTC_Time[3]>=24){MyRTC_Time[3]=0;MyRTC_SetTime();}
    }
    else if (KeyNum==2) //数值-1
    {
        Change_RTC_Time(3,0);  
        if (MyRTC_Time[3]<0){MyRTC_Time[3]=23;MyRTC_SetTime();}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        return 0;
    }
    ShowTime_SecondUI();
    OLED_ReverseArea(24, 0, 16, 16);
    OLED_Update();
  }
}

int SetMin(void)
{
    
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Change_RTC_Time(4,1);
        if (MyRTC_Time[4]>=60){MyRTC_Time[4]=0;MyRTC_SetTime();}
    }
    else if (KeyNum==2) //数值-1
    {
        Change_RTC_Time(4,0);  
        if (MyRTC_Time[4]<0){MyRTC_Time[4]=60;MyRTC_SetTime();}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        MyRTC_SetTime();
        return 0;
    }
    ShowTime_SecondUI();
    OLED_ReverseArea(24, 16, 16, 16);
    OLED_Update();
  }
}

int SetSec(void)
{
    
    while (1)
    {
    KeyNum=kEY_GetNum();
    Power_UIActivityTask(KeyNum);
    if (KeyNum==1)  //数值+1
    {
        Change_RTC_Time(5,1);
        if (MyRTC_Time[5]>=60){MyRTC_Time[5]=0;MyRTC_SetTime();}
    }
    else if (KeyNum==2) //数值-1
    {
        Change_RTC_Time(5,0);  
        if (MyRTC_Time[5]<0){MyRTC_Time[5]=60;MyRTC_SetTime();}
    }
    else if (KeyNum==3)  //确认，保存并退出
    {
        return 0;
    }
    ShowTime_SecondUI();
    OLED_ReverseArea(24, 32, 16, 16);
    OLED_Update();
  }
}


int Set_Time_flag=1;

int SetTime(void)
{
        while(1)
    {
        KeyNum=kEY_GetNum();
        Power_UIActivityTask(KeyNum);
        uint8_t set_time_flag_temp=0;
    if (KeyNum==1) //上一项
    {
        Set_Time_flag--;
        if (Set_Time_flag<=0){Set_Time_flag=7;}
    }
    else if ( KeyNum==2) //下一项
    {
        Set_Time_flag++;
        if (Set_Time_flag>=8){Set_Time_flag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        // return Set_Time_flag;
        set_time_flag_temp=Set_Time_flag;
    }
        if (set_time_flag_temp==1){return 0;}
        else if(set_time_flag_temp==2){SetYear();}    //年
        else if(set_time_flag_temp==3){SetMonth();}    //月
        else if(set_time_flag_temp==4){SetDay();}    //日
        else if(set_time_flag_temp==5){SetHour();}    //时
        else if(set_time_flag_temp==6){SetMin();}    //分
        else if(set_time_flag_temp==7){SetSec();}    //秒

    switch(Set_Time_flag)
    {
        case 1:
        OLED_Clear();
        ShowTime_FirstUI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;
        
        case 2:
        OLED_Clear();
        ShowTime_FirstUI();
        OLED_ReverseArea(0, 16, 16, 16);
        OLED_Update();
        break;
        
        case 3:
        OLED_Clear();
        ShowTime_FirstUI();
        OLED_ReverseArea(0, 32, 16, 16);
        OLED_Update();
        break;

        case 4:
        OLED_Clear();
        ShowTime_FirstUI();
        OLED_ReverseArea(0, 48, 16, 16);
        OLED_Update();
        break;

        case 5:
        OLED_Clear();
        ShowTime_SecondUI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;

        case 6:
        OLED_Clear();
        ShowTime_SecondUI();
        OLED_ReverseArea(0, 16, 16, 16);
        OLED_Update();
        break;

        case 7:
        OLED_Clear();
        ShowTime_SecondUI();
        OLED_ReverseArea(0, 32, 16, 16);
        OLED_Update();
        break;
    }
}

}