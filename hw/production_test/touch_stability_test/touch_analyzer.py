#!/usr/bin/python3

import struct
import argparse

# parser = argparse.ArgumentParser()
# parser.add_argument("port")
# args = parser.parse_args()

class args:
    port = '/dev/ttyACM1'
    def __init__(self):
        pass

with open(args.port.replace('/','-'),'r') as f:
    # Skip the first two lines since they might be corrupted
    f.readline()
    f.readline()

    records = 0
    tx_count_errors = 0
    touch_count_changes = 0

    [first_time,last_tx_count,last_touch_count] = [int(i) for i in f.readline().split(',')]

    try:
        while True:
            [time,tx_count,touch_count] = [int(i) for i in f.readline().split(',')]

            if tx_count != (last_tx_count + 1) % 256:
                tx_count_errors += 1

            if touch_count != last_touch_count:
                touch_count_changes +=1
                print('touch count change', time,touch_count,tx_count)

            last_tx_count = tx_count
            last_touch_count = touch_count

            records += 1

    except Exception as e:
        print('tx_count errors:', tx_count_errors,
                'touch_count_changes:', touch_count_changes,
                'records:', records)

        time_hours = (time - first_time)/60/60
        print('run time:{:.1f} hours'.format(time_hours))

