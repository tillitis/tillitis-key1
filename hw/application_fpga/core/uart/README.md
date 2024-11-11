uart
====

A simple universal asynchronous receiver/transmitter (UART) core
implemented in Verilog. The core is completed and has been used in
several FPGA designs.


## Introduction

The UART core is used as main communication channel between the TKey
device System on Chip (SoC) and the TKey client. The UART contains a
512 byte receive buffer, allowing the SW running on the SoC to not
have to wait for bytes and poll them as soon as they are received. The
number of bytes in the FIFO is also exposed to the SW through the
ADDR_RX_BYTES address.

The number of data and stop bits can be configured prior to building
the core.

The bit rate can also be configured prior to building the core. It
should be based on the target clock frequency divided by the bit rate
in order to hit the center of the bits. For example, a clock of 18 MHz
and a target bit rate of 62500 bps yields:
Divisor = 18E6 / 62500 = 288

## API

```
ADDR_RX_STATUS: 0x20
ADDR_RX_DATA:   0x21
ADDR_RX_BYTES:  0x22

ADDR_TX_STATUS: 0x40
ADDR_TX_DATA:   0x41
```

## Implementation notes.

The FIFO allocates a single block RAM (EBR).
