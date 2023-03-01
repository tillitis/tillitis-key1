# Tillitis TKey Quickstart

This document describes how to build the FPGA bitstream, including the
firmware, and get this programmed onto the flash memory of the
Tillitis TKey USB device.

*Note well*: If you have a TKey which already has been flashed, then
you don't need to do anything unless you want to set your own Unique
Device Secret (UDS). You can start running apps on it immediately. See
[tillitis-key1-apps](https://github.com/tillitis/tillitis-key1-apps)
for a few examples.

The Tillitis TKey kit includes:

- Tillitis TKey USB stick with USB-C plug, marked MTA1-USB V1
- Programmer board based on Raspberry Pi Pico, with a white holder/jig
- USB cable with micro-B plug, for connecting the programmer to
  computer
- USB-C extension cable
- USB-C to USB-A adapter

## Programming FPGA bitstream and firmware onto TKey

Connect the programmer to the computer using the USB cable with
micro-B plug.

Place the TKey USB stick correctly in the programming jig and close
the hatch.

The USB stick can remain in the jig during repeated development,
programming and testing cycles. The USB stick should then be connected
to the computer using the provided USB-C cable (use the USB-C-to-A
adapter if needed). The jig also has a cutout to allow touching where
the touch sensor is located (next to the LED on the outer edge). Note
that connecting the USB stick to the computer is not required for
programming it. Note also that with this setup, to reset the USB stick
back to firmware mode after loading an app, you need to unplug both
the USB cable to the stick and the one to the programmer.
Alternatively, you can try the script in
`../hw/application_fpga/tools/reset-tk1` which pokes at the TKey
that's sitting in the jig, leaving it in firmware mode so that a new
app can be loaded.

On Linux, `lsusb` should list the connected programmer as `cafe:4004
Blinkinlabs ICE40 programmer`. If the USB stick is also connected it
shows up as `1207:8887 Tillitis MTA1-USB-V1`.

To install the software needed for building and programming, please
refer to [toolchain_setup.md](toolchain_setup.md).

You are now ready to generate the FPGA bitstream (including building
the standard firmware) and program it onto the flash memory of the USB
stick. Note that this will give a default Unique Device Secret. If you
want to personalize your TKey, see under Device personalization below
first.

The following should be run as your regular non-root user, but
the programming is currently done using `sudo` (so sudo is expected to
be set up for your user; the Makefile runs `sudo tillitis-iceprog …`).

```
$ git clone https://github.com/tillitis/tillitis-key1
$ cd tillitis-key1/hw/application_fpga
$ make prog_flash
```

After programming, the TKey can be connected to your computer (use the
USB-C-to-A adapter if needed) and will boot the firmware. When boot
has completed the LED will be steady white. This indicates that it is
ready to receive and measure an app.

To try out an app, continue to the README.md the apps repo:
https://github.com/tillitis/tillitis-key1-apps#readme

To learn more about the concepts and workings of the firmware, see:
[system_description/system_description.md](system_description/system_description.md)
and [system_description/software.md](system_description/software.md).

## Device personalization - setting Unique Device Secret (UDS)

To personalize your TKey you need to modify the Unique Device Secret
(UDS) and, maybe, the Unique Device Identity (UDI).

The simplest way to generate a new UDS is to:

```
$ cd tillitis-key1/hw/application_fpga
$ make secret
```

Then proceed with the `make prog_flash` as discussed above.

In detail: You need to modify the hex file that contains the Unique
Device Secret (UDS). You might also want to update the Unique Device
Identity (UDI). These hex files are located in
`hw/application_fpga/data/`. Note that after modify the files in this
directory, you need to rebuild and program the device again (as
above).

To make this easier there is a tool that can generate these files. The
tool can be found in `hw/application_fpga/tools/tpt`. The tool by
default allow you to supply a secret used as part of the UDS
generation and only generates a new `uds.hex`. See `--help` for more
flags.
