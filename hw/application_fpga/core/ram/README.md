# ram
RAM for the application.


## Introduction
This core implements the 128 kByte RAM available for the device
applications. The core also implements (from the view of the CPU)
transparent RAM address and data scrambling. This scrambling is
applied to make it harder to extract application and application data
from a memory dump directly from the memory cores.

## API
The core does not have an API.


## Implementation Details
The core is implemented by explicitly instantiating the four
SB_SPRAM256KA 16 kW16 cores in the FPGA. The blocks are used in pairs
to achieve a complete 32kW32 RAM.

The RAM address and data scrambling use 32 bit ram_addr_rand and
ram_data_rand inputs as seeds for the scrambling mechanism. When data
is read out it is descrambled before sent out on the read_data output
port. The scrambling functionality does not add latency.

The memory protection is based on two separate mechanisms:

1. Address randomisation
2. Address dependent data randomization

The address randomization is implemented by XORing the RAM address
with the contents of the ADDR\_RAM\_ADDR\_RAND register in the tk1
core. The result is used as the RAM address.

The data randomization is implemented by XORing the data written to
the RAM with the contents of the ADDR\_RAM\_DATA\_RAND register in the
tk1 core as well as XORing with the CPU address. This means that the
same data written to two different addresses will be scrambled
differently. The same pair or XOR operations is also performed on the
data read out from the RAM.

Note: the scrambling mechanism is NOT a cryptographically secure
function. Even if it was, a 32 bit key would be too short to add any
security.
