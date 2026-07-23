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


#endif // !__SERIAL_H


