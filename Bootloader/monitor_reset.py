import time
import serial


PORT = "COM9"
BAUDRATE = 115200


def main():
    ser = serial.Serial()

    ser.port = PORT
    ser.baudrate = BAUDRATE
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.timeout = 0.2

    ser.rtscts = False
    ser.dsrdtr = False

    # 不发送U
    ser.dtr = False
    ser.rts = False

    ser.open()

    print("Serial opened.")
    print("This program will not send U.")
    print("Press reset button to test.")
    print("Press Ctrl+C to stop.")

    try:
        while True:
            line = ser.readline()

            if not line:
                continue

            text = line.decode(
                "ascii",
                errors="replace"
            ).strip()

            current_time = time.strftime(
                "%H:%M:%S"
            )

            print("[%s] RX: %s" % (current_time, text))

    except KeyboardInterrupt:
        print("\nMonitor stopped.")

    finally:
        ser.close()
        print("Serial closed.")


if __name__ == "__main__":
    main()