
This document describes how to build the FPGA bitstream, including the
firmware, and get this programmed onto the flash memory of the
Tillitis Key 1 USB device.

The Tillitis Key 1 kit includes:

- Tillitis Key 1 USB stick with USB-C plug, marked MTA1-USB V1
- Programmer board based on Raspberry Pi Pico, with a white holder/jig
- USB cable with micro-B plug, for connecting the programmer to
  computer
- USB-C extension cable
- USB-C to USB-A adapter

# Programming FPGA bitstream and firmware onto Tillitis Key 1

Connect the programmer to the computer using the USB cable with
micro-B plug.

Place the Tillitis Key 1 (the USB stick) correctly in the programming
jig and close the hatch.

The USB stick can remain in the jig during repeated development,
programming and testing cycles. The USB stick should then be connected
to the computer using the provided USB-C cable (use the USB-C-to-A
adapter if needed). The jig also has a cutout to allow touching where
the touch sensor is located (next to the LED). Note that connecting
the USB stick to the computer is not required for programming it. Note
also that with this setup, to reset the USB stick back to firmware
mode after loading an app, you need to unplug both the USB cable to
the stick and the one to the programmer.

On Linux, `lsusb` should list the connected programmer as `cafe:4004
Blinkinlabs ICE40 programmer`. If the USB stick is also connected it
shows up as `1207:8887 Tillitis MTA1-USB-V1`.

To install the software needed for building and programming, please
refer to [toolchain_setup.md](toolchain_setup.md).

You are now ready to generate the FPGA bitstream (including building
the standard firmware) and program it onto the flash memory of the USB
stick. The following should be run as your regular non-root user, but
the programming is currently done using `sudo` (so sudo is expected to
be set up for your user; the Makefile runs `sudo tillitis-iceprog …`).

```
$ git clone https://github.com/tillitis/tillitis-key1
$ cd tillitis-key1/hw/application_fpga
$ make prog_flash
```

After programming, the Tillitis Key 1 USB stick can be connected to
your computer (use the USB-C-to-A adapter if needed) and will boot the
firmware. When boot has completed it will start flashing the LED
white. This indicates that it is ready to receive and measure an app.

To try out an app, continue to the README.md the apps repo:
https://github.com/tillitis/tillitis-key1-apps#readme

To learn more about the concepts and workings of the firmware, see:
[system_description/system_description.md](system_description/system_description.md)
and [system_description/software.md](system_description/software.md).

# Device personalization

To personalize Tillitis Key 1, you need to modify the hex file that
contains the Unique Device Secret (UDS). You should also update the
Unique Device Identity (UDI). These hex files are located in
`hw/application_fpga/data/`. Note that after modify the files in this
directory, you need to rebuild and program the device again (as
above).

To make this easier there is a tool that can generate these files. The
tool can be found in `hw/application_fpga/tools/tpt`. The tool allow
you to supply a secret used as part of the UDS generation. The tool
can be run interactively, or by suppling inputs on the command line:

```
usage: tpt.py [-h] [-v] [--ent ENT] [--vid VID] [--pid PID] [--rev REV] [--serial SERIAL]

options:
  -h, --help       show this help message and exit
  -v, --verbose    Verbose operation
  --ent ENT        User supplied entropy
  --vid VID        Vendor id (0 - 65535)
  --pid PID        Product id (0 - 2555
  --rev REV        Revision number (0 - 15)
  --serial SERIAL  Serial number (0 - (2**31 - 1))
```
