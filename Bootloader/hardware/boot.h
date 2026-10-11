#include "stm32f10x.h"                  // Device header
#ifndef __BOOT_H
#define __BOOT_H

/*
 * Bootloader 占用 0x08000000~0x08003FFF（16 KB），
 * APP 从 0x08004000 开始链接和存放。APP 工程的 IROM 起始地址必须一致。
 */
#define APP_ADDR 0x08004000

/* 检查 APP 初始栈地址；返回 1 表示基本有效，返回 0 表示无效。 */
uint8_t Boot_CheckAppValid(void);

/* 切换向量表、MSP 和入口地址后跳转执行 APP。 */
void Boot_JumpToApp(void);


/* DR1用于RTC初始化标记；DR2用于升级请求 */
#define BOOT_REQUEST_REGISTER  BKP_DR2
#define BOOT_REQUEST_MAGIC     0xB007U

uint8_t Boot_IsUpdateRequested(void);
void Boot_ClearUpdateRequest(void);

/* 检查启动时是否按住KEY3，按住返回1 */
uint8_t Boot_IsForceUpdateKeyPressed(void);

#endif