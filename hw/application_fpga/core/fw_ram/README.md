# fw_ram
RAM for the firmware (FW).


## Introduction
This core implements the RAM exclusive to the FW. This allows the FW
to execute without having to use the application RAM. The core
supports mode based lock down to prevent any access in application
mode.


## API
The core does not have an API.


## Implementation Details
The core is implemented by explicitly instantiating one or more pairs
of SB_RAM40_4K RAM cores in the FPGA.

The contents of the fw_ram is cleared when the FPGA is powered up and
configured by the bitstream. The contents is not cleared by a system
reset.

If the fw_app_mode input is set, no memory accesses are allowed. Any
reads when the fw_app_mode is set will retun an all zero word.
