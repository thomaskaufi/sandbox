#!/usr/bin/env python3
import sys

import serial

PORT = "/dev/tty.PL2303G-USBtoUART14410"
BAUD = 115200


def main() -> None:
    port = sys.argv[1] if len(sys.argv) > 1 else PORT
    with serial.Serial(port, BAUD, timeout=1) as ser:
        while True:
            line = ser.readline().decode("ascii", errors="replace").strip()
            if line:
                print(line, flush=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
