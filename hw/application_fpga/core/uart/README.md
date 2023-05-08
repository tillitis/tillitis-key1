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

The number of data and data bits can be set by SW. The default is
eight data bits and one stop bit.

The default bit rate is based on target clock frequency divided by the
bit rate times in order to hit the center of the bits. I.e.  Clock: 18
MHz, 62500 bps Divisor = 18E6 / 62500 = 288

## API

```
  ADDR_BIT_RATE:  0x10
  ADDR_DATA_BITS: 0x11
  ADDR_STOP_BITS: 0x12

  ADDR_RX_STATUS: 0x20
  ADDR_RX_DATA:   0x21
  ADDR_RX_BYTES:  0x22

  ADDR_TX_STATUS: 0x40
  ADDR_TX_DATA:   0x41
```

## Implementation notes.

The FIFO allocates a single block RAM (EBR).
