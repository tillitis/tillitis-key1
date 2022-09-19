# CH552 USB-to-Serial firmware

## Toolchain setup

SDCC:

    sudo apt install build-essential sdcc

CH554 SDK:

    cd ~
    git clone https://github.com/Blinkinlabs/ch554_sdcc.git

chprog (for flashing the firmware to a device):

    cd ~
    git clone https://github.com/ole00/chprog.git
    cd chprog
    ./build.linux.sh

## Usage

Build the firmware using a default serial number:

    make

Flash the firmware to a device:

    make flash_patched

## Re-programming the firmware

By design, once the USB to serial firmware is loaded onto the chip, there isn't an intended way to reflash it using only software. However, if 3.3V is applied to the D+ line through a 10K resistor during power-up, then the CH552 will enter bootloader mode, and a new firmware can be programmed onto the chip. Note that the CH552 flash is only guaranteed for a few hundred flash cycles.
