#ifndef __SERIAL_H
#define __SERIAL_H

/* USART1 初始化：PA9/TX、PA10/RX、115200-8-N-1。 */
void Serial_Init(void);
/* 发送一个字节。 */
void Serial_SendByte(uint8_t Byte);

/* 查询接收；Byte 为输出地址，收到返回 1，未收到返回 0。 */
uint8_t Serial_ReceiveByte(uint8_t *Byte);

/* 发送以 '\0' 结尾的字符串。 */
void Serial_Send_String(char *String);

/* buffer容量必须至少为length字节 */
uint8_t Serial_DMAReceiveStart(uint8_t *buffer, uint16_t length);

/* 成功返回1，超时或接收错误返回0 */
uint8_t Serial_DMAReceiveWait(uint32_t timeout_count);

#include <stdint.h>

/* 接收接口仍返回1/0，具体失败原因通过此类型查询 */
typedef enum
{
    SERIAL_RX_ERROR_NONE = 0,
    SERIAL_RX_ERROR_PARAMETER,
    SERIAL_RX_ERROR_BUSY,
    SERIAL_RX_ERROR_NOT_STARTED,
    SERIAL_RX_ERROR_TIMEOUT,
    SERIAL_RX_ERROR_DMA,
    SERIAL_RX_ERROR_UART
} Serial_RxError_t;

Serial_RxError_t Serial_DMAGetLastError(void);

/* 跳转APP前关闭Boot使用过的DMA接收资源 */
void Serial_DMADeInit(void);


#endif // !__SERIAL_H


