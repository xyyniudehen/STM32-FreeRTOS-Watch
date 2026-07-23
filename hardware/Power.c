#include "stm32f10x.h"                  // Device header
#include "Power.h"
#include "OLED.h"
#include "Delay.h"
#include "Timer.h"
#include "KEY.h"
#include "Alarm.h"
#include "MPU6050.h"
#include "MPU6050_Reg.h"
#include "LED.h"
#include "FreeRTOS.h"
#include "task.h"

static volatile uint32_t Power_RunMs = 0U;
static uint32_t Power_LastActivityMs = 0U;

static uint8_t Power_LockMask = 0U;

// static void Power_DebugPinInit(void)
// {
//     GPIO_InitTypeDef GPIO_InitStructure;

//     /* PB12属于GPIOB，必须先打开GPIOB时钟 */
//     RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

//     /* 配置PB12为普通推挽输出 */
//     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
//     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//     GPIO_Init(GPIOB, &GPIO_InitStructure);

//     /* 初始化完成后CPU正在正常运行，因此输出高电平 */
//     GPIO_SetBits(GPIOB, GPIO_Pin_12);
// }

void Power_WakeupInit(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource12);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource13);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource15);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line =EXTI_Line12 | EXTI_Line13 | EXTI_Line15 ;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =1 ;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    // Power_DebugPinInit();

}

static void Power_BusyDelayMs(uint32_t ms)
{
    while (ms--)
    {
        volatile uint32_t i = 7200;
        while (i--)
        {
            __NOP();
        }
    }
}

void Power_PrepareSleep(void)
{

    /* 2. 关闭OLED显示 */
    OLED_Clear();
    OLED_ShowString(24, 24, "sleep...", OLED_8X16);
    OLED_Update();

    // vTaskDelay(pdMS_TO_TICKS(500));
    // Delay_ms(500);
    Power_BusyDelayMs(500);

    LED0_OFF();
    /* 关闭OLED显示 */
    OLED_WriteCommand(0xAE);

    /* 关闭OLED内部电荷泵 */
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x10);

    /* 关闭ADC模拟部分 */
    ADC_Cmd(ADC1, DISABLE);
    /* 3. 让MPU6050进入睡眠 */
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x40);

    /* 4. 停止TIM2 */
    // TIM_Cmd(TIM2, DISABLE);
}

void Power_AfterWakeup(void)
{
    OLED_Init();
    OLED_Clear();
    OLED_Update();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    /* 该函数内部会使用Delay_us，所以必须放在恢复SysTick之前 */
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x00);

    Power_BusyDelayMs(100);
}

void Power_ClearWakeupKey(void)
{
    while ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0) ||
           (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_13) == 0) ||
           (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_15) == 0))
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    kEY_GetNum();
}


extern uint8_t kEY_Num;
extern int press_time;

void  Power_EnterStopMode(void)
{
    if (Power_LockMask != 0U)
    {
        return;
    }


    Power_ClearWakeupKey();

    if (Alarm_Enable == 1)
    {
        Alarm_SetHardwareAlarm();
    }

    /* 2. 清旧的RTC闹钟标志 */
    RTC_ClearITPendingBit(RTC_IT_ALR);
    EXTI_ClearITPendingBit(EXTI_Line17);

    /* 3. 清KEY1-3的EXTI标志 */
    EXTI_ClearITPendingBit(EXTI_Line15);
    EXTI_ClearITPendingBit(EXTI_Line12);
    EXTI_ClearITPendingBit(EXTI_Line13);

    OLED_Clear();
    OLED_ShowString(0, 0, "Before STOP", OLED_8X16);
    OLED_Update();
    // Delay_ms(500);
    Power_BusyDelayMs(500);

    Power_PrepareSleep();
    
    PWR_ClearFlag(PWR_FLAG_WU);

    GPIO_ResetBits(GPIOB, GPIO_Pin_12);

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

    SystemInit();
    
    Power_AfterWakeup();

    SysTick_Config(SystemCoreClock / configTICK_RATE_HZ);

    OLED_Clear();
    OLED_ShowString(24, 24, "Wake Up", OLED_8X16);
    OLED_Update();
    // vTaskDelay(pdMS_TO_TICKS(500));

    // vTaskDelay(pdMS_TO_TICKS(500));
     GPIO_SetBits(GPIOB, GPIO_Pin_12);    


    if (Alarm_RingFlag == 1U)
    {
        /*
        * RTC闹钟唤醒：
        * 不等待按键松开，也不显示普通Wake Up提示。
        * 尽快返回首页循环，由Alarm_Task()进入闹钟页面。
        */
        OLED_Clear();
        OLED_Update();
    }
    else
    {
        /*
        * 按键唤醒：
        * 等待唤醒按键释放，并清除这次按键事件，
        * 避免唤醒动作继续触发菜单操作。
        */
        Power_ClearWakeupKey();

        OLED_Clear();
        OLED_ShowString(24, 24, "Wake Up", OLED_8X16);
        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(300));

        OLED_Clear();
        OLED_Update();
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line12) == SET)
    {
        EXTI_ClearITPendingBit(EXTI_Line12);
    }

    if (EXTI_GetITStatus(EXTI_Line13) == SET)
    {
        EXTI_ClearITPendingBit(EXTI_Line13);
    }

    if (EXTI_GetITStatus(EXTI_Line15) == SET)
    {
        EXTI_ClearITPendingBit(EXTI_Line15);
    }
}




void Power_Tick1ms(void)
{
    Power_RunMs++;
}

void Power_ReportActivity(void)
{
    Power_LastActivityMs = Power_RunMs;
}

void Power_UIActivityTask(uint8_t key_num)
{
    /* 有按键事件，重新开始计算空闲时间 */
    if (key_num != 0U)
    {
        Power_ReportActivity();
    }

    /* 没有功能上锁且空闲超时，就进入STOP */
    Power_Task();
}

void Power_Lock(uint8_t reason)
{
    /* 对应原因原来没有上锁，才执行设置 */
    if ((Power_LockMask & reason) == 0U)
    {
        Power_LockMask |= reason;
        Power_ReportActivity();
    }
}

void Power_Unlock(uint8_t reason)
{
    /* 对应原因原来确实上锁，才执行清除 */
    if ((Power_LockMask & reason) != 0U)
    {
        Power_LockMask &= (uint8_t)(~reason);
        Power_ReportActivity();
    }
}


void Power_Task(void)
{
    if (Power_LockMask != 0U)
    {
        return;
    }

    if ((uint32_t)(Power_RunMs - Power_LastActivityMs)
        >= POWER_AUTO_STOP_MS)
    {
        Power_EnterStopMode();
        Power_ReportActivity();
    }
}




