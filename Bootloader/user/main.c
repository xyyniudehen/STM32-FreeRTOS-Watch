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
        Serial_Send_String("DMA TIMEOUT OR ERROR\r\n");
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

int main(void)
{
    uint8_t byte;

    /* Bootloader 首先初始化串口，后续握手和错误信息都依赖 USART1。 */
    Serial_Init();

    // /* 临时测试入口；下面无限循环使原有Boot升级和APP跳转暂不执行。 */
    // Boot_DMAReceiveTest();

    // while (1)
    // {
    // }

    /* 仅排查复位来源时取消下一行注释；正式功能可保持关闭。 */
    // Boot_PrintResetCause();

    Serial_Send_String("Bootloader Start\r\n");

    uint32_t wait_ms;

Serial_Send_String("Send U within 5 seconds\r\n");

/*
 * 轮询约 5000 次，每轮末尾延时 1 ms，因此升级入口窗口约为 5 秒。
 * 收到 ASCII 字符 U（数值 0x55）才进入升级；否则超时后尝试启动 APP。
 */
for (wait_ms = 0; wait_ms < 5000; wait_ms++)
{
    if (Serial_ReceiveByte(&byte) == 1)
    {
        Serial_Send_String("Receive: ");
        Serial_SendByte(byte);
        Serial_Send_String("\r\n");

        /* Python 脚本发送 b"U"，Serial_ReceiveByte 通过 &byte 写回后在此判断。 */
		if (byte == 'U')
		{
			Serial_Send_String("Update Mode\r\n");

            /* if 中会先调用升级函数，再把它的返回值与 1 比较。 */
			if (BootUpdate_Start() == 1)
			{
				Serial_Send_String("UPDATE SUCCESS\r\n");
			}
			else
			{
				Serial_Send_String("HEADER TEST FAIL\r\n");
			}

            /* 升级流程结束后停在 Bootloader；当前设计需要复位后再启动 APP。 */
			while (1)
			{
			}
		}

	}
    Delay_ms(1);
}

    Serial_Send_String("Jump App\r\n");

    /*
     * 未收到升级命令时检查 APP 初始栈地址。有效则切换 VTOR、MSP 和入口；
     * Boot_JumpToApp() 正常情况下不会返回。
     */
    if (Boot_CheckAppValid())
    {
        Boot_JumpToApp();
    }

    Serial_Send_String("No Valid App\r\n");

    while (1)
    {
    }
}


// void TIM2_IRQHandler(void)
// {

// 	if (TIM_GetITStatus(TIM2, TIM_IT_Update)==SET)
// 	{
// 		Key3_Tick();
// 		Key_Tick();
// 		StopWatch_Tick();
// 		Dino_Tick();
// 		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
// 	}	
	
	

// }