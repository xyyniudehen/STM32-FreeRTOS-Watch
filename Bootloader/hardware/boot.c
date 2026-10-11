#include "stm32f10x.h"                  // Device header
#include "boot.h"
#include "Serial.h"
#include "BootFlash.h"
#include "Serial.h"

/*
 * pFunction 是函数指针类型：指向“无参数、无返回值”的函数。
 * APP 向量表第 2 个 32 位数据保存 Reset_Handler 地址，读出后需要
 * 转换成该函数指针类型，CPU 才能通过 app_entry() 跳到 APP 执行。
 * 注意：函数指针是“保存函数地址的指针”，不是“返回指针的函数”。
 */
typedef void (*pFunction)(void);

uint8_t Boot_IsForceUpdateKeyPressed(void)
{
    GPIO_InitTypeDef gpio;

    /*
     * PA15默认复用为JTAG功能。
     * 关闭JTAG，保留SWD，既释放PA15又不影响调试下载。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO,ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    /*
     * KEY3采用上拉输入：
     * 未按下 = 1
     * 按下   = 0
     */
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_15) == Bit_RESET)
    {
        return 1U;
    }

    return 0U;
}

static void Boot_EnableBackupAccess(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
}

uint8_t Boot_IsUpdateRequested(void)
{
    Boot_EnableBackupAccess();

    return (BKP_ReadBackupRegister(BOOT_REQUEST_REGISTER)== BOOT_REQUEST_MAGIC) ? 1U : 0U;
}

void Boot_ClearUpdateRequest(void)
{
    Boot_EnableBackupAccess();

    BKP_WriteBackupRegister(BOOT_REQUEST_REGISTER, 0U);
}



/*------------------------------------------------------------*/
 /* 检查 APP 向量表中的初始栈顶地址是否落在 SRAM 地址段。
 * 返回 1 表示具备基本可启动条件，返回 0 表示 APP 不存在或向量表异常。
 * 这里的 1/0 只是函数返回给调用者的判断结果，不会直接让 CPU 跳转。
 /*------------------------------------------------------------*/
uint8_t Boot_CheckAppValid(void)
{
    uint32_t app_stack;
    uint32_t app_reset;
    uint32_t reset_address;

    app_stack = *(__IO uint32_t *)APP_ADDR;
    app_reset = *(__IO uint32_t *)(APP_ADDR + 4U);

    /*
     * STM32F103C8：SRAM为20KiB。
     * 栈向低地址增长，初始栈顶允许等于SRAM末尾后一地址。
     */
    if ((app_stack <= 0x20000000U) ||
        (app_stack > 0x20005000U))
    {
        return 0U;
    }

    /* 初始栈顶要求8字节对齐 */
    if ((app_stack & 0x7U) != 0U)
    {
        return 0U;
    }

    /* Cortex-M复位入口必须带Thumb状态位 */
    if ((app_reset & 0x1U) == 0U)
    {
        return 0U;
    }

    /* 去掉Thumb位后，检查实际代码地址 */
    reset_address = app_reset & ~0x1U;

    if ((reset_address < APP_FLASH_START) ||
        (reset_address >= APP_FLASH_END))
    {
        return 0U;
    }

    return 1U;
}


/*
 * 按 Cortex-M3 向量表规则跳转到 APP：
 * 1. 读取 APP 初始 MSP；2. 读取 APP Reset_Handler；3. 清理 Boot 状态；
 * 4. 重定位向量表；5. 设置 MSP；6. 通过函数指针执行 APP 入口。
 */
void Boot_JumpToApp(void)
{
    uint32_t app_stack;
    uint32_t app_reset;
    pFunction app_entry;

    /* 向量表第 1 项是初始 MSP，第 2 项（偏移 4 字节）是 Reset_Handler。 */
    app_stack = *(__IO uint32_t *)APP_ADDR;
    app_reset = *(__IO uint32_t *)(APP_ADDR+4U);

    /*
     * 跳转准备期间禁止全局中断，避免 Bootloader 的中断在切换向量表和栈时触发。
     * PRIMASK 不会因为普通函数跳转自动恢复，APP 侧需要确保重新使能中断。
     */
    __disable_irq();

    Serial_DMADeInit();

    /*
     * 关闭并清空 Bootloader 使用过的 SysTick，防止 APP 刚启动就收到旧节拍。
     * CTRL 控制使能和中断，LOAD 是重装值，VAL 是当前计数值。
     */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /*
     * VTOR 告诉内核“中断向量表现在从 APP_ADDR 开始”；它是运行时寄存器。
     * __set_MSP 把主栈指针切换到 APP 向量表给出的栈顶，随后 APP 的
     * Reset_Handler 才能按自己的栈环境运行。
     */
    SCB->VTOR = APP_ADDR;
    __set_MSP(app_stack);

    /* 将 32 位入口地址解释成函数指针；调用后开始执行 APP Reset_Handler。 */
    app_entry = (pFunction)app_reset;

    __enable_irq();
    app_entry();
}