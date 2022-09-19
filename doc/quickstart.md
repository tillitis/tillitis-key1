
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

After programming, when your Key1 device is connected to the host, it would boot the firmware.
When boot has completed, the device will start flashing the LED white. This indicates that the device
is ready to receive and measure an app.

To personalize the device, you need to modify the hex file that contain the Unique Device Secret (UDS).
You should also update the Unique Device Identity (UDI). These hex files are located in hw/application_fpga/data.
To make this easier there is a tool, tpt that can generate these files. The tool can be found in hw/application_fpga/tools/tpt.
The tool allow you to supply a secret used as part of the UDS generation. The tool can be run interactively, or by suppling
inputs on the command line:

```
usage: tpt.py [-h] [-v] [--uss USS] [--vid VID] [--pid PID] [--rev REV] [--serial SERIAL]

options:
  -h, --help       show this help message and exit
  -v, --verbose    Verbose operation
  --uss USS        User supplied secret
  --vid VID        Vendor id (0 - 65535)
  --pid PID        Product id (0 - 2555
  --rev REV        Revision number (0 - 15)
  --serial SERIAL  Serial number (0 - (2**31 - 1))
```
