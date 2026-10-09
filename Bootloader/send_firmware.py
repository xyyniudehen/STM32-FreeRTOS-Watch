# -*- coding: gb2312 -*-
"""
STM32 Bootloader 电脑端固件发送工具。

职责：读取 APP.bin，生成固件头，通过 USART 协议分包发送，并根据
Bootloader 的 READY / PACKET READY / PACKET OK 响应控制发送节奏。
该脚本运行在电脑内存中，不属于 STM32 固件。
"""

import struct
import time
import zlib
import serial

# struct：按指定字节序打包固件头；time：超时和发送节拍；
# zlib：计算与 STM32 软件算法一致的 CRC32；serial：pyserial 串口库。


# 必须与设备管理器中的 CH340 端口、STM32 USART1 配置保持一致。
PORT = "COM10"
BAUDRATE = 115200

# 与 BootUpdate.h 中 BOOT_PACKET_SIZE 一致；电脑每次最多发送 256 字节。
PACKET_SIZE = 256

# APP 工程生成的原始二进制文件，不是 Bootloader 的 hex 文件。
# bin 中第 0 字节将写入 STM32 的 APP_FLASH_START（0x08004000）。
FIRMWARE_PATH = (
    r"E:\xieyiyi\gz\stm32\STM32-WATCH-main"
    r"\11\14freertos\RTOS\Objects\APP.bin"
)


def main():
    """执行一次完整的固件发送流程。"""
    # rb 表示按原始字节读取。firmware_data 位于电脑 RAM，可容纳整个固件。
    with open(FIRMWARE_PATH, "rb") as file:
        firmware_data = file.read()

    # 固件大小按真实字节数计算，不能使用资源管理器中四舍五入后的 KB 数值。
    firmware_size = len(firmware_data)

    # zlib.crc32 可能按 Python 平台表现为带符号概念，& 0xFFFFFFFF 保留低 32 位。
    # 该值放入固件头，STM32 写完 Flash 后重新计算并比较。
    firmware_crc = zlib.crc32(firmware_data) & 0xFFFFFFFF

    # 固件头共 10 字节：55 AA + size(4B) + crc(4B)。
    # “<”表示小端；2B 表示两个 uint8_t；II 表示两个 uint32_t。
    header = struct.pack(
        "<2BII",
        0x55,
        0xAA,
        firmware_size,
        firmware_crc
    )

    # 整数向上取整。41756 字节会被分成 163 个 256 字节包和 1 个 28 字节包。
    total_packets = (
        firmware_size + PACKET_SIZE - 1
    ) // PACKET_SIZE

    print("Firmware size:", firmware_size)
    print("Firmware CRC: 0x%08X" % firmware_crc)
    print("Total packets:", total_packets)
    print("Header:", header.hex(" ").upper())

    # 打开电脑端串口。timeout 只控制单次 read 等待时间，整体升级另有 120 秒超时。
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.1,
        rtscts=False,
        dsrdtr=False
    )

    # 明确关闭 DTR/RTS 控制线；本协议只使用 TX、RX、GND，不使用硬件流控。
    ser.dtr = False
    ser.rts = False

    print("Serial opened.")
    print("Press reset button now.")

    start_time = time.time()
    last_u_time = 0

    # True：自动周期发送 ASCII 字符 U 申请升级；进入 Update Mode 后自动关闭。
    # 调试“只监听、不升级”时可设为 False，但正式下载必须保持 True。
    send_u_enable = True
    # send_u_enable = False
    header_sent = False

    # offset：下一包在 firmware_data 中的起始下标。
    # pending_size：已经发送但尚未收到 PACKET OK 的包长度。
    # completed_packets：STM32 已确认写入成功的包数量。
    offset = 0
    pending_size = 0
    completed_packets = 0

    # 串口一次 read 可能只得到半行，也可能得到多行；该缓冲区用于按 \n 拆分完整响应。
    line_buffer = b""

    try:
        while time.time() - start_time < 120:
            now = time.time()

            # 尚未进入升级模式时，每 200 ms 发送一次 U，避免错过 5 秒窗口。
            if send_u_enable:
                if now - last_u_time >= 0.2:
                    # b"U" 是单个原始字节 0x55，不是文本字符串“55”。
                    ser.write(b"U")
                    ser.flush()
                    last_u_time = now
                    print("TX: U")

            # 读取串口原始数据
            data = ser.read(ser.in_waiting or 1)

            if not data:
                continue

            # 将本次收到的碎片追加到软件行缓冲区，等待出现换行符。
            line_buffer += data

            # STM32 响应以 \r\n 结束；循环可处理一次 read 得到的多行响应。
            while b"\n" in line_buffer:
                raw_line, line_buffer = line_buffer.split(
                    b"\n", 1
                )

                # STM32 状态文字均为 ASCII；errors="replace" 防止异常字节让工具崩溃。
                line = raw_line.rstrip(b"\r").decode(
                    "ascii",
                    errors="replace"
                )

                if not line:
                    continue

                print("RX:", line)

                # Bootloader 已接收第一个 U，此后必须停止重复发送，避免 U 混入固件头。
                if line == "Update Mode":
                    send_u_enable = False
                    print("Stop sending U")

                # READY 表示 STM32 已清理旧接收数据，并开始等待 10 字节固件头。
                elif line == "READY":
                    if not header_sent:
                        ser.write(header)
                        ser.flush()
                        header_sent = True
                        print(
                            "TX header:",
                            header.hex(" ").upper()
                        )

                # 只有收到 PACKET READY 才发送下一包，避免 STM32 写 Flash 时 USART 溢出。
                elif line == "PACKET READY":
                    if pending_size != 0:
                        raise RuntimeError(
                            "New packet requested before PACKET OK"
                        )

                    # Python 切片不会越界；最后不足 256 字节时自动得到短包。
                    packet = firmware_data[
                        offset:offset + PACKET_SIZE
                    ]

                    if len(packet) == 0:
                        raise RuntimeError(
                            "STM32 requested too much data"
                        )

                    ser.write(packet)
                    ser.flush()

                    # 暂不推进 offset，必须等 STM32 返回 PACKET OK 后才确认本包成功。
                    pending_size = len(packet)

                    print(
                        "TX packet:",
                        completed_packets + 1,
                        "/",
                        total_packets,
                        "bytes:",
                        pending_size
                    )

                # PACKET OK 表示 STM32 已接收、写入并回读校验当前包。
                elif line == "PACKET OK":
                    if pending_size == 0:
                        raise RuntimeError(
                            "Unexpected PACKET OK"
                        )

                    # 只有收到确认后才移动文件下标，保证电脑和 STM32 的包进度一致。
                    offset += pending_size
                    pending_size = 0
                    completed_packets += 1

                    progress = offset * 100 / firmware_size

                    print(
                        "Progress: %.1f%% (%d/%d bytes)"
                        % (
                            progress,
                            offset,
                            firmware_size
                        )
                    )

                # 该响应在 STM32 完成全固件 CRC32 校验后发出，代表升级闭环成功。
                elif line == "FIRMWARE RECEIVE OK":
                    if offset != firmware_size:
                        raise RuntimeError(
                            "Firmware size mismatch"
                        )

                    print("Firmware transfer completed.")
                    return

                # 任意错误、失败或超时响应都立即终止本次升级，防止继续发送错误数据。
                elif (
                    "ERROR" in line
                    or "FAIL" in line
                    or "TIMEOUT" in line
                ):
                    raise RuntimeError(
                        "Bootloader error: " + line
                    )

        raise RuntimeError("Bootloader timeout")

    finally:
        # 无论成功、异常还是超时都关闭 COM 口，避免下次被串口助手提示端口占用。
        ser.close()
        print("Serial closed.")


# 只有直接运行 python send_firmware.py 时才启动升级；被其他脚本导入时不会自动下载。
if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print("Update failed:")
        print(type(error).__name__, error)

    input("Press Enter to exit...")