#!/usr/bin/env python3

import usb.core
import usb.util
import time

VID = 0x1209  # replace
PID = 0x8885  # replace
# VID = 0x1207  # Bellatrix
# PID = 0x8887 # Bellatrix

dev = usb.core.find(idVendor=VID, idProduct=PID)
if dev is None:
    raise RuntimeError("Device not found")

# Detach kernel driver if needed (Linux)
#if dev.is_kernel_driver_active(0):
#    dev.detach_kernel_driver(0)

# Read supported languages (index 0)
langs = dev.ctrl_transfer(
    bmRequestType=usb.util.build_request_type(
        usb.util.CTRL_IN,
        usb.util.CTRL_TYPE_STANDARD,
        usb.util.CTRL_RECIPIENT_DEVICE
    ),
    bRequest=usb.REQ_GET_DESCRIPTOR,
    wValue=(usb.util.DESC_TYPE_STRING << 8) | 0,
    wIndex=0,
    data_or_wLength=255
)
langid = langs[2] | (langs[3] << 8)  # usually 0x0409 = English (US)

print(f"Using language ID: 0x{langid:04X}\n")

# Continuously loop over string descriptors 1 to 8
while True:
    # for index in range(1, 9):  # 1 through 8
    for index in (3, 6, 7, 8):  # 1 through 8
    # for index = 7:  # 1 through 8
    # index = 7
        try:
            s = usb.util.get_string(dev, index, langid)
            time.sleep(0.002)
            print(f"String descriptor {index}: {s}")
        except usb.core.USBError:
            print(f"String descriptor {index}: <not available>")
print("-" * 40)
