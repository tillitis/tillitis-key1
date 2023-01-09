# MTA1-USB-CH552 hardware test

This is a hardware bringup test for the MTA1-USB-CH552 board.

## Usage

The flash chip for the application FPGA must be programmed with the included firmware

## Voltage rails

The MTA1-USB board has three voltage supplies- 1.2V, 3.3V, and 2.5V. These are measured by hand using a sufficiently accurate DMM. Additionally, the power supplies must come up in a specific sequence (see the notes on schematic page 4). This can be verified using an oscilloscope.

## CH552 to  App FPGA connections

This includes the signals TX, RX, RTS, and CTS. On the FPGA, these signals are routed as loopback connections. They can be tested using a script on a PC (TODO)

## Application FPGA programming connections

This inclues '_ICE_MISO, _ICE_MOSI, _ICE_SCK, _ICE_SS, _CRESET, _CDONE'. These nets are tested by using the FTDI chip to program the SPI flash. If the FPGA boots, then it is assumed that all of the connections on these nets are connected correctly.

## GPIO connections

The application FPGA has '_GPIO1, _GPIO2, _GPIO3, _GPIO4' expansion connections. These are tested by outputting different frequency clocks to them, which are manually verified using a logic analyzer or oscilloscope.

| GPIO | Expected clock frequency |
| ---  | ---                      |
| 1    | 24 MHz                   |
| 2    | 12 MHz                   |
| 3    | 6 MHz                    |
| 4    | 3 MHz                    |

## RGB LEDs

The application FPGAs has an RGB status LED. This is verified by feeding different clock signals into each color channel, so that the LEDs will cycle through the color sequence Red,Green,Blue,White.

## CH552 USB interface

The USB connection on the CH552 is verified by loading the USB-to-serial converter firmware onto it.

## Application FPGA touch sensor

The touch sensor is used to gate the output of the application FPGA LED. The touch sensor is considered connected if it turns off the application FPGA LED when it is touched.
