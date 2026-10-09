#include "stm32f10x.h"                  // Device header
#include "Serial.h"
#include "stm32f10x_dma.h"
#include "Delay.h"

/* 文件内接收状态：0为空闲，1为已启动；static限制其他文件直接访问。 */
static uint8_t Serial_DMA_RxActive = 0U;

/*
 * 初始化 USART1：PA9 为 TX，PA10 为 RX，115200-8-N-1。
 * 本项目使用查询方式收发，不使用中断、DMA和硬件流控。
 */
void Serial_Init(void)
{
    /* GPIOA 和 USART1 都挂在 APB2，总线时钟未开启时寄存器配置不会生效。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* DMA1位于AHB总线；打开时钟不等于已经开始搬运数据。 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

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

/* 结束接收，并清除DMA历史标志 */
/* 私有清理函数：关闭请求和通道，清标志，并恢复软件空闲状态。 */
static void Serial_DMAReceiveStop(void)
{
    USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
    DMA_Cmd(DMA1_Channel5, DISABLE);
    /* GL5清除通道5的完成、半完成和错误等历史标志。 */
    DMA_ClearFlag(DMA1_FLAG_GL5);

    Serial_DMA_RxActive = 0U;
}


/* 配置并启动搬运，返回1仅表示启动成功，不表示已经收满。
 * buffer为RAM目的地址，数组容量至少为length，等待完成前必须保持有效。 */
uint8_t Serial_DMAReceiveStart(uint8_t *buffer, uint16_t length)
{
    DMA_InitTypeDef dma;
    /* dummy承接寄存器读值；volatile保留读值写入，数据随后丢弃。 */
    volatile uint32_t dummy;

    /* 空指针不能作为RAM目的地址；零长度无有效传输；正在接收时禁止覆盖配置。
     * 0U是无符号零；这里检查地址和长度，不检查数组内容是否为零。 */
    if ((buffer == 0) || (length == 0U) ||
        (Serial_DMA_RxActive != 0U))
    {
        return 0;
    }

    Serial_DMAReceiveStop();
    /* USART1_RX映射DMA1通道5，关闭通道后恢复其默认配置。 */
    DMA_DeInit(DMA1_Channel5);

    /*
     * 清除上次残留的接收数据和UART错误状态。
     * 必须在通知电脑发送之前执行，不能在正常接收中途清。
     */
    dummy = USART1->SR;
    dummy = USART1->DR;
    (void)dummy;

    /* 用默认值初始化配置结构体；传入地址，使库函数能修改结构体字段。 */
    DMA_StructInit(&dma);
    /* 源地址固定为USART1接收数据寄存器；地址转成库接口要求的32位整数。 */
    dma.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    /* 目的地址为调用者数组的首地址，接收数据将实际写入这块SRAM。 */
    dma.DMA_MemoryBaseAddr = (uint32_t)buffer;
    /* 外设为源：数据从UART寄存器搬向内存。 */
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    /* 设置传输元素数量；本例每元素1字节，所以length就是字节数。 */
    dma.DMA_BufferSize = length;
    /* 源地址不递增：每次都读取同一个USART1->DR。 */
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    /* 目的地址递增：依次填buffer[0]、buffer[1]等，避免覆盖同一元素。 */
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    /* UART为8位数据帧，DMA每次从DR读取一个字节。 */
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    /* RAM每次写一个字节，与uint8_t数组元素宽度一致。 */
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    /* 普通模式：计数降至0后停止新的搬运；通道关闭和下次重装由软件完成。 */
    dma.DMA_Mode = DMA_Mode_Normal;
    /* DMA通道仲裁优先级；与NVIC中断优先级、FreeRTOS任务优先级不同。 */
    dma.DMA_Priority = DMA_Priority_High;
    /* 关闭内存到内存模式，按USART的外设请求触发搬运。 */
    dma.DMA_M2M = DMA_M2M_Disable;

    /* 将上述结构体写入通道5寄存器；此时尚未使能通道。 */
    DMA_Init(DMA1_Channel5, &dma);

    /* 先准备好DMA，再允许USART产生DMA请求 */
    Serial_DMA_RxActive = 1U;
    DMA_Cmd(DMA1_Channel5, ENABLE);
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

    return 1;
}

/* 查询硬件状态等待收满；CPU仍阻塞等待，逐字节搬运由DMA完成。
 * timeout_count为轮询次数，并非精确毫秒。
 * 错误和超时统一返回0；调用者负责打印和决定是否继续升级。 */
uint8_t Serial_DMAReceiveWait(uint32_t timeout_count)
{
    /* 未启动接收时不等待，直接报告失败。 */
    if (Serial_DMA_RxActive == 0U)
    {
        return 0;
    }

    while (timeout_count > 0U)
    {
        /* DMA搬运错误或UART接收错误，结束本次接收 */
        /* TE为DMA传输错误；ORE溢出、FE帧错误、NE噪声、PE校验错误。 */
        if ((DMA_GetFlagStatus(DMA1_FLAG_TE5) == SET) ||
            ((USART1->SR & (USART_SR_ORE | USART_SR_FE |
                           USART_SR_NE | USART_SR_PE)) != 0U))
        {
            Serial_DMAReceiveStop();
            return 0;
        }

        /* TC5表示已搬完指定数量，不需要用户编写DMA中断服务函数。 */
        if (DMA_GetFlagStatus(DMA1_FLAG_TC5) == SET)
        {
            Serial_DMAReceiveStop();
            /* 内存屏障约束访问顺序；DMA是否完成由上面的TC5判定。 */
            __DMB();
            return 1;
        }

        /* 轮询间隔仍是CPU忙等待，减少查询次数，不会让CPU自动休眠。 */
        Delay_us(10);
        timeout_count--;
    }

    /* 次数耗尽：终止接收，避免超时后DMA仍继续改写数组。 */
    Serial_DMAReceiveStop();
    return 0;
}