#!/bin/bash
set -e

iceprog -I B interface_fpga_usb.bin
echo 'Waiting 2s for USB to stabalize'
sleep 2
if lsusb -d 1d50:6159; then
    echo 'device found, USB test passed'
else
    echo 'device not found, USB test failed'
fi
