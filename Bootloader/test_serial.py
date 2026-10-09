import time
import serial


PORT = "COM9"
BAUDRATE = 115200


try:
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.2,
        rtscts=False,
        dsrdtr=False
    )

    ser.dtr = False
    ser.rts = False

    print("Serial opened.")
    print("Press reset button now.")

    start_time = time.time()
    last_send_time = 0

    while time.time() - start_time < 15:
        now = time.time()

        # 反复发送U，避免错过Bootloader的等待时间
        if now - last_send_time >= 0.2:
            ser.write(b"U")
            ser.flush()
            last_send_time = now
            print("TX: 55")

        data = ser.read(64)

        if len(data) > 0:
            print("RX:", repr(data))

    ser.close()
    print("Test finished.")

except Exception as error:
    print("Python error:")
    print(type(error).__name__)
    print(str(error))

input("Press Enter to exit...")