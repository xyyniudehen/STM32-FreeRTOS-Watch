#include "stm32f10x.h"
#include "BootUpdate.h"
#include "Serial.h"
#include "Delay.h"
#include "BootFlash.h"

/*
 * static 放在文件作用域时，表示该符号只在 BootUpdate.c 内可见，
 * 不会与其他文件的同名变量冲突。数组位于 RAM，升级期间反复复用为 256 字节包缓冲区。
 */
static uint8_t BootUpdate_Packet[BOOT_PACKET_SIZE];

/*
 * 计算与 Python zlib.crc32() 一致的标准 CRC32。
 * static 表示该辅助函数只供本文件内部调用；const 表示只读取 data 指向的数据。
 * 传入 APP_FLASH_START 时，data 直接指向存储器映射的 Flash，不占用整包 RAM。
 */
static uint32_t BootUpdate_CRC32(const uint8_t *data, uint32_t length)
{
    uint32_t crc;
    uint32_t i;
    uint32_t bit;

    /* 标准 CRC32 初始值。CRC 用于检错，不是加密或身份认证。 */
    crc = 0xFFFFFFFFU;
    
    for(i = 0U; i<length; i++)
    {
        /* 先把当前字节异或进 CRC 低 8 位，再逐位完成多项式除法。 */
        crc^=data[i];

        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc&1U)!=0U)
            {
                /*
                 * 最低位为 1 时，右移后异或反射形式的 CRC32 多项式。
                 * 0xEDB88320 是 0x04C11DB7 按位反射后的表示，匹配 zlib 的位序。
                 */
                crc = (crc>>1U) ^ 0xEDB88320U;
            }
            else
            {
                /* 最低位为 0 时只右移；必须写回 crc，单写 crc >> 1U 不会改变变量。 */
                crc = crc >> 1U;
            }
        }

    }
    /* 标准 CRC32 最终异或，返回值应与电脑端 firmware_crc 一致。 */
    return crc ^ 0xFFFFFFFFU;
}

/*
 * 在有限 轮询 次数内接收 1 字节。byte 是输出地址，timeout_counnt 是最多轮询次数；
 * 每轮延时约 10 us，因此它是“次数”而不是直接的毫秒数。
 */
static uint8_t BootUpdate_ReceiveByteTimeout(uint8_t *byte, uint32_t timeout_counnt)
{
    while (timeout_counnt--)
    {
        if (Serial_ReceiveByte(byte) ==1)
        {
            return 1;
        }
        Delay_us(10);
    }
    return 0;
}

/*
 * 从 data[0..3] 解析一个小端 uint32_t。
 * 小端格式把最低有效字节放在最低地址，例如 41756 为 1C A3 00 00。
 */
static uint32_t BootUpdate_GetUint32LE(uint8_t *data)
{
    uint32_t value;
    /* 4 个字节分别放入 32 位结果的 bit[7:0]、[15:8]、[23:16]、[31:24]。 */
    value = (uint32_t) data[0];
    value |= (uint32_t)data[1]<<8;
    value |= (uint32_t)data[2]<<16;
    value |= (uint32_t)data[3]<<24;

    return value;
}

/*
 * 接收并解析 10 字节固件头。
 * firmware_size、firmware_crc 都是输出指针，函数通过 * 指针把解析结果
 * 写回 BootUpdate_Start() 中的同名变量，因此调用时必须传 &变量。
 */
static uint8_t BootUpdate_ReceiveHeader(uint32_t *firmware_size, uint32_t *firmware_crc)
{
    /* 局部 10 字节头缓冲区位于当前函数栈中，函数返回后不再使用。 */
    uint8_t header[BOOT_HEADER_SIZE];
    uint8_t i;

    for (i=0; i<BOOT_HEADER_SIZE; i++)
    {
        if(BootUpdate_ReceiveByteTimeout(&header[i], 1000000) ==0)
        {
            Serial_Send_String("HEADER TIMEOUT\r\n");
            return 0;
        }
    }

    /* 前两字节必须依次是 0x55、0xAA，否则当前数据流不是有效固件头。 */
    if (header[0] != BOOT_MAGIC_0|| header[1] !=BOOT_MAGIC_1)
    {
        Serial_Send_String("MAGIC ERROR\r\n");
        return 0;
    }
    /*
     * header[2..5] 是 4 字节固件大小，header[6..9] 是 4 字节 CRC32。
     * &header[2] / &header[6] 分别传递这两个字段的首地址。
     */
    *firmware_size = BootUpdate_GetUint32LE(&header[2]);

    *firmware_crc = BootUpdate_GetUint32LE(&header[6]);

    if (*firmware_size ==0U ||*firmware_size > APP_FLASH_SIZE)
    {
        Serial_Send_String("SIZE ERROR\r\n");
        return 0;    
    }
    return 1;
}

/* 按字节填满指定缓冲区；buffer 是目的首地址，length 是本包有效长度。 */
static uint8_t BootUpdate_ReceiveBuffer(uint8_t *buffer, uint32_t length)
{
    uint32_t i;

    for (i=0U; i<length; i++)
    {
        /*
         * &buffer[i] 把第 i 个元素的地址交给接收函数；函数先执行，
         * 再用 == 0U 判断是否超时，不能把比较表达式误写进函数参数中。
         */
        if(BootUpdate_ReceiveByteTimeout(&buffer[i], 100000U) ==0U)
        {
            Serial_Send_String("DATA TIMEOUT\r\n");
            return 0;
        }
    }
    return 1;
}

/*
 * 按 256 字节分包接收并写入 APP Flash。
 * “PACKET READY -> 电脑发一包 -> 写 Flash -> PACKET OK”构成软件流量控制，
 * 防止 CPU 擦写 Flash 时电脑仍连续发送，导致 USART 接收溢出。
 */
static uint8_t BootUpdate_ReceiveFirmware(uint32_t firmware_size)
{
    uint32_t received_size;
    uint32_t remaining_size;
    uint32_t packet_size;
    uint32_t write_address;

    received_size = 0U;
    write_address = APP_FLASH_START;

    while (received_size < firmware_size)
    {
        /* 尚未接收的字节数决定本包是完整 256 字节还是最后的短包。 */
        remaining_size = firmware_size - received_size;

        if (remaining_size > BOOT_PACKET_SIZE)
        {
            packet_size = BOOT_PACKET_SIZE;
        }
        else
        {
            packet_size = remaining_size;
        }
/*
         * 先准备DMA，再通知电脑发包。
         * DMA将本包数据直接搬到现有RAM缓冲区。
         */
        if (Serial_DMAReceiveStart(
                BootUpdate_Packet,
                (uint16_t)packet_size) == 0U)
        {
            Serial_Send_String("DMA START ERROR\r\n");
            return 0;
        }

        Serial_Send_String("PACKET READY\r\n");

        /*
         * 等待本包收满。
         * 返回成功时，DMA接口已经关闭接收通道。
         */
        if (Serial_DMAReceiveWait(1000000U) == 0U)
        {
            Serial_Send_String("DMA DATA TIMEOUT OR ERROR\r\n");
            return 0;
        }

        /*
         * DMA只负责UART到RAM。
         * Flash编程仍由CPU调用原来的写入函数完成。
         */

        /* 将 RAM 包缓冲区写到当前 Flash 地址，并在写入函数内部回读校验。 */
        if (BootFlash_Write(write_address, BootUpdate_Packet, packet_size) ==0U)
        {
            Serial_Send_String ("FLASH WRITE ERROR\r\n");
            return 0;
        }

        /* 只有本包写成功后，累计长度和下一包 Flash 地址才向前推进。 */
        received_size += packet_size;
        write_address += packet_size;

        Serial_Send_String("PACKET OK\r\n");
    }

return 1;

}

/*
 * 清空进入升级模式前残留在 USART 接收寄存器中的字节。
 * dummy 只用于承接并丢弃数据；static 函数只在本文件内可见。
 */
static void BootUpdate_ClearReceiveData(void)
{
    uint8_t dummy;

    while (Serial_ReceiveByte(&dummy) == 1)
    {
    }
}

/*
 * 升级总流程：清残留 -> 收头 -> 擦除 -> 分包接收 -> Flash CRC32 校验。
 * 返回 1 表示整个流程完成；任一步失败都返回 0，由 main 决定后续行为。
 */
uint8_t BootUpdate_Start(void)
{
    /*
     * firmware_size / firmware_crc 来自电脑端固件头；
     * actual_crc 是 STM32 对写入 Flash 后的 APP 重新计算得到的值。
     */
    uint32_t firmware_size;
    uint32_t firmware_crc;
    uint32_t actual_crc;

    BootUpdate_ClearReceiveData();

    Serial_Send_String("READY\r\n");

    if (BootUpdate_ReceiveHeader(&firmware_size, &firmware_crc) ==0)
    {
        return 0;
    }
    Serial_Send_String("HEADER OK\r\n");

    if(BootFlash_EraseApp(firmware_size)==0U)
    {
        Serial_Send_String("FLASH ERASE ERROR\r\n");
        return 0;
    }

    Serial_Send_String("ERASE OK\r\n");

    if (BootUpdate_ReceiveFirmware(firmware_size)==0U)
    {
        return 0;
    }

    /*
     * 把数值地址 0x08004000 转成只读字节指针，直接按存储器映射读取 Flash。
     * 只计算 firmware_size 个有效字节，不把页尾补齐的 0xFF 算入 CRC。
     */
    actual_crc = BootUpdate_CRC32((const uint8_t *)APP_FLASH_START, firmware_size);

    /* 整包 CRC 不同表示电脑原始 bin 与 Flash 中最终内容不一致。 */
    if (actual_crc != firmware_crc)
    {
        Serial_Send_String("CRC ERROR\r\n");
        return 0;
    }
    Serial_Send_String("CRC OK\r\n");

    Serial_Send_String("FIRMWARE RECEIVE OK\r\n");

    return 1;
}

