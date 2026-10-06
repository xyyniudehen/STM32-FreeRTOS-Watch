[33mtag v1.0-freertos[m
Tagger: xyyniudehen <x3369477538@163.com>
Date:   Sat Jul 25 20:58:02 2026 +0800

Stable FreeRTOS watch release

[33mcommit 30cbcc456ea9f2959a526e783b55761c9ffb84b0[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mmain[m[33m, [m[1;33mtag: [m[1;33mv1.0-freertos[m[33m, [m[1;31morigin/main[m[33m)[m
Author: xyyniudehen <x3369477538@163.com>
Date:   Sat Jul 25 20:57:42 2026 +0800

    feat: complete FreeRTOS watch integration

[1mdiff --git a/hardware/KEY.c b/hardware/KEY.c[m
[1mindex 730d23d..609b2bc 100644[m
[1m--- a/hardware/KEY.c[m
[1m+++ b/hardware/KEY.c[m
[36m@@ -1,7 +1,25 @@[m
 #include "stm32f10x.h"                  // Device header[m
 #include "Delay.h"[m
[32m+[m[32m#include "Serial.h"[m
[32m+[m[32m#include "FreeRTOS.h"[m
[32m+[m[32m#include "queue.h"[m
 [m
[31m-uint8_t kEY_Num;[m
[32m+[m[32mstatic QueueHandle_t KeyQueueHandle = NULL;[m
[32m+[m
[32m+[m[32mstatic uint32_t KeyQueueDropCount = 0;[m
[32m+[m
[32m+[m[32m/*------------------åˆ›å»ºé˜Ÿåˆ—-------------------*/[m
[32m+[m[32muint8_t KEY_QueueInit(void)[m
[32m+[m[32m{[m
[32m+[m[32m    KeyQueueHandle = xQueueCreate(8, sizeof(uint8_t));[m
[32m+[m
[32m+[m[32m    if (KeyQueueHandle == NULL)[m
[32m+[m[32m    {[m
[32m+[m[32m        return 0;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    return 1;[m
[32m+[m[32m}[m
 [m
 void KEY_Init(void)[m
 {[m
[36m@@ -18,20 +36,34 @@[m [mvoid KEY_Init(void)[m
 [m
 uint8_t kEY_GetNum(void)[m
 {[m
[31m-	uint8_t Temp;[m
[31m-	if (kEY_Num)[m
[31m-	{[m
[31m-		Temp=kEY_Num;[m
[31m-		kEY_Num=0;[m
[31m-		return Temp;[m
[31m-	}[m
[31m-	else [m
[31m-	{[m
[31m-		return 0;[m
[31m-	}[m
[31m-	[m
[32m+[m[32m    uint8_t key_num = 0;[m
[32m+[m
[32m+[m[32m    if (KeyQueueHandle == NULL)[m
[32m+[m[32m    {[m
[32m+[m[32m        return 0;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    xQueueReceive(KeyQueueHandle, &key_num, 0);[m
[32m+[m
[32m+[m[32m    return key_num;[m
 }[m
 [m
[32m+[m[32m// uint8_t kEY_GetNum(void)[m
[32m+[m[32m// {[m
[32m+[m[32m// 	uint8_t Temp;[m
[32m+[m[32m// 	if (kEY_Num)[m
[32m+[m[32m// 	{[m
[32m+[m[32m// 		Temp=kEY_Num;[m
[32m+[m[32m// 		kEY_Num=0;[m
[32m+[m[32m// 		return Temp;[m
[32m+[m[32m// 	}[m
[32m+[m[32m// 	else[m[41m [m
[32m+[m[32m// 	{[m
[32m+[m[32m// 		return 0;[m
[32m+[m[32m// 	}[m
[32m+[m[41m	[m
[32m+[m[32m// }[m
[32m+[m
 int press_time;[m
 void Key3_Tick(void)[m
 {[m
[36m@@ -78,9 +110,21 @@[m [mvoid Key_Tick(void)[m
 			Count=0;[m
 			PreState=CurrentState;[m
 			CurrentState=Key_GetState();[m
[31m-			if (PreState !=0 && CurrentState==0)[m
[32m+[m[41m		[m
[32m+[m
[32m+[m[41m		[m
[32m+[m			[32mif (PreState != 0 && CurrentState == 0)[m
 			{[m
[31m-				kEY_Num=PreState;[m
[32m+[m				[32muint8_t key_num = PreState;[m
[32m+[m
[32m+[m				[32mif (KeyQueueHandle != NULL)[m
[32m+[m				[32m{[m
[32m+[m					[32mif (xQueueSend([m
[32m+[m							[32mKeyQueueHandle,&key_num,0) != pdPASS)[m
[32m+[m					[32m{[m
[32m+[m						[32mKeyQueueDropCount++;[m
[32m+[m					[32m}[m
[32m+[m				[32m}[m
 			}[m
 		}[m
 }[m
\ No newline at end of file[m
[1mdiff --git a/hardware/KEY.h b/hardware/KEY.h[m
[1mindex 2f04d9a..324ba11 100644[m
[1m--- a/hardware/KEY.h[m
[1m+++ b/hardware/KEY.h[m
[36m@@ -9,5 +9,7 @@[m [muint8_t kEY_GetNum(void);[m
 void Key_Tick(void);[m
 void Key3_Tick(void);[m
 uint8_t Key_GetState(void);[m
[32m+[m[32muint8_t KEY_QueueInit(void);[m
[32m+[m
 [m
 #endif[m
[1mdiff --git a/hardware/MPU6050.c b/hardware/MPU6050.c[m
[1mindex 7e479d2..25e2bd7 100644[m
[1m--- a/hardware/MPU6050.c[m
[1m+++ b/hardware/MPU6050.c[m
[36m@@ -37,26 +37,59 @@[m [muint8_t MPU6050_readReg(uint8_t RegAddress)[m
 [m
 }[m
 [m
[31m-void MPU6050_Init(void)[m
[32m+[m[32muint8_t MPU6050_Init(void)[m
 {[m
[32m+[m[32m    uint8_t retry;[m
[32m+[m
     MyI2C_Init();[m
[32m+[m[32m    Delay_ms(300);[m
[32m+[m
[32m+[m[32m    /* ÉÏµçºóWHO_AM_I¿ÉÄÜµÚÒ»´Î¶ÁÈ¡²»ÎÈ¶¨£¬×î¶àÖØÊÔ5´Î */[m
[32m+[m[32m    for (retry = 0; retry < 5; retry++)[m
[32m+[m[32m    {[m
[32m+[m[32m        if (MPU6050_readReg(0x75) == 0x68)[m
[32m+[m[32m        {[m
[32m+[m[32m            break;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        Delay_ms(50);[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    if (retry >= 5)[m
[32m+[m[32m    {[m
[32m+[m[32m        return 0;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    /* Èí¼þ¸´Î»£¬¸´Î»ºóPWR_MGMT_1»áÖØÐÂ±ä³É0x40 */[m
[32m+[m[32m    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x80);[m
     Delay_ms(100);[m
 [m
[31m-    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x80); // ¸´Î»[m
[31m-    Delay_ms(100);[m
[32m+[m[32m    /* »½ÐÑ²¢»Ø¶ÁÑéÖ¤ */[m
[32m+[m[32m    for (retry = 0; retry < 3; retry++)[m
[32m+[m[32m    {[m
[32m+[m[32m        MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);[m
[32m+[m[32m        Delay_ms(20);[m
 [m
[31m-    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x00); // »½ÐÑ£¬ÏÈÓÃÄÚ²¿Ê±ÖÓ[m
[31m-    Delay_ms(100);[m
[32m+[m[32m        if ((MPU6050_readReg(MPU6050_PWR_MGMT_1) & 0x40) == 0)[m
[32m+[m[32m        {[m
[32m+[m[32m            break;[m
[32m+[m[32m        }[m
[32m+[m[32m    }[m
 [m
[31m-    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);[m
[31m-    Delay_ms(10);[m
[31m-    [m
[32m+[m[32m    if (retry >= 3)[m
[32m+[m[32m    {[m
[32m+[m[32m        return 0;[m
[32m+[m[32m    }[m
 [m
[32m+[m[32m    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);[m
     MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);[m
     MPU6050_WriteReg(MPU6050_CONFIG, 0x06);[m
[31m-    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x00); // ÏÈÓÃ +-2g[m
[31m-    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x00);  // ÏÈÓÃ +-250deg/s[m
[31m-        Delay_ms(100); // µÈ´ýÊý¾ÝÎÈ¶¨[m
[32m+[m[32m    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x00);[m
[32m+[m[32m    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x00);[m
[32m+[m
[32m+[m[32m    Delay_ms(100);[m
[32m+[m
[32m+[m[32m    return 1;[m
 }[m
 [m
 void MPU6050_GetData(int16_t *AccX, int16_t *ACCY, int16_t *AccZ,[m
[1mdiff --git a/hardware/MPU6050.h b/hardware/MPU6050.h[m
[1mindex bd56be8..bba4303 100644[m
[1m--- a/hardware/MPU6050.h[m
[1m+++ b/hardware/MPU6050.h[m
[36m@@ -5,7 +5,7 @@[m
 [m
 void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);[m
 uint8_t MPU6050_readReg(uint8_t RegAddress);[m
[31m-void MPU6050_Init(void);[m
[32m+[m[32muint8_t MPU6050_Init(void);[m
 void MPU6050_GetData(int16_t *AccX, int16_t *ACCY, int16_t *AccZ,[m
                        int16_t *GYOX, int16_t *GYOY, int16_t *GYOZ);[m
 [m
[1mdiff --git a/hardware/Power.c b/hardware/Power.c[m
[1mindex 2a00cd6..d06b48d 100644[m
[1m--- a/hardware/Power.c[m
[1m+++ b/hardware/Power.c[m
[36m@@ -128,7 +128,6 @@[m [mvoid Power_ClearWakeupKey(void)[m
 }[m
 [m
 [m
[31m-extern uint8_t kEY_Num;[m
 extern int press_time;[m
 [m
 void  Power_EnterStopMode(void)[m
[36m@@ -139,7 +138,7 @@[m [mvoid  Power_EnterStopMode(void)[m
     }[m
 [m
 [m
[31m-    Power_ClearWakeupKey();[m
[32m+[m[32m    // Power_ClearWakeupKey();[m
 [m
     if (Alarm_Enable == 1)[m
     {[m
[36m@@ -162,10 +161,13 @@[m [mvoid  Power_EnterStopMode(void)[m
 [m
     GPIO_ResetBits(GPIOB, GPIO_Pin_12);[m
 [m
[32m+[m[32m    Power_ClearWakeupKey();[m
[32m+[m
     SysTick->CTRL = 0;[m
     SysTick->LOAD = 0;[m
     SysTick->VAL  = 0;[m
 [m
[32m+[m
     PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);[m
 [m
     SystemInit();[m
[1mdiff --git a/project.uvprojx b/project.uvprojx[m
[1mindex 47c2a48..b29ca22 100644[m
[1m--- a/project.uvprojx[m
[1m+++ b/project.uvprojx[m
[36m@@ -314,7 +314,7 @@[m
           </ArmAdsMisc>[m
           <Cads>[m
             <interw>1</interw>[m
[31m-            <Optim>1</Optim>[m
[32m+[m[32m            <Optim>2</Optim>[m
             <oTime>0</oTime>[m
             <SplitLS>0</SplitLS>[m
             <OneElfS>1</OneElfS>[m
[1mdiff --git a/user/FreeRTOSConfig.h b/user/FreeRTOSConfig.h[m
[1mindex c84eb11..a77ad8b 100644[m
[1m--- a/user/FreeRTOSConfig.h[m
[1m+++ b/user/FreeRTOSConfig.h[m
[36m@@ -18,7 +18,6 @@[m
 #define configTOTAL_HEAP_SIZE                   ((size_t)(12 * 1024))[m
 #define configMAX_TASK_NAME_LEN                 16[m
 [m
[31m-#define INCLUDE_xTaskDelayUntil    1[m
 [m
 #define configPRIO_BITS                         4[m
 [m
[36m@@ -42,11 +41,15 @@[m
 #define configTIMER_QUEUE_LENGTH                5[m
 #define configTIMER_TASK_STACK_DEPTH            128[m
 [m
[32m+[m[32m#define INCLUDE_xTaskDelayUntil    1[m
[32m+[m
 #define INCLUDE_vTaskDelay                      1[m
 #define INCLUDE_vTaskDelete                     1[m
 #define INCLUDE_vTaskSuspend                    1[m
 #define INCLUDE_xTaskGetSchedulerState          1[m
 [m
[32m+[m[32m#define INCLUDE_uxTaskGetStackHighWaterMark    1[m
[32m+[m
 #define vPortSVCHandler                         SVC_Handler[m
 #define xPortPendSVHandler                      PendSV_Handler[m
 #define xPortSysTickHandler                     SysTick_Handler[m
[1mdiff --git a/user/main.c b/user/main.c[m
[1mindex 0e61a49..c103fa2 100644[m
[1m--- a/user/main.c[m
[1m+++ b/user/main.c[m
[36m@@ -34,6 +34,27 @@[m [mvolatile uint8_t malloc_failed_flag = 0;[m
  *   Ò³ÃæÄÚ²¿Í¨¹ýkEY_GetNum»ñÈ¡°´¼üÊÂ¼þ¡£[m
  */[m
 [m
[32m+[m
[32m+[m[32m    /*ÈÎÎñ¾ä±ú*/[m
[32m+[m[32m    static TaskHandle_t HeartbeatTaskHandle = NULL;[m
[32m+[m[32m    static TaskHandle_t KeyTaskHandle = NULL;[m
[32m+[m[32m    static TaskHandle_t TickTaskHandle = NULL;[m
[32m+[m[32m    static TaskHandle_t UITaskHandle = NULL;[m
[32m+[m
[32m+[m[41m    [m
[32m+[m[32m    /*¼ÇÂ¼¸÷ÈÎÎñÀúÊ·ÉÏ×îÉÙÊ£Óà¹ý¶àÉÙÕ»µÄ±äÁ¿*/[m
[32m+[m[32m    volatile UBaseType_t led_stack_free = 0;[m
[32m+[m[32m    volatile UBaseType_t key_stack_free = 0;[m
[32m+[m[32m    volatile UBaseType_t tick_stack_free = 0;[m
[32m+[m[32m    volatile UBaseType_t ui_stack_free = 0;[m
[32m+[m
[32m+[m[32m    /*¼ÇÂ¼µ±Ç°Ê£ÓàFreeRTOS¶Ñ¿Õ¼ä*/[m
[32m+[m[32m    volatile size_t heap_free = 0;[m
[32m+[m
[32m+[m[32m    /*Í³ÔËÐÐÒÔÀ´£¬¶Ñ¿Õ¼ä×îÉÙÊ£Óà¹ý¶àÉÙ*/[m
[32m+[m[32m    volatile size_t heap_min_free = 0;[m
[32m+[m
[32m+[m[32m    /*--------------------------------------¼ÆÊýÈÎÎñ-----------------------------------*/[m
 static void TickTask(void *param)[m
 {[m
     TickType_t last_wake_time;[m
[36m@@ -48,13 +69,12 @@[m [mstatic void TickTask(void *param)[m
         StopWatch_Tick();[m
         Dino_Tick();[m
 [m
[31m-        xTaskDelayUntil([m
[31m-            &last_wake_time,[m
[31m-            pdMS_TO_TICKS(1)[m
[31m-        );[m
[32m+[m[32m        xTaskDelayUntil(&last_wake_time,pdMS_TO_TICKS(1));[m
     }[m
 }[m
 [m
[32m+[m
[32m+[m[32m    /*--------------------------------------ÊÖ±íUIÈÎÎñ-----------------------------------*/[m
 static void UITask(void *param)[m
 {[m
     int clkflag1;[m
[36m@@ -82,6 +102,7 @@[m [mstatic void UITask(void *param)[m
     }[m
 }[m
 [m
[32m+[m[32m    /*--------------------------------------°´¼üÈÎÎñ-----------------------------------*/[m
 static void KeyTask(void *param)[m
 {[m
     uint8_t state;[m
[36m@@ -107,21 +128,112 @@[m [mstatic void KeyTask(void *param)[m
     }[m
 }[m
 [m
[32m+[m[32m/*---------------------------Êý×Ö´òÓ¡º¯Êý--------------------------------*/[m
[32m+[m[32mstatic void Monitor_PrintUint32(uint32_t value)[m
[32m+[m[32m{[m
[32m+[m[32m    char buffer[10];[m
[32m+[m[32m    uint8_t length = 0;[m
[32m+[m
[32m+[m[32m    if (value == 0)[m
[32m+[m[32m    {[m
[32m+[m[32m        Serial_SendByte('0');[m
[32m+[m[32m        return;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    while (value > 0)[m
[32m+[m[32m    {[m
[32m+[m[32m        buffer[length] = (char)('0' + value % 10);[m
[32m+[m[32m        length++;[m
[32m+[m[32m        value /= 10;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    while (length > 0)[m
[32m+[m[32m    {[m
[32m+[m[32m        length--;[m
[32m+[m[32m        Serial_SendByte((uint8_t)buffer[length]);[m
[32m+[m[32m    }[m
[32m+[m[32m}[m
[32m+[m
[32m+[m[32m/*---------------------------¼à¿Ø´òÓ¡½Ó¿Ú--------------------------------*/[m
[32m+[m[32mstatic void Monitor_PrintRuntimeInfo(void)[m
[32m+[m[32m{[m
[32m+[m[32m    Serial_Send_String("\r\n[RTOS Monitor]\r\n");[m
[32m+[m
[32m+[m[32m    Serial_Send_String("LED stack free: ");[m
[32m+[m[32m    Monitor_PrintUint32((uint32_t)led_stack_free);[m
[32m+[m[32m    Serial_Send_String(" words\r\n");[m
[32m+[m
[32m+[m[32m    Serial_Send_String("KEY stack free: ");[m
[32m+[m[32m    Monitor_PrintUint32((uint32_t)key_stack_free);[m
[32m+[m[32m    Serial_Send_String(" words\r\n");[m
[32m+[m
[32m+[m[32m    Serial_Send_String("TICK stack free: ");[m
[32m+[m[32m    Monitor_PrintUint32((uint32_t)tick_stack_free);[m
[32m+[m[32m    Serial_Send_String(" words\r\n");[m
[32m+[m
[32m+[m[32m    Serial_Send_String("UI stack free: ");[m
[32m+[m[32m    Monitor_PrintUint32((uint32_t)ui_stack_free);[m
[32m+[m[32m    Serial_Send_String(" words\r\n");[m
[32m+[m
[32m+[m[32m    Serial_Send_String("Heap free: ");[m
[32m+[m[32m    Monitor_PrintUint32((uint32_t)heap_free);[m
[32m+[m[32m    Serial_Send_String(" bytes\r\n");[m
[32m+[m
[32m+[m[32m    Serial_Send_String("Heap min free: ");[m
[32m+[m[32m    Monitor_PrintUint32((uint32_t)heap_min_free);[m
[32m+[m[32m    Serial_Send_String(" bytes\r\n");[m
[32m+[m[32m}[m
 [m
[32m+[m
[32m+[m[32m    /*--------------------------------------LEDÈÎÎñ£¨ÅÐ¶ÏÓÐÃ»ÓÐ½øµ÷¶ÈÆ÷µÄ£©ºÍÕ»¿Õ¼äÅÐ¶Ï-----------------------------------*/[m
 static void HeartbeatTask(void *param)[m
 {[m
[32m+[m[32m    uint8_t monitor_count = 0;[m
[32m+[m
[32m+[m[32m    (void)param;[m
[32m+[m
     while (1)[m
     {[m
         led_count++;[m
 [m
[32m+[m[32m        led_stack_free =[m
[32m+[m[32m            uxTaskGetStackHighWaterMark(HeartbeatTaskHandle);[m
[32m+[m
[32m+[m[32m        key_stack_free =[m
[32m+[m[32m            uxTaskGetStackHighWaterMark(KeyTaskHandle);[m
[32m+[m
[32m+[m[32m        tick_stack_free =[m
[32m+[m[32m            uxTaskGetStackHighWaterMark(TickTaskHandle);[m
[32m+[m
[32m+[m[32m        ui_stack_free =[m
[32m+[m[32m            uxTaskGetStackHighWaterMark(UITaskHandle);[m
[32m+[m
[32m+[m[32m        heap_free =[m
[32m+[m[32m            xPortGetFreeHeapSize();[m
[32m+[m
[32m+[m[32m        heap_min_free =[m
[32m+[m[32m            xPortGetMinimumEverFreeHeapSize();[m
[32m+[m
         LED0_ON();[m
         vTaskDelay(pdMS_TO_TICKS(50));[m
 [m
         LED0_OFF();[m
         vTaskDelay(pdMS_TO_TICKS(950));[m
[32m+[m
[32m+[m[32m        monitor_count++;[m
[32m+[m
[32m+[m[32m        if (monitor_count >= 5)[m
[32m+[m[32m        {[m
[32m+[m[32m            monitor_count = 0;[m
[32m+[m[32m            Monitor_PrintRuntimeInfo();[m
[32m+[m[32m        }[m
     }[m
[32m+[m
[32m+[m
 }[m
 [m
[32m+[m
[32m+[m[32m/*Õ»Òç³ö£¨´®¿Ú£©*/[m
 void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)[m
 {[m
     (void)xTask;[m
[36m@@ -135,6 +247,7 @@[m [mvoid vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)[m
     }[m
 }[m
 [m
[32m+[m[32m/*freertos¶ÑÄÚ´æµÄ¾¯¸æ£¨´®¿Ú£©*/[m
 void vApplicationMallocFailedHook(void)[m
 {[m
     malloc_failed_flag = 1;[m
[36m@@ -154,7 +267,20 @@[m [mint main(void)[m
 [m
     Peripheral_Init();[m
 [m
[31m-    create_ret = xTaskCreate(HeartbeatTask, "LED", 128, NULL, 1, NULL);[m
[32m+[m[32m    /*--------------------´´½¨¶ÓÁÐ --------------------*/[m
[32m+[m[32m    if (KEY_QueueInit() == 0)[m
[32m+[m[32m    {[m
[32m+[m[32m    Serial_Send_String("Create key queue failed\r\n");[m
[32m+[m
[32m+[m[32m    while (1)[m
[32m+[m[32m    {[m
[32m+[m[32m        LED0_ON();[m
[32m+[m[32m    }[m
[32m+[m[32m    }[m[41m   [m
[32m+[m
[32m+[m[32m    /*--------------------´´½¨ÈÎÎñ----------------------*/[m
[32m+[m
[32m+[m[32m    create_ret = xTaskCreate(HeartbeatTask, "LED", 128, NULL, 1, &HeartbeatTaskHandle);[m
     if (create_ret != pdPASS)[m
     {[m
         Serial_Send_String("Create LED task failed\r\n");[m
[36m@@ -165,7 +291,7 @@[m [mint main(void)[m
         }[m
     }[m
 [m
[31m-    create_ret = xTaskCreate(KeyTask, "KEY", 128, NULL, 2, NULL);[m
[32m+[m[32m    create_ret = xTaskCreate(KeyTask, "KEY", 128, NULL, 2, &KeyTaskHandle);[m
     if (create_ret != pdPASS)[m
     {[m
     Serial_Send_String("Create KEY task failed\r\n");[m
[36m@@ -176,7 +302,7 @@[m [mint main(void)[m
     }[m
     }[m
 [m
[31m-    create_ret = xTaskCreate(TickTask, "TICK", 128, NULL, 3, NULL);[m
[32m+[m[32m    create_ret = xTaskCreate(TickTask, "TICK", 128, NULL, 3, &TickTaskHandle);[m
     if (create_ret != pdPASS)[m
     {[m
         Serial_Send_String("Create TICK task failed\r\n");[m
[36m@@ -188,7 +314,7 @@[m [mint main(void)[m
     }[m
 [m
 [m
[31m-    create_ret = xTaskCreate(UITask, "UI", 1024, NULL, 1, NULL);[m
[32m+[m[32m    create_ret = xTaskCreate(UITask, "UI", 512, NULL, 1, &UITaskHandle);[m
     if (create_ret != pdPASS)[m
     {[m
         Serial_Send_String("Create UI task failed\r\n");[m
