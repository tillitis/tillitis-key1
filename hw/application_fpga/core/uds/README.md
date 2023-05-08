# uds

Unique Device Secret core

## Introduction

This core store and protect the Unique Device Secret (UDS) asset. The
UDS can be accessed as eight separate 32-bit words. The words can be
accessed in any order, but a given word can only be accessed once
between reset cycles. The words can only be accessed as long as the
fw_app_mode input is low, implying that the CPU is executing the FW.

Each UDS words has a companion read bit that is set when the word is
accessed. This means that the even if the chip select (cs) control
input is forced high, the content will become all zero when the read
bit has been set after one cycle.

## API
There are eight addresses in the API. These are defined by the
two values ADDR_UDS_FIRST and ADDR_UDS_LAST:

´´´
ADDR_UDS_FIRST: 0x10
ADDR_UDS_LAST:  0x17
´´´

These addresses are read once.

Any access to another address will be ignored by the core.


## Implementation

The UDS words are implemented using discrete registers.
