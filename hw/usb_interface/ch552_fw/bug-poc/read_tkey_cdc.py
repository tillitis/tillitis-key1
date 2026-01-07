#!/usr/bin/env python3

import argparse
import serial
import time
import datetime

def format_bytes_verbose(data, prefix=""):
    lines = []
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]

        hex_part = " ".join(f"0x{b:02X}" for b in chunk)
        ascii_part = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)

        lines.append(f"{prefix}{hex_part:<63}  |{ascii_part}|")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Read TKey serial port (CDC)")
    parser.add_argument("--port", help="serial port to read")
    args = parser.parse_args()

    # Require main arguments only if not using --list-devices     
    required = ["port"]
    missing = [arg for arg in required if getattr(args, arg) is None]
    if missing:
        parser.error(f"Missing required arguments: {', '.join('--' + m.replace('_', '-') for m in missing)}")

    vid = 0x1209          # Change it for your device
    pid = 0x8885          # Change it for your device
    target_interface = 1  # Change it for your device
    send_data = b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x40'
    BAUDRATE = 625000
    ser = serial.Serial(args.port, baudrate=BAUDRATE, timeout=1)
    while True:
        ser.write(send_data)
        data = ser.read(len(send_data))
        # data = ser.read_all()
        if data:
            dt = datetime.datetime.now()
            print(f"{dt} [TKEY (length: {len(data)})")
            print(format_bytes_verbose(data, prefix="  "))

        time.sleep(0.051)

if __name__ == "__main__":
    main()
