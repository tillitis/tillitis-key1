# uds

Unique Device Secret core

## Introduction

This core store and protect the Unique Device Secret (UDS) asset. The
UDS can be accessed as eight separate 32-bit words. The words can only
be accessed as long as the `en` input is high.

The UDS words can be accessed in any order, but a given word can only
be accessed once between reset cycles. This read once functionality is
implemented with a companion read bit for each word. The read bit is
set when the word is first accessed. The read bit controls if the real
UDS word is returned or not.

This means that the even if the chip select (cs) control
input is forced high, the content will become all zero when the read
bit has been set after one cycle.


## API

Three address bits are used. Addresses 0-7 address each of the UDS
words. All words are read-only and read-once between reset.


## Implementation

These read-only registers provide read access to the 256-bit UDS.

The eight UDS words are stored using 32 named SB\_LUT4 FPGA
multiplexer (MUX) instances, identified in the source code as
"uds\_rom\_idx". One instance for each bit in the core read\_data
output bus.

During build of the FPGA design, the UDS is set to a known bit
pattern, which means that the SB\_LUT4 instantiations are initialized
to a fixed bit pattern.

The tool 'patch\_uds\_udi.py' is used to replace the fixed bit pattern
with a unique bit pattern before generating the per device unique FPGA
bitstream. This allows us to generate these device unique FPGA
bitstreams without haveing to do a full FPGA build.

Each SB\_LUT4 MUX is able to store 16 bits of data, in total 512 bits.
But since the UDS is 256 bits, we only use the eight LSBs in each MUX.

The eighth MSBs in each MUX will be initialized to zero. The read
access bit (se description above) for a given word is used as the
highest address bit to the MUXes. This forces any subsequent accesses
to a UDS word to read from the MUX MSBs, not the LSBs where the UDS is
stored.
