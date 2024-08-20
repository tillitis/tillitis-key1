# rom
ROM for the firmware (FW).


## Introduction
This core implements the ROM where the FW is stored.


## API
The core does not have an API.


## Implementation Details
The core is implemented by implicitly allocating a number of Embedded
Block Memory (EBM) cores, also known as SB_RAM40_4K.

During building of the FPGA bitstream the ROM binary image is mapped
into the ROM address space. When the FPGA is configured by the
bitstream, the SB_RAM40_4K cores are loaded with the ROM. The contents
is not changed by a system reset.
