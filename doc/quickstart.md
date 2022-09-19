
This document describes how to build the FPGA bitstream, including the
firmware, and get this programmed onto the flash of the Tillitis Key1
USB device.

The Tillitis Key1 kit includes:

- Tillitis Key1 USB device, marked MTA1-USB V1
- Programmer board based on Raspberry Pi Pico, with a white device
  holder/jig
- USB-cable with micro-B plug, for connecting the programmer to
  computer
- USB-C cable
- USB-C to USB-A adapter

Connect the programmer to the computer using the mentioned cable. It
is convenient to connect the USB device to the USB-C cable, and then
connect the cable to the computer. The latter using the USB-C-to-A, if
needed.

`lsusb` should list two new devies: `cafe:4004 Blinkinlabs ICE40 programmer`
and `1207:8887 Tillitis MTA1-USB-V1`.

The USB device is then placed correctly in the programming jig, and
the hatch closed. The USB device can remain in the jig during repeated
programming and testing cycles. The jig has a cutout to allow for
touching next to the LED where the touch sensor is located.

To install the software needed for building and programming, please
refer to [toolchain_setup.md](toolchain_setup.md).

You are now ready to generate the FPGA bitstream including the standard
firmware, and program the flash on the connected USB device. This should be run
as your regular non-root user, but the the programming is done (in the
Makefile) with `sudo tillitis-iceprog` (so sudo is expected be set up).

```
$ git clone https://github.com/tillitis/tillitis-key1
$ cd tillitis-key1/hw/application_fpga
$ make prog_flash
```

Your Key1 device should eventually be running the firmware with the LED
flashing white, indicating that it is ready to receive an app.
