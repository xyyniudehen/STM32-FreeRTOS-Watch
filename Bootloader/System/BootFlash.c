#include "stm32f10x.h"
#include "BootFlash.h"
#include "stm32f10x_flash.h"

/*
 * 擦除能够容纳 firmware_size 的 APP Flash 页。
 * Flash 擦除单位是“页”而不是字节，因此最后不足一页也必须整页擦除。
 */
uint8_t BootFlash_EraseApp(uint32_t firmware_size)
{
    uint32_t page_count;
    uint32_t erase_address;
    uint32_t erase_end;
    FLASH_Status flash_status;

    /* 拒绝 0 字节和超过 48 KB APP 分区的固件，防止擦到分区外。 */
    if (firmware_size ==0U ||firmware_size > APP_FLASH_SIZE)
    {
        return 0;
    }

    /*
     * 整数除法向上取整公式。例如 1025 字节需要 2 页，而不是 1 页。
     * 常量后的 U 表示无符号整数，避免有符号运算和地址计算产生歧义。
     */
    page_count = (firmware_size + FLASH_PAGE_SIZE -1U)/FLASH_PAGE_SIZE;

    erase_address = APP_FLASH_START;
    erase_end = APP_FLASH_START +page_count * FLASH_PAGE_SIZE;

    /* Flash 上电后默认锁定，擦写前解锁控制器。 */
    FLASH_Unlock();

    /*
     * 清除上一次操作遗留的完成、编程错误和写保护错误标志。
     * 按位或 | 用于把多个独立标志位组合成一个参数。
     */
    FLASH_ClearFlag(FLASH_FLAG_EOP |FLASH_FLAG_PGERR |FLASH_FLAG_WRPRTERR);

    while(erase_address < erase_end)
    {
        /* 每次擦除 1 KB 页，擦除后该页通常为全 0xFF。 */
        flash_status = FLASH_ErasePage(erase_address);

        if (flash_status !=FLASH_COMPLETE)
        {
            FLASH_Lock();
            return 0;
        }
        erase_address +=FLASH_PAGE_SIZE;
    }
    FLASH_Lock();
    return 1;

}

/*
 * 将 RAM 缓冲区写入 APP Flash。
 * address 是目标 Flash 地址；data 是源缓冲区首地址；length 是有效字节数。
 * const 约束的是“不能通过 data 修改源数据”，指针本身仍可用于向后索引。
 */
uint8_t BootFlash_Write(uint32_t address, const uint8_t *data, uint32_t length)
{
    uint32_t i;
    uint16_t half_word;
    FLASH_Status flash_status;

    /* 空指针或 0 长度都没有可写内容，直接判为失败。 */
    if (data ==0 || length ==0U)
    {
        return 0;
    }
    
    /* 起始地址必须位于 APP 分区，禁止覆盖 0x08000000 的 Bootloader。 */
    if (address <APP_FLASH_START || address>=APP_FLASH_END)
    {
        return 0;
    }

    /* STM32F1 按 16 位半字编程，写地址必须是 2 字节对齐的偶数地址。 */
    if ((address & 0x1U) != 0U)
    {
        return 0;
    }

    /* 检查 address + length 不越过 APP 分区末尾，同时避免加法溢出。 */
    if (length >APP_FLASH_END - address)
    {
        return 0;
    }

    FLASH_Unlock();

    FLASH_ClearFlag(FLASH_FLAG_EOP |FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    /* 每次组合并写入 2 字节半字，因此索引步长是 2，而不是 20。 */
    for (i=0; i<length; i+=2U)
    {
        /* 小端序：data[i] 放低 8 位，下一字节放高 8 位。 */
        half_word =data[i];

        if (i+1U<length)
        {
            /* 转为 uint16_t 后左移 8 位，再用 | 合并为一个 16 位半字。 */
            half_word |=((uint16_t) (data[i+1U]) <<8);
        }
        else
        {
            /* 最后一包为奇数字节时，高字节补 0xFF（Flash 擦除态）。 */
            half_word|= 0xFF00U;
        }
    
        /* 通过 Flash 控制器编程；Flash 不能像 RAM 一样直接赋值写入。 */
        flash_status = FLASH_ProgramHalfWord(address+i, half_word);
        if (flash_status != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return 0;
        }

        /*
         * Flash 是存储器映射的，可把目标地址转换成指针后直接回读。
         * 回读值与 half_word 不同，说明本次编程未按预期落入 Flash。
         */
        if (*(__IO uint16_t *)(address+i) != half_word)
        {
            FLASH_Lock();
            return 0;

        }
    }
    FLASH_Lock();
    return 1;

}
