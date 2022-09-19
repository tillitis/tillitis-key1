# Toolchain setup

Here are instructions for setting up the tools required to build the project.
Tested on Ubuntu 22.04 LTS.

## Gateware: icestorm toolchain

These steps are used to build and install the
[icestorm](http://bygone.clairexen.net/icestorm/) toolchain (in
`/usr/local`). Note that nextpnr replaces Arachne-PNR.

    sudo apt install build-essential clang lld bison flex libreadline-dev \
                         gawk tcl-dev libffi-dev git mercurial graphviz   \
                         xdot pkg-config python3 libftdi-dev \
                         python3-dev libboost-dev libeigen3-dev \
                         libboost-dev libboost-filesystem-dev \
                         libboost-thread-dev libboost-program-options-dev \
                         libboost-iostreams-dev cmake libhidapi-dev

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
    cmake -DARCH=ice40 -DCMAKE_INSTALL_PREFIX=/usr/local .
    make -j$(nproc)
    sudo make install

References:
* http://bygone.clairexen.net/icestorm/

## Firmware: riscv toolchain

The Tillitis Key 1 implements a
[picorv32](https://github.com/YosysHQ/picorv32) soft core CPU, which
is a RISC-V microcontroller with the M and C instructions (RV32IMC).
You can read
[more](https://www.sifive.com/blog/all-aboard-part-1-compiler-args)
about it.

The project uses the LLVM/Clang suite, where version 14 is the latest
stable (as of writing). Usually the LLVM/Clang packages that are part
of your distro will work, if not, there are installations instructions
for "Install (stable branch)" at https://apt.llvm.org/ for Debian and
Ubuntu.

References:
* https://github.com/YosysHQ/picorv32

## Optional

These tools are used for specific sub-components of the project, and
are not required for general development

### Kicad 6.0: Circuit board designs

The circuit board designs were all created in [KiCad
6.0](https://www.kicad.org/).

### mta1-usb-v1-programmer: RPi 2040 toolchain

These tools are needed to build the programmer firmware for the
mta1-usb-v1-programmer

TODO

* source code: https://github.com/Blinkinlabs/ice40_flasher


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
