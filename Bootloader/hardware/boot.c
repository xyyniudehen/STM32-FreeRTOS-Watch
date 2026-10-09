#include "stm32f10x.h"                  // Device header
#include "boot.h"

/*
 * pFunction 是函数指针类型：指向“无参数、无返回值”的函数。
 * APP 向量表第 2 个 32 位数据保存 Reset_Handler 地址，读出后需要
 * 转换成该函数指针类型，CPU 才能通过 app_entry() 跳到 APP 执行。
 * 注意：函数指针是“保存函数地址的指针”，不是“返回指针的函数”。
 */
typedef void (*pFunction)(void);

/*
 * 检查 APP 向量表中的初始栈顶地址是否落在 SRAM 地址段。
 * 返回 1 表示具备基本可启动条件，返回 0 表示 APP 不存在或向量表异常。
 * 这里的 1/0 只是函数返回给调用者的判断结果，不会直接让 CPU 跳转。
 */
uint8_t Boot_CheckAppValid(void)
{
    uint32_t app_stack;

    /*
     * Flash 是存储器映射的：APP_ADDR 可像普通地址一样被 CPU 直接读取。
     * 向量表第 1 个字（APP_ADDR + 0）是 APP 启动时应装入 MSP 的栈顶地址。
     * __IO 带有 volatile 语义，要求编译器每次都真正访问该地址。
     */
    app_stack = *(__IO uint32_t *)APP_ADDR;

    /*
     * STM32F103 的 SRAM 从 0x20000000 开始。掩码保留高位，用于排除
     * 0xFFFFFFFF（Flash 擦除态）、0x00000000 等明显无效的栈地址。
     */
    if ((app_stack & 0x2FFE0000) == 0x20000000)
    {
        return 1;
    }
    return 0;

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