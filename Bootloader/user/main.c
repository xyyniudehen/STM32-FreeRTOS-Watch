#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Timer.h"
#include "KEY.h"
#include "MPU6050.h"
#include "Serial.h"
#include "boot.h"
#include "BootUpdate.h"


/* 独立RAM搬运测试；回显用于比较接收内容，此阶段不写Flash。 */
static void Boot_DMAReceiveTest(void)
{
    /* 静态数组在SRAM中持续存在，容量10字节，初值默认全0。 */
    static uint8_t buffer[10];
    uint8_t i;

    /* 数组名传首地址；这里sizeof作用于真实数组，结果是10。
     * if先执行启动函数，再判断返回值；启动失败就停止测试。 */
    if (Serial_DMAReceiveStart(buffer, sizeof(buffer)) == 0U)
    {
        Serial_Send_String("DMA START ERROR\r\n");
        return;
    }

    /* DMA已经准备好，电脑看到这句再发送 */
    Serial_Send_String("DMA TEST READY: send 10 bytes\r\n");

    /* 等待完整10字节；返回0可能是超时或错误，因此使用合并错误提示。 */
    if (Serial_DMAReceiveWait(500000U) == 0U)
    {
        Serial_Send_String("DMA DATA TIMEOUT OR ERROR\r\n");
        return;
    }

    Serial_Send_String("DMA RECEIVE OK: ");

    /* 回发RAM中已接收的10个字节，以验证内容和顺序。
     * 按长度发送而非按字符串发送：数组不保证存在结束符，且可包含0字节。 */
    for (i = 0; i < sizeof(buffer); i++)
    {
        /* 接收使用DMA，测试回显仍使用原来的UART轮询发送。 */
        Serial_SendByte(buffer[i]);
    }

    Serial_Send_String("\r\n");
}

//
// * 调试辅助函数：读取 RCC 中锁存的复位来源并清除标志。
// * static 表示只在 main.c 内可见；当前调用被注释，不参与正式升级流程。
 //*/

static void Boot_PrintResetCause(void)
{
    if (RCC_GetFlagStatus(RCC_FLAG_PINRST) == SET)
    {
        Serial_Send_String("RESET: PIN\r\n");
    }

    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) == SET)
    {
        Serial_Send_String("RESET: POWER\r\n");
    }

    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) == SET)
    {
        Serial_Send_String("RESET: SOFTWARE\r\n");
    }

    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET)
    {
        Serial_Send_String("RESET: IWDG\r\n");
    }

    if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) == SET)
    {
        Serial_Send_String("RESET: WWDG\r\n");
    }

    if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) == SET)
    {
        Serial_Send_String("RESET: LOW POWER\r\n");
    }

    /* 复位标志会累积保持，必须在全部读取完成后再统一清除。 */
    RCC_ClearFlag();
}

static void Boot_OLEDTest(void)
{
    OLED_Init();
    OLED_Clear();

    OLED_ShowString(16, 0, "BOOT OLED", OLED_8X16);
    OLED_ShowString(16, 16, "SPI TEST OK", OLED_8X16);

    /* 外框：100像素宽，12像素高 */
    OLED_DrawRectangle(14, 40, 100, 12, OLED_UNFILLED);

    /* 内部填充一半，模拟50%进度 */
    OLED_DrawRectangle(16, 42, 48, 8, OLED_FILLED);

    OLED_ShowString(52, 56, "50%", OLED_6X8);

    /* 绘图只修改RAM显存，此处才发送到屏幕 */
    OLED_Update();
}

int main(void)
{
    uint8_t byte;
    uint8_t update_requested;
    uint8_t force_update;

    /*
     * 优先读取请求。
     * 这里只开放BKP访问，不初始化RTC、不清备份域。
     */
    // Serial_Init();
    // Boot_PrintResetCause();
    // Serial_Send_String("BOOT ENTRY\r\n");

    // update_requested = Boot_IsUpdateRequested();

    force_update = Boot_IsForceUpdateKeyPressed();

    update_requested = Boot_IsUpdateRequested();


    if (force_update != 0U)
    {
        update_requested = 1U;
    }
    else if (update_requested != 0U)
    {
        Boot_ClearUpdateRequest();
    }
    else if (Boot_CheckAppValid() != 0U)
    {
        /*
         * 正常启动直接进入APP。
         * Serial_Init打开DMA时钟，供跳转清理接口使用。
         */
        Serial_Init();
        Boot_JumpToApp();

        /* 正常跳转不应返回 */
        while (1)
        {
        }
    }


    Serial_Init();

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(16, 16, "Bootloader", OLED_8X16);
    OLED_ShowString(16, 40, "Waiting...", OLED_8X16);
    OLED_Update();

    Serial_Send_String("Bootloader Start\r\n");

    if (force_update != 0U)
    {
        Serial_Send_String("Force update key\r\n");
    }
    else if (update_requested != 0U)
    {
        Serial_Send_String("Update requested\r\n");
    }
    else
    {
        Serial_Send_String("No Valid App\r\n");
    }

    Serial_Send_String("Waiting for U\r\n");

    /* 升级模式中持续等待上位机，不再限5秒 */
    while (1)
    {
        if (Serial_ReceiveByte(&byte) == 1U)
        {
            if (byte == 'U')
            {
                Serial_Send_String("Update Mode\r\n");

                if (BootUpdate_Start() == 1U)
                {
                    Serial_Send_String("UPDATE SUCCESS\r\n");

                    /* 保留当前设计：成功后由用户复位启动APP */
                    while (1)
                    {
                    }
                }

                Serial_Send_String("UPDATE FAIL\r\n");
                Serial_Send_String("Waiting for U\r\n");
            }
        }

        Delay_ms(1);
    }
}


