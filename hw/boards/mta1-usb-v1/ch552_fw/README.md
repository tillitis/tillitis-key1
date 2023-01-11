# CH552 USB-to-Serial firmware

## Toolchain setup

SDCC:

    sudo apt install build-essential sdcc

chprog (for flashing the firmware to a device):

    cd ~
    sudo apt install libusb-1.0-0-dev
    git clone https://github.com/ole00/chprog.git
    cd chprog
    ./build_linux.sh
    sudo cp chprog /usr/local/bin

## Usage

Build the firmware using a default serial number:

    make

Flash the firmware to a device:

    make flash_patched

## Re-programming the firmware

By design, once the USB to serial firmware is loaded onto the chip, there isn't an intended way to reflash it using only software. However, if 3.3V is applied to the D+ line through a 10K resistor during power-up, then the CH552 will enter bootloader mode, and a new firmware can be programmed onto the chip. Note that the CH552 flash is only guaranteed for a few hundred flash cycles.
