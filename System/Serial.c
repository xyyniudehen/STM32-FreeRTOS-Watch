#include "stm32f10x.h"                  // Device header
#include "Serial.h"

/*
 * 初始化 USART1：PA9 为 TX，PA10 为 RX，115200-8-N-1。
 * 本项目使用查询方式收发，不使用中断、DMA和硬件流控。
 */
void Serial_Init(void)
{
    /* GPIOA 和 USART1 都挂在 APB2，总线时钟未开启时寄存器配置不会生效。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    /* PA9：复用推挽输出，由 USART1 外设驱动 TX 电平。 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA10：上拉输入，用于接收 USB-TTL 的 TX 数据。 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 115200 波特率、8 数据位、无校验、1 停止位，即常说的 115200-8-N-1。 */
    USART_InitTypeDef USART1_InitStructure;
    USART1_InitStructure.USART_BaudRate = 115200;
    USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART1_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
    USART1_InitStructure.USART_Parity = USART_Parity_No;
    USART1_InitStructure.USART_StopBits = USART_StopBits_1;
    USART1_InitStructure.USART_WordLength = USART_WordLength_8b; 
    USART_Init(USART1, &USART1_InitStructure);

    USART_Cmd(USART1, ENABLE);

}


/*
 * 发送一个原始字节。TXE 表示发送数据寄存器已空，可以写入下一字节；
 * 这里是阻塞等待，因此调用者会停在 while 中直到 USART 可继续发送。
 */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

/*
 * 逐字节发送以 '\0' 结尾的 C 字符串。
 * String 是首字符地址，函数通过 String[i] 依次读取，直到遇到结束符。
 */
void Serial_Send_String(char *String)
{
    uint8_t i ;
    for (i=0; String[i]!='\0'; i++)
    {
        Serial_SendByte(String[i]);
    }
}


/*
 * 非阻塞查询接收一个字节。
 * Byte 是输出参数，调用者必须传入变量地址（例如 &byte）；收到数据时通过
 * *Byte 写回并返回 1，没有数据时不修改目标变量并返回 0。
 */
uint8_t Serial_ReceiveByte(uint8_t *Byte)
{
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
        /* 解引用输出指针，把 USART 数据寄存器中的值写到调用者变量。 */
        *Byte = USART_ReceiveData(USART1);
        return 1;
    }
    return 0;
}
