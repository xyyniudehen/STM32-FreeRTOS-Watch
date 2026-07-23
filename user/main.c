#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "LED.h"
#include "KEY.h"
#include "Serial.h"
#include "OLED.h"
#include "menu.h"
#include "Power.h"
#include "dino.h"

volatile uint32_t led_count = 0;
volatile BaseType_t create_ret = 0;

volatile uint8_t malloc_failed_flag = 0;


/*
 * FreeRTOS任务划分：
 *
 * HeartbeatTask：
 *   系统心跳任务，用LED闪烁判断调度器是否正常运行。
 *
 * KeyTask：
 *   1ms周期调用Key_Tick，代替原来TIM2中断中的按键扫描。
 *   只负责产生按键事件，不直接处理菜单。
 *
 * TickTask：
 *   1ms周期调用StopWatch_Tick、Dino_Tick等周期函数，
 *   代替原来TIM2中断中的业务节拍。
 *
 * UITask：
 *   运行原来的手表页面函数，如First_Page_Clock、Meun1、SettingPage。
 *   页面内部通过kEY_GetNum获取按键事件。
 */

static void TickTask(void *param)
{
    TickType_t last_wake_time;

    (void)param;

    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        Power_Tick1ms();
        StopWatch_Tick();
        Dino_Tick();

        xTaskDelayUntil(
            &last_wake_time,
            pdMS_TO_TICKS(1)
        );
    }
}

static void UITask(void *param)
{
    int clkflag1;

    // OLED_Init();
    // OLED_Clear();
    // OLED_Update();

    // Peripheral_Init();

    while (1)
    {
        clkflag1 = First_Page_Clock();

        if (clkflag1 == 1)
        {
            Meun1();
        }
        else if (clkflag1 == 2)
        {
            SettingPage();
        }

        vTaskDelay(1);
    }
}

static void KeyTask(void *param)
{
    uint8_t state;
    uint8_t last_state = 0;

    while (1)
    {
        Key3_Tick();
        Key_Tick();

        state = Key_GetState();

        if (state != last_state)
        {
            Serial_Send_String("state=");
            Serial_SendByte(state + '0');
            Serial_Send_String("\r\n");

            last_state = state;
        }

        vTaskDelay(1);
    }
}


static void HeartbeatTask(void *param)
{
    while (1)
    {
        led_count++;

        LED0_ON();
        vTaskDelay(pdMS_TO_TICKS(50));

        LED0_OFF();
        vTaskDelay(pdMS_TO_TICKS(950));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    Serial_Send_String("Stack overflow: ");
    Serial_Send_String(pcTaskName);
    Serial_Send_String("\r\n");

    while (1)
    {
        LED0_ON();
    }
}

void vApplicationMallocFailedHook(void)
{
    malloc_failed_flag = 1;

    Serial_Send_String("Malloc failed\r\n");
}

int main(void)
{
    LED_Init();
    KEY_Init();
    Serial_Init();

    OLED_Init();
    OLED_Clear();
    OLED_Update();

    Peripheral_Init();

    create_ret = xTaskCreate(HeartbeatTask, "LED", 128, NULL, 1, NULL);
    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create LED task failed\r\n");

        while (1)
        {
            LED0_ON();
        }
    }

    create_ret = xTaskCreate(KeyTask, "KEY", 128, NULL, 2, NULL);
    if (create_ret != pdPASS)
    {
    Serial_Send_String("Create KEY task failed\r\n");

    while (1)
    {
        LED0_ON();
    }
    }

    create_ret = xTaskCreate(TickTask, "TICK", 128, NULL, 3, NULL);
    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create TICK task failed\r\n");

        while (1)
        {
            LED0_ON();
        }
    }


    create_ret = xTaskCreate(UITask, "UI", 1024, NULL, 1, NULL);
    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create UI task failed\r\n");

        while (1)
        {
            LED0_ON();
        }
    }

    vTaskStartScheduler();
    /*
    * 正常情况下不会执行到这里。
    * 如果执行到这里，通常表示FreeRTOS创建内部任务失败，
    * 或调度器没有成功启动。
    */
    Serial_Send_String("Scheduler start failed\r\n");

    while (1)
    {
        LED0_ON();
    }


}