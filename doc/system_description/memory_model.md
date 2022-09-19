
# Address map breakdown and scheme and data formats

## Introduction

We need to agree and define how we use the 32-bit address space. In a way that is:

    1. Efficient for HW implementation (decoding logic)
    2. Easy to implement in QEMU model
    3. Natural, easy to understand and use in SW and develop for
    4. Allow simple access separation, control based on security state (FW, APP)

This shared document (hopefully) allows us to accomplish these goals.

## Addressing and core access

In general, the cores only operates on 32-bit words. They expect to get 32-bits
when written to, and return 32-bits when read. The APIs are very simplistic and
does not support byte or half words operations (this would require additional
wires from the CPU core to signal which load-store operation it is trying to
perform - byte, half word, word.

The RISC-V architecture in contrast has byte addressable addresses. This means
that 0x00000000 .. 0x00000003 points to the four different bytes in word 0.

In order to reduce confusion we should:

- Skip the two LSBs in the address. Since the cores have 8 bit address space,
  their address space as seen by the CPU is 10 bits, but will be 0x000, 0x004,
  0x008, 0x00c etc. A core always sees an 8-bit address. It is the way the core
  is hooked up in the application_fpga that controls which 8 bits those are (of
  the 32-bit address the CPU requests).

- Always use 32-bit read or write instructions when accessing core registers.
  That is lw, sw and use uint32_t as source or destination data type. Using lh,
  lb will cause confusion.

Looking at the address format above I would suggest something like:

```
31st bit                              0th bit
v                                     v
0000 0000 0000 0000 0000 0000 0000 0000

- Bits [31 .. 30] (2 bits): Top level area prefix (0b00:ROM, 0b01:RAM, 0b10:reserved, 0b11:MMIO)
- Bits [29 .. 24] (6 bits): Core select. We want to support at least 16 cores
- Bits [23 ..  0] (24 bits): Memory/in-core address.
```

This leaves 24 bits for memory or in-core address. Actually, for memory it is
possible to use 30 bits, because core select is not needed.

The cores only have 8-bit addresses, but they are 32-bit word-aligned,
so they will occupy 10 LSBs (also see above).


Assigned top level prefixes:

```
ROM      0b00, 30-bit address
RAM      0b01, 30-bit address
reserved 0b10
MMIO     0b11, 6 bits for core select, 24 bits rest
```

Assigned core prefixes:

```
ROM   0x00
RAM   0x40
TRNG  0xc0 // cores from 0b11000000...
TIMER 0xc1
UDS   0xc2
UART  0xc3
TOUCH 0xc4
MTA1  0xff
```


Examples:

```
0xc3000004: NAME1 in UART
0xff000000: NAME0 in MTA1
0xff000008: VERSION in MTA1
```

## Endianness

Currently all cores handles words in Big Endian format. I need to ensure that
the cores are can handle little endian words. This will probably affect, force
update of which bits are used in status, configuration and control registers.

## Mirror effects

Since each core (and ROM, RAM) only use lower subset of the allocated 24 bit
address, contents will be mirrored modulo the size of the real address. For
example, the TRNG will appear every 10 bit address.

The reason for this is that there is no boundary checks on the address for
in-core addresses. This would require 24 bit comparators. This could be added
when we see that we have resources to do so. A later stage cleanup.


---
