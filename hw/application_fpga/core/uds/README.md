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

```
	ADDR_UDS_FIRST: 0x10
	ADDR_UDS_LAST:  0x17
```

These addresses are read only and read once between reset.

Any access to another address will be ignored by the core.


## Implementation

The 8 UDS words are stored using 32 named LUT4s identified by
"uds\_rom\_idx".

We use bit indexing from all of the 32 LUTs to store each bit of the
word, i.e., the first UDS word consists of bit 0 from each 32 LUTs,
the second word consists of bit 1 from each of the 32 LUTs, and so
on... for a maximum storage of 16 words.

The most significant bit in the LUT address serves as the read-enable
input. This requires the lower half of initialization bits to be
forced to zero, ensuring that the memory outputs zero when the
read-enable signal is inactive.
