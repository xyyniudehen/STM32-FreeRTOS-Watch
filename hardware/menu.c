#include "stm32f10x.h"                  // Device header
#include "MyRTC.h"
#include "OLED.h"
#include "KEY.h"
#include "LED.h"
#include "SetTime.h"
#include "menu.h"
#include "MPU6050.h"
#include "Delay.h"
#include "math.h"
#include "dino.h"
#include "AD.h"
#include "Power.h"
#include "Alarm.h"

uint8_t KeyNum;

void Peripheral_Init(void)
{
    MyRTC_Init();
    Alarm_HardwareInit();
    KEY_Init();
    Power_WakeupInit();
    LED_Init();
    MPU6050_Init();
    // MPU6050_Calibrate();
    AD_Init();

}

//*---------------------------首页时钟--------------------------------------*/


uint16_t AD_value;
float V_bat;
int Battery_Capacity;
void  Show_Battary(void)
{
    int sum=0;
    for (uint16_t i=0; i<3000;i++)
    {
            sum+= Get_ADCvalue();
    }
    AD_value = sum/3000;
    V_bat= (float)AD_value/4095*3.3;
    Battery_Capacity = (AD_value - 3276)*100/813;
    if(Battery_Capacity<0)Battery_Capacity=0;

    // OLED_ShowNum(64, 0, AD_value, 4, OLED_6X8);
    // OLED_Printf(64, 8, OLED_6X8, "VBAT:%.2f", V_bat);
    OLED_ShowNum(85, 4, Battery_Capacity,3, OLED_6X8);
    OLED_ShowChar(103, 4, '%', OLED_6X8);

    if (Battery_Capacity==100)OLED_ShowImage(110, 0, 16, 16,Battery1);
    else if (Battery_Capacity>=10&&Battery_Capacity<100)
    {
        OLED_ShowImage(110, 0, 16, 16,Battery1);
        OLED_ClearArea((112+Battery_Capacity/10), 5, (10-Battery_Capacity/10),6);
        OLED_ClearArea(85, 4, 6, 8);
    }
    else
    {
        OLED_ShowImage(110, 0, 16, 16,Battery1);
        OLED_ClearArea(112, 5, 10, 6);
        OLED_ClearArea(85, 4, 12, 8);
    }

}


void Show_Clock_UI(void)
{
    Show_Battary();
    MyRTC_ReadTime();
    OLED_Printf(0 , 0, OLED_6X8, "%04d-%02d-%02d", MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]);
    OLED_Printf(16 , 16, OLED_12X24, "%02d:%02d:%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
    OLED_ShowString(0, 48, "菜单", OLED_8X16);
    OLED_ShowString(96, 48, "设置", OLED_8X16);
    

}

int clkflag=1;

int First_Page_Clock(void)
{
    while(1)
    {   
        Alarm_Task();

        KeyNum=kEY_GetNum();
    
        Power_UIActivityTask(KeyNum);

        Power_Task();

    if (KeyNum==1) //上一项
    {
        clkflag--;
        if (clkflag<=0){clkflag=2;}
    }
    else if ( KeyNum==2) //下一项
    {
        clkflag++;
        if (clkflag>=3){clkflag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        return clkflag;
    }

    else if (KeyNum==4)
    {
        // GPIO_ResetBits(GPIOB, GPIO_Pin_13);
        // GPIO_SetBits(GPIOB, GPIO_Pin_12);
        GPIO_ResetBits(GPIOA, GPIO_Pin_2);
        GPIO_SetBits(GPIOA, GPIO_Pin_3);

    }
        
    switch(clkflag)
    {
        case 1:
        Show_Clock_UI();
        OLED_ReverseArea(0, 48, 32, 16);
        OLED_Update();
        break;
        case 2:
        Show_Clock_UI();
        OLED_ReverseArea(96, 48, 32, 16);
        OLED_Update();
        break;
    }
}
}

//*---------------------------设置界面--------------------------------------*/

void Show_SettingPage_UI(void)
{
    OLED_ShowImage(0, 0, 16,16 ,Return);
    OLED_ShowString(0, 16, "日期时间设置", OLED_8X16);

}

int setflag=1;
int SettingPage(void)
{

    while(1)
    {
        Alarm_Task();
        KeyNum=kEY_GetNum();

        Power_UIActivityTask(KeyNum);

        uint8_t setflag_temp=0;

    if (KeyNum==1) //上一项
    {
        setflag--;
        if (setflag<=0){setflag=2;}
    }
    else if ( KeyNum==2) //下一项
    {
        setflag++;
        if (setflag>=3){setflag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        setflag_temp=setflag;
    }

    if (setflag_temp==1){return 0;}
    else if(setflag_temp==2){SetTime();}

        
    switch(setflag)
    {
        case 1:
        Show_SettingPage_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;
        case 2:
        Show_SettingPage_UI();
        OLED_ReverseArea(0, 16, 96, 16);
        OLED_Update();
        break;
    }
    }
}


//*---------------------------滑动菜单界面--------------------------------------*/
uint8_t pre_selection;      //上次选择的选项
uint8_t target_selection;   //目标选项
uint8_t x_pre = 48;              //上次选项的x坐标
uint8_t Speed = 6;
uint8_t move_flag;          //=0停止，=1移动

void Menu_Animation(void)
{
    OLED_Clear();
    OLED_ShowImage(42, 10, 44, 44, Frame);

    if (pre_selection<target_selection)
    {
        x_pre-=Speed;
        if (x_pre==0)
        {
            pre_selection++;
            move_flag = 0;
            x_pre = 48;
        }
    }

    if (pre_selection>target_selection)
    {
        x_pre+=Speed;
        if (x_pre==96)
        {
            pre_selection--;
            move_flag = 0;
            x_pre = 48;
        }
    }

    if (pre_selection>=1)
    {
    OLED_ShowImage(x_pre-48, 16,32 , 32, Menu_Graph[pre_selection-1]);
    }
    if (pre_selection>=2)
    {
    OLED_ShowImage(x_pre-96, 16,32 , 32, Menu_Graph[pre_selection-2]);
    }

    OLED_ShowImage(x_pre, 16,32 , 32, Menu_Graph[pre_selection]);
    OLED_ShowImage(x_pre+48, 16,32 , 32, Menu_Graph[pre_selection+1]);
    OLED_ShowImage(x_pre+96, 16,32 , 32, Menu_Graph[pre_selection+2]);

    OLED_Update();
}

void Set_Selection(uint8_t move_flag, uint8_t Pre_Selection, uint8_t Target_Selection)
{
    if (move_flag==1)
    {
        pre_selection = Pre_Selection;
        target_selection = Target_Selection; 
    }
    Menu_Animation();
}


void MeunToStopWatch(void)
{

    for (uint8_t i =0; i<=7; i++)
    {
    OLED_Clear();
    if (pre_selection>=1)
    {
    OLED_ShowImage(x_pre-48, 16+8*i,32 , 32, Menu_Graph[pre_selection-1]);
    }
    OLED_ShowImage(x_pre,16+8*i    ,32 , 32, Menu_Graph[pre_selection]);
    OLED_ShowImage(x_pre+48,16+8*i ,32 , 32, Menu_Graph[pre_selection+1]);
        OLED_Update();
    }


}


uint8_t meun_flag=1;
int Meun1(void)
{
    move_flag=1;
    uint8_t DirectFlag=2; //1：移动到上一项， 2：移动到下一项
    while(1)
    {
        Alarm_Task();
        KeyNum = kEY_GetNum();

        Power_UIActivityTask(KeyNum);

        Power_Task();

        uint8_t meun_flag_temp = 0;

    if (KeyNum==1) //上一项
    {
        meun_flag--;
        DirectFlag=1;
        move_flag=1;
        if (meun_flag<=0){meun_flag=8;}
    }
    else if ( KeyNum==2) //下一项
    {
        DirectFlag=2;
        move_flag=1;
        meun_flag++;
        if (meun_flag>=9){meun_flag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        meun_flag_temp=meun_flag;
    }

    if (meun_flag_temp==1){return 0;}
    else if(meun_flag_temp==2){MeunToStopWatch(); Clock_meun();}
    else if(meun_flag_temp==3){MeunToStopWatch();LED();}
    else if(meun_flag_temp==4){MeunToStopWatch();MPU6050();}
    else if(meun_flag_temp==5){MeunToStopWatch();;Game(); }
    else if(meun_flag_temp==6){MeunToStopWatch();Emoji();}
    else if(meun_flag_temp==7){MeunToStopWatch();Gradienter();}
    else if(meun_flag_temp==8){MeunToStopWatch();Power_EnterStopMode();}
        

        if (meun_flag==1)
        {if (DirectFlag ==1)Set_Selection(move_flag, 1, 0);
         if (DirectFlag ==2)Set_Selection(move_flag, 0, 0);
        }
        else 
        {
        if (DirectFlag ==1)Set_Selection(move_flag, meun_flag, meun_flag-1);
        if (DirectFlag ==2)Set_Selection(move_flag, meun_flag-2, meun_flag-1);
        }
         

    }

}

//*---------------------------时钟菜单--------------------------------------*/


void Show_TimeTool_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(0, 16, "秒表", OLED_8X16);
    OLED_ShowString(0, 32, "闹钟", OLED_8X16);
}


uint8_t TimeTool_flag=1;
int Clock_meun(void)
{

    while(1)
    {
        Alarm_Task();
        KeyNum=kEY_GetNum();

        Power_UIActivityTask(KeyNum);
        uint8_t TimeTool_flag_temp=0;

    if (KeyNum==1) //上一项
    {
        TimeTool_flag--;
        if (TimeTool_flag<=0){TimeTool_flag=3;}
    }
    else if ( KeyNum==2) //下一项
    {
        TimeTool_flag++;
        if (TimeTool_flag>=4){TimeTool_flag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        TimeTool_flag_temp=TimeTool_flag;
    }

    if (TimeTool_flag_temp==1){return 0;}
    else if(TimeTool_flag_temp==2){StopWatch();}
    else if(TimeTool_flag_temp==3){SetAlarmTime();}


        
    switch(TimeTool_flag)
    {
        case 1:
        Show_TimeTool_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;

        case 2:
        Show_TimeTool_UI();
        OLED_ReverseArea(0, 16, 32, 16);
        OLED_Update();
        break;

        case 3:
        Show_TimeTool_UI();
        OLED_ReverseArea(0, 32, 32, 16);
        OLED_Update();
        break;


    }
    }
}

//*--------------------------闹钟--------------------------------------*/







//*---------------------------秒表--------------------------------------*/

uint8_t hour, min, sec;
void Show_StopWatch_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_Printf(32, 20, OLED_8X16, "%02d:%02d:%02d", hour, min, sec);
    OLED_ShowString(8, 44, "开始", OLED_8X16);
    OLED_ShowString(48, 44, "停止", OLED_8X16);
    OLED_ShowString(88, 44, "清除", OLED_8X16);
}

uint8_t start_timing_flag;  //开始1，停止0
void StopWatch_Tick(void)
{
    static uint16_t Count;
    Count++;
    if (Count>=1000)
    {
        Count=0;

                if (start_timing_flag ==1)
        {
            sec++;
            if (sec>=60)
            {
                sec=0;
                min++;
                if(min>=60)
                {
                    hour++;
                    min=0;
                    if (hour>99)hour=0;
                }
            }
        }
    }


}



uint8_t stopwatch_flag=1;
int StopWatch(void)
{

    while(1)
    {
        Alarm_Task();
        KeyNum = kEY_GetNum();

        Power_UIActivityTask(KeyNum);

        uint8_t setflag_temp = 0;

    if (KeyNum==1) //上一项
    {
        stopwatch_flag--;
        if (stopwatch_flag<=0){stopwatch_flag=4;}
    }
    else if ( KeyNum==2) //下一项
    {
        stopwatch_flag++;
        if (stopwatch_flag>=5){stopwatch_flag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        setflag_temp=stopwatch_flag;
    }

    if (setflag_temp==1){return 0;}


        
    switch(stopwatch_flag)
    {
        case 1:
        Show_StopWatch_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;

        case 2:
        Show_StopWatch_UI();
        start_timing_flag = 1;
        Power_Lock(POWER_LOCK_STOPWATCH);
        OLED_ReverseArea(8, 44, 32, 16);
        OLED_Update();
        break;

        case 3:
        Show_StopWatch_UI();
        start_timing_flag = 0;
        Power_Unlock(POWER_LOCK_STOPWATCH);
        OLED_ReverseArea(48, 44, 32, 16);
        OLED_Update();
        break;

        case 4:
        Show_StopWatch_UI();
        start_timing_flag = 0;
        Power_Unlock(POWER_LOCK_STOPWATCH);
        hour = 0;
        min = 0;
        sec = 0;
        OLED_ReverseArea(88, 44, 32, 16);
        OLED_Update();
        break;
    }
    Power_Task();
    }
}




//*---------------------------手电筒--------------------------------------*/


void Show_LED_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(20, 20, "OFF", OLED_12X24);
    OLED_ShowString(72, 20, "ON", OLED_12X24);
}

uint8_t LED_flag=1;
int LED(void)
{
    while(1)
    {
        Alarm_Task();
        KeyNum=kEY_GetNum();

        Power_UIActivityTask(KeyNum);
        uint8_t LED_flag_temp=0;

    if (KeyNum==1) //上一项
    {
        LED_flag--;
        if (LED_flag<=0){LED_flag=3;}
    }
    else if ( KeyNum==2) //下一项
    {
        LED_flag++;
        if (LED_flag>=4){LED_flag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        LED_flag_temp=LED_flag;
    }

    if (LED_flag_temp==1){return 0;}

        
    switch(LED_flag)
    {
        case 1:
        Show_LED_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;

        case 2:
        Show_LED_UI();
        LED0_OFF();
        OLED_ReverseArea(20, 20, 36, 24);

        OLED_Update();
        break;

        case 3:
        Show_LED_UI();
        OLED_ReverseArea(72, 20, 24, 24);
        LED0_ON();
        OLED_Update();
        break;

    }
    }
}


//*---------------------------MPU6050--------------------------------------*/

int16_t ax, ay, az, gx, gy, gz;  //MPU6050得到的三轴加速度和角速度  
float roll_g, pitch_g, yaw_g;    //陀螺仪---欧拉角
float roll_a, pitch_a;          //加速度---欧拉角
float Roll, Pitch, Yaw;            //互补---欧拉角
float a=0.95;                    //互补权重
float Delta_t= 0.005;           //采样周期
double pi=3.1415927;

// void MPU6050_Calculation(void)
// {
//     Delay_ms(5);
//     MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

//     //陀螺仪--欧拉角
//     roll_g = roll_g + (float)gx*Delta_t;
//     pitch_g= pitch_g +(float)gy*Delta_t;
//     yaw_g = yaw_g +(float)gz*Delta_t;

//     //加速度--欧拉角
//     pitch_a = atan2((-1)*ax, az)*180/pi;
//     roll_a = atan2(ay, az)*180/pi;

//     //通过互补滤波
//     Roll = a*roll_g +(1-a)*roll_a ;
//     Pitch= a*pitch_g +(1-a)*pitch_a ;
//     Yaw = a*yaw_g;
// }

float gx_offset = 0;
float gy_offset = 0;
float gz_offset = 0;

void MPU6050_Calculation(void)
{
    Delay_ms(5);
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

    float gx_dps = ((float)gx - gx_offset) / 131.0f;
    float gy_dps = ((float)gy - gy_offset) / 131.0f;
    float gz_dps = ((float)gz - gz_offset) / 131.0f;

    roll_a = atan2((float)ay, (float)az) * 180.0f / pi;
    pitch_a = atan2(-(float)ax, sqrt((float)ay * ay + (float)az * az)) * 180.0f / pi;

    Roll  = a * (Roll  + gx_dps * Delta_t) + (1 - a) * roll_a;
    Pitch = a * (Pitch + gy_dps * Delta_t) + (1 - a) * pitch_a;
    Yaw   = Yaw + gz_dps * Delta_t;
}


void Show_MPU6050_UI(void)
{
    OLED_ShowImage(0,0,16,16,Return);
    OLED_Printf (0, 16, OLED_8X16, "Roll:%.2f", Roll);
    OLED_Printf (0, 32, OLED_8X16, "Pitch:%.2f", Pitch);
    OLED_Printf (0, 48, OLED_8X16, "Yaw:%.2f", Yaw);

}

// void Show_MPU6050_UI(void)
// {
//     OLED_ShowImage(0, 0, 16, 16, Return);
//     OLED_Printf(0, 16, OLED_8X16, "AX:%d", ax);
//     OLED_Printf(0, 32, OLED_8X16, "AY:%d", ay);
//     OLED_Printf(0, 48, OLED_8X16, "AZ:%d", az);
// }

void MPU6050_Calibrate(void)
{
    int32_t gx_sum = 0;
    int32_t gy_sum = 0;
    int32_t gz_sum = 0;

    for (uint16_t i = 0; i < 500; i++)
    {
        MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

        gx_sum += gx;
        gy_sum += gy;
        gz_sum += gz;

        Delay_ms(2);
    }

    gx_offset = gx_sum / 500.0f;
    gy_offset = gy_sum / 500.0f;
    gz_offset = gz_sum / 500.0f;
}

int MPU6050(void)
{
    while (1)
    {
        

    static uint8_t calibrated = 0;

    if (calibrated == 0)
    {
        OLED_Clear();
        OLED_ShowString(16, 24, "Calibrating", OLED_8X16);
        OLED_Update();

        MPU6050_Calibrate();
        calibrated = 1;
    }

        Alarm_Task();
        KeyNum = kEY_GetNum();
        Power_UIActivityTask(KeyNum);
        if (KeyNum ==3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        OLED_Clear();
        MPU6050_Calculation();
        Show_MPU6050_UI();
        OLED_ReverseArea(0,0,16,16);
        OLED_Update();

    }
    

}

//*---------------------------游戏--------------------------------------*/

void Show_Game_UI(void)
{
    OLED_ShowImage(0,0,16,16,Return);
    OLED_ShowString (0, 16, "谷歌小恐龙", OLED_8X16);
}

uint8_t Game_flag=1;
int Game(void)
{
    while(1)
    {
        Alarm_Task();
        KeyNum = kEY_GetNum();

        Power_UIActivityTask(KeyNum);

        /* 游戏选择菜单属于普通页面，允许自动休眠 */
        Power_Task();

        uint8_t Game_flag_temp = 0;

    if (KeyNum==1) //上一项
    {
        Game_flag--;
        if (Game_flag<=0){Game_flag=2;}
    }
    else if ( KeyNum==2) //下一项
    {
        Game_flag++;
        if (Game_flag>=3){Game_flag=1;}
    }
    else if (KeyNum==3) //确认
    {
        OLED_Clear();
        OLED_Update();
        Game_flag_temp=Game_flag;
    }

    if (Game_flag_temp==1){return 0;}
    else if (Game_flag_temp==2){Dino_Game_Pos_Init();DinoGame_Animation();}

        
    switch(Game_flag)
    {
        case 1:
        Show_Game_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
        break;

        case 2:
        Show_Game_UI();
        OLED_ReverseArea(0, 16, 80, 16);
        OLED_Update();
        break;

    }
    }
}

//*---------------------------动态表情--------------------------------------*/

void Show_Emoji_UI(void)
{
    //闭眼
    for (uint8_t i=0; i<3; i++)
    {
    OLED_ShowImage(30, 10+i, 16, 16, Eyebrow[0]);
    OLED_ShowImage(82, 10+i, 16, 16, Eyebrow[1]);
    OLED_DrawEllipse(40, 32, 6, 6-i, 1);
    OLED_DrawEllipse(88, 32, 6, 6-i, 1);
    OLED_ShowImage(54, 40, 20, 20, Mouth);
    OLED_Update();
    Delay_ms(100);
    }
    //睁眼
    for (uint8_t i=0; i<3; i++)
    {
    OLED_ShowImage(30, 12-i, 16, 16, Eyebrow[0]);
    OLED_ShowImage(82, 12-i, 16, 16, Eyebrow[1]);
    OLED_DrawEllipse(40, 32, 6, 4+i, 1);
    OLED_DrawEllipse(88, 32, 6, 4+i, 1);
    OLED_ShowImage(54, 40, 20, 20, Mouth);
    OLED_Update();
    Delay_ms(100);
    }
     Delay_ms(500);
}

int Emoji(void)
{

   while (1)
    {
        Alarm_Task();
        KeyNum = kEY_GetNum();
        Power_UIActivityTask(KeyNum);
        if (KeyNum ==3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        Show_Emoji_UI();


    }
    

}

//*---------------------------水平仪--------------------------------------*/

void Show_Gradienter_UI(void)
{
        OLED_Clear();
        MPU6050_Calculation();
        OLED_DrawCircle(64, 32, 30, 0);
        OLED_DrawCircle(64-Roll, 32+Pitch, 4, 1);
}

int Gradienter(void)
{

   while (1)
    {
        Alarm_Task();
        KeyNum = kEY_GetNum();
            Power_UIActivityTask(KeyNum);
        if (KeyNum ==3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        Show_Gradienter_UI();
        OLED_Update();


    }
    

}


//*-----------------------------低功耗-------------------------------*/