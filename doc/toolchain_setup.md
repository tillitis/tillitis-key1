# Toolchain setup

Here are instructions for setting up the tools required to build the
project. Tested on Ubuntu 22.10.

## General development environment

The following is intended to be a complete list of the packages that
are *required* to build the gateware icestorm toolchain, the firmware,
the apps, and as well our QEMU machine.

    sudo apt install build-essential clang lld llvm bison flex libreadline-dev \
                     gawk tcl-dev libffi-dev git mercurial graphviz \
                     xdot pkg-config python3 libftdi-dev \
                     python3-dev libeigen3-dev \
                     libboost-dev libboost-filesystem-dev \
                     libboost-thread-dev libboost-program-options-dev \
                     libboost-iostreams-dev cmake libhidapi-dev \
                     ninja-build libglib2.0-dev libpixman-1-dev \
                     golang

## Gateware: icestorm toolchain

These steps are used to build and install the
[icestorm](http://bygone.clairexen.net/icestorm/) toolchain (in
`/usr/local`). Note that nextpnr is replacing Arachne-PNR.

    git clone https://github.com/YosysHQ/icestorm
    cd icestorm
    make -j$(nproc)
    sudo make install
    cd ..

    # Custom iceprog for the RPi 2040-based programmer (will be upstreamed).
    git clone -b interfaces https://github.com/tillitis/icestorm tillitis--icestorm
    cd tillitis--icestorm/iceprog
    make
    sudo make PROGRAM_PREFIX=tillitis- install
    cd ../..

    git clone https://github.com/YosysHQ/yosys
    cd yosys
    # Avoiding current issue with yosys & icebram, filed in:
    # https://github.com/YosysHQ/yosys/issues/3478
    git checkout 06ef3f264afaa3eaeab45cc0404d8006c15f02b1
    make -j$(nproc)
    sudo make install
    cd ..

    git clone https://github.com/YosysHQ/nextpnr
    cd nextpnr
    # Use nextpnr-0.4. Aa few commits later we got issues, like on f4e6bbd383f6c43.
    git checkout nextpnr-0.4
    cmake -DARCH=ice40 -DCMAKE_INSTALL_PREFIX=/usr/local .
    make -j$(nproc)
    sudo make install

References:
* http://bygone.clairexen.net/icestorm/

## Firmware: riscv toolchain

The TKey implements a [picorv32](https://github.com/YosysHQ/picorv32)
soft core CPU, which is a RISC-V microcontroller with the C
instructions and Zmmul extension, multiply without divide
(RV32ICZmmul). You can read
[more](https://www.sifive.com/blog/all-aboard-part-1-compiler-args)
about it.

The project uses the LLVM/Clang suite and version 15 or later is
required. As of writing Ubuntu 22.10 has version 15 packaged. You may
be able to get it installed on older Ubuntu and Debian using the
instructions on https://apt.llvm.org/ . There are also binary releases
here: https://github.com/llvm/llvm-project/releases

References:
* https://github.com/YosysHQ/picorv32

If your available `objcopy` and `size` commands is anything other than
the default `llvm-objcopy` and `llvm-size` define `OBJCOPY` and `SIZE`
to whatever they're called on your system before calling `make`.

## Optional

These tools are used for specific sub-components of the project, and
are not required for general development

### Kicad 6.0: Circuit board designs

The circuit board designs were all created in [KiCad
6.0](https://www.kicad.org/).

### mta1-usb-v1-programmer: RPi 2040 toolchain

These tools are needed to build the programmer firmware for the
mta1-usb-v1-programmer


#### FW update of programmer board

The programmer board is running a custom firmware developed by Blinkinlabs. The source code
for this firnware is available on Github:
https://github.com/Blinkinlabs/ice40_flasher

There is also a pre built firmware binary available for the programmer board:
https://github.com/Blinkinlabs/ice40_flasher/tree/main/bin

To update the firmware on the programmer board, either build the file "main.uf2", or download
the pre built file to your host computer. Then do the following:

1. Disconnect the programming board from the host computer
2. Press and hold the "BOOTSEL" button on the RPi2040 sub board on the programming board
3. Reconnect the programming board to the host computer
4. Release the "BOOTSEL" button after connecting the programming board to the host. The board should now appear to the host as a USB connected storage device
5. Open the storage device and drop the firmware file ("main.uf2") into the storage device

The programmer will update its firmware with the file and restart itself. After reboot the storage device will automatically be disconnected.


### mta1-usb-v1: ch552 USB to Serial firmware

The USB to Serial firmware runs on the CH552 microcontroller, and
provides a USB CDC profile which should work with the default drivers
on all major operating systems.

TODO

References:
* source code: https://github.com/tillitis/tillitis-key1/tree/main/hw/boards/mta1-usb-v1/ch552_fw
* Compiler: [SDCC](http://sdcc.sourceforge.net/)
* Library: https://github.com/Blinkinlabs/ch554_sdcc
* Flashing tool: https://github.com/ole00/chprog
