#include "stm32f10x.h"                  // Device header
#ifndef __BOOTFLASH_H
#define __BOOTFLASH_H


/* APP 可写区域：[0x08004000, 0x08010000)，总计 0xC000 = 48 KB。 */
#define APP_FLASH_START 0x08004000U
#define APP_FLASH_END   0x08010000U
#define APP_FLASH_SIZE  0x0000C000U

/* STM32F103C8 中容量 Flash 的页大小为 0x400 = 1024 字节。 */
#define FLASH_PAGE_SIZE   0x00000400U

/* 按固件大小向上取整页数并擦除 APP 区；成功返回 1，失败返回 0。 */
uint8_t BootFlash_EraseApp(uint32_t firmware_size);

/*
 * 将 data 指向的 length 字节写入 address。
 * const 表示函数只读取 data 指向的数据，不允许通过该指针修改发送缓冲区。
 */
uint8_t BootFlash_Write(uint32_t address, const uint8_t *data, uint32_t length);

void BootUpdate_PrintReceiveError(void);

#endif