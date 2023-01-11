#!/usr/bin/python3

import serial
import struct
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument("port")
args = parser.parse_args()

s = serial.Serial(args.port, 9600)
while True:
    [report_count, touch_count] = struct.unpack('<BB', s.read(2))
    data = ','.join([str(x) for x in (int(time.time()), report_count, touch_count)])

    print(data)
    with open(args.port.replace('/','-'),'a') as f:
        f.write(data)
        f.write('\n')
            
