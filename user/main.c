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


    /*任务句柄*/
    static TaskHandle_t HeartbeatTaskHandle = NULL;
    static TaskHandle_t KeyTaskHandle = NULL;
    static TaskHandle_t TickTaskHandle = NULL;
    static TaskHandle_t UITaskHandle = NULL;

    
    /*记录各任务历史上最少剩余过多少栈的变量*/
    volatile UBaseType_t led_stack_free = 0;
    volatile UBaseType_t key_stack_free = 0;
    volatile UBaseType_t tick_stack_free = 0;
    volatile UBaseType_t ui_stack_free = 0;

    /*记录当前剩余FreeRTOS堆空间*/
    volatile size_t heap_free = 0;

    /*统运行以来，堆空间最少剩余过多少*/
    volatile size_t heap_min_free = 0;

    static void Monitor_PrintUint32(uint32_t value);


//判断帧数的
static void OLED_BenchmarkTask(void *param)
{
    uint32_t i;
    uint32_t fps;
    TickType_t start_tick;
    TickType_t end_tick;
    TickType_t elapsed_tick;

    (void)param;

    /* 开始计时 */
    start_tick = xTaskGetTickCount();

    /* 连续发送100个完整屏幕 */
    for (i = 0; i < 100; i++)
    {
        OLED_Update();
    }

    /* 结束计时 */
    end_tick = xTaskGetTickCount();

    elapsed_tick = end_tick - start_tick;

    if (elapsed_tick != 0)
    {
        fps =
            100U *
            configTICK_RATE_HZ /
            elapsed_tick;

        Serial_Send_String("\r\nOLED benchmark:\r\n");

        Serial_Send_String("100 frames ticks: ");
        Monitor_PrintUint32((uint32_t)elapsed_tick);
        Serial_Send_String("\r\n");

        Serial_Send_String("OLED max FPS: ");
        Monitor_PrintUint32(fps);
        Serial_Send_String("\r\n");
    }

    /* 测试完成，删除当前任务 */
    vTaskDelete(NULL);
}

    /*--------------------------------------计数任务-----------------------------------*/
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

        xTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(1));
    }
}


    /*--------------------------------------手表UI任务-----------------------------------*/
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

    /*--------------------------------------按键任务-----------------------------------*/
static void KeyTask(void *param)
{
    // uint8_t state;
    // uint8_t last_state = 0;

    while (1)
    {
        Key3_Tick();
        Key_Tick();

        // state = Key_GetState();

        // if (state != last_state)
        // {
        //     Serial_Send_String("state=");
        //     Serial_SendByte(state + '0');
        //     Serial_Send_String("\r\n");

        //     last_state = state;
        // }

        vTaskDelay(1);
    }
}

/*---------------------------数字打印函数--------------------------------*/
static void Monitor_PrintUint32(uint32_t value)
{
    char buffer[10];
    uint8_t length = 0;

    if (value == 0)
    {
        Serial_SendByte('0');
        return;
    }

    while (value > 0)
    {
        buffer[length] = (char)('0' + value % 10);
        length++;
        value /= 10;
    }

    while (length > 0)
    {
        length--;
        Serial_SendByte((uint8_t)buffer[length]);
    }
}

/*---------------------------监控打印接口--------------------------------*/
static void Monitor_PrintRuntimeInfo(void)
{
    Serial_Send_String("\r\n[RTOS Monitor]\r\n");

    Serial_Send_String("LED stack free: ");
    Monitor_PrintUint32((uint32_t)led_stack_free);
    Serial_Send_String(" words\r\n");

    Serial_Send_String("KEY stack free: ");
    Monitor_PrintUint32((uint32_t)key_stack_free);
    Serial_Send_String(" words\r\n");

    Serial_Send_String("TICK stack free: ");
    Monitor_PrintUint32((uint32_t)tick_stack_free);
    Serial_Send_String(" words\r\n");

    Serial_Send_String("UI stack free: ");
    Monitor_PrintUint32((uint32_t)ui_stack_free);
    Serial_Send_String(" words\r\n");

    Serial_Send_String("Heap free: ");
    Monitor_PrintUint32((uint32_t)heap_free);
    Serial_Send_String(" bytes\r\n");

    Serial_Send_String("Heap min free: ");
    Monitor_PrintUint32((uint32_t)heap_min_free);
    Serial_Send_String(" bytes\r\n");
}


    /*--------------------------------------LED任务（判断有没有进调度器的）和栈空间判断-----------------------------------*/
static void HeartbeatTask(void *param)
{
    uint8_t monitor_count = 0;

    /*--------------OLED_FPS变量------------- */
    uint32_t last_frame_count;
    uint32_t current_frame_count;
    uint32_t frame_delta;
    uint32_t current_fps;

    TickType_t last_measure_tick;
    TickType_t current_measure_tick;
    TickType_t tick_delta;

    last_frame_count = OLED_FrameCount;
    last_measure_tick = xTaskGetTickCount();

    (void)param;

    while (1)
    {
        led_count++;

        led_stack_free =
            uxTaskGetStackHighWaterMark(HeartbeatTaskHandle);

        key_stack_free =
            uxTaskGetStackHighWaterMark(KeyTaskHandle);

        tick_stack_free =
            uxTaskGetStackHighWaterMark(TickTaskHandle);

        ui_stack_free =
            uxTaskGetStackHighWaterMark(UITaskHandle);

        heap_free =
            xPortGetFreeHeapSize();

        heap_min_free =
            xPortGetMinimumEverFreeHeapSize();

        LED0_ON();
        vTaskDelay(pdMS_TO_TICKS(50));

        LED0_OFF();
        vTaskDelay(pdMS_TO_TICKS(950));


        current_measure_tick = xTaskGetTickCount();
        current_frame_count = OLED_FrameCount;

        tick_delta = current_measure_tick - last_measure_tick;

        frame_delta = current_frame_count - last_frame_count;

        if (tick_delta != 0)
        {
            /*
             * FPS = 帧数差 ÷ 经过的秒数
             *
             * 经过的秒数 =
             * tick_delta / configTICK_RATE_HZ
             *
             * 整理后：
             * FPS =
             * frame_delta * configTICK_RATE_HZ / tick_delta
             */
            current_fps =
                frame_delta *
                configTICK_RATE_HZ /
                tick_delta;

            Serial_Send_String("OLED current FPS: ");
            Monitor_PrintUint32(current_fps);
            Serial_Send_String("\r\n");
        }

        last_frame_count = current_frame_count;
        last_measure_tick = current_measure_tick;

        monitor_count++;

        if (monitor_count >= 5)
        {
            monitor_count = 0;
            Monitor_PrintRuntimeInfo();
        }
    }


}


/*栈溢出（串口）*/
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

/*freertos堆内存的警告（串口）*/
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
    Peripheral_Init();


    /*--------------------创建队列 --------------------*/
    if (KEY_QueueInit() == 0)
    {
    Serial_Send_String("Create key queue failed\r\n");

    while (1)
    {
        LED0_ON();
    }
    }   

    /*--------------------创建任务----------------------*/

    create_ret = xTaskCreate(HeartbeatTask, "LED", 128, NULL, 1, &HeartbeatTaskHandle);
    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create LED task failed\r\n");

        while (1)
        {
            LED0_ON();
        }
    }

    create_ret = xTaskCreate(KeyTask, "KEY", 128, NULL, 2, &KeyTaskHandle);
    if (create_ret != pdPASS)
    {
    Serial_Send_String("Create KEY task failed\r\n");

    while (1)
    {
        LED0_ON();
    }
    }

    create_ret = xTaskCreate(TickTask, "TICK", 128, NULL, 3, &TickTaskHandle);
    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create TICK task failed\r\n");

        while (1)
        {
            LED0_ON();
        }
    }


    create_ret = xTaskCreate(UITask, "UI", 512, NULL, 1, &UITaskHandle);
    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create UI task failed\r\n");

        while (1)
        {
            LED0_ON();
        }
    }
        create_ret = xTaskCreate(
        OLED_BenchmarkTask,
        "OLED_BENCH",
        128,
        NULL,
        4,
        NULL
         );

    if (create_ret != pdPASS)
    {
        Serial_Send_String("Create OLED benchmark failed\r\n");
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