#ifndef __BOOTUPDATE_H
#define __BOOTUPDATE_H

#include "stm32f10x.h"

/* 固件头前 2 字节固定为 0x55、0xAA，用于识别升级协议。 */
#define BOOT_MAGIC_0       0x55U
#define BOOT_MAGIC_1       0xAAU

/* 固件头 = 2 字节魔数 + 4 字节固件大小 + 4 字节 CRC32，共 10 字节。 */
#define BOOT_HEADER_SIZE   10U

/* 每次只用 256 字节 RAM 缓冲固件，避免把整个约 40 KB 文件装入 SRAM。 */
#define BOOT_PACKET_SIZE   256U
/* 执行一次完整升级；全部接收、写入和 CRC 校验成功时返回 1。 */
uint8_t BootUpdate_Start(void);

#endif