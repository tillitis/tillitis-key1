# tk1

## Introduction

The tk1 core is where information, control and monitoring
functionality unique to the TKey1 can be found. This means that it
provides more than one distinct functionality accessible via the core
API.


## API

### Access to device information

```
	ADDR_NAME0:   0x00
	ADDR_NAME1:   0x01
	ADDR_VERSION: 0x02
```

These addresses provide read only information about the name (type)
and version of the device. They can be read by FW as well as
applications.


### Control of execution mode

```
	ADDR_SWITCH_APP: 0x08
```

This register controls if the device is executing in FW mode or in App
mode. The register can be written once between power cycles, and only
by FW. If set the device is in app mode.


### Control of RGB LED

```
	ADDR_LED:  0x09
	LED_B_BIT: 0
	LED_G_BIT: 1
	LED_R_BIT: 2
```

This register control the RGB LED on the TKey device. Setting a bit to
one turns the corresponding color on. It can be read and written by FW
as well as by Apps.


### Control and status of GPIO

```
	ADDR_GPIO: 0x0a
	GPIO1_BIT: 0
	GPIO2_BIT: 1
	GPIO3_BIT: 2
	GPIO4_BIT: 3
```

This register control and provide status access to the four GPIOs on
the TKey. GPIO one and two are inputs. They will sample the input
level and present it to SW. GPIO three and four are outputs. They will
emit either high or low voltage level depending on if the
corresponding register is one or zero.


### Application introspection

```
	ADDR_APP_START: 0x0c
	ADDR_APP_SIZE:  0x0d
```

These registers provide read only information to the loaded app to
itself - where it was loaded and its size. The values are written by
FW as part of the loading of the app. The registers can't be written
when the ADDR_SWITCH_APP has been set.


### Access to Blake2s

```
	ADDR_BLAKE2S: 0x10
```

This register provides the 32-bit function pointer address to the
Blake2s hash function in the FW. It is written by FW during boot. The
register can't be written to when the ADDR_SWITCH_APP has been set.


### Access to CDI

```
  ADDR_CDI_FIRST: 0x20
  ADDR_CDI_LAST:  0x27
```

These registers provide access to the 256-bit compound device secret
calculated by the FW as part of loading an application. The registers
are written by the FW. The register can't be written to when the
ADDR_SWITCH_APP has been set. Apps can read the CDI and is it as base
secret for any secrets it needs to perform its intended use case.


### Access to UDI

```
  ADDR_UDI_FIRST: 0x30
  ADDR_UDI_LAST:  0x31
```

These registers provide read access to the 64-bit unique device
identity. The UDI is stored as ROM within the FPGA configuration. The
registers can't be written to.


### RAM memory protecion

```
  ADDR_RAM_ASLR:     0x40
  ADDR_RAM_SCRAMBLE: 0x41
```

These write only registers control how the data in the RAM is
scrambled as a way of protecting the data. The ADDR_RAM_ASLR control
how the addresses are scrambled. The ADDR_RAM_SCRAMBLE control how the
data itself is scrambled. FW writes random values to these registers
during boot.


### Execution monitor

```
  ADDR_CPU_MON_CTRL: 0x60
  ADDR_CPU_MON_FIRST:0x61
  ADDR_CPU_MON_LAST: 0x62
```

These registers control the execution monitor related to the RAM. Once
enabled, by writing to ADDR_CPU_MON_CTRL, the memory are defined by
ADDR_CPU_MON_FIRST and ADDR_CPU_MON_LAST inclusive will be protected
against execution. Typically this will be the application stack and,
or heap.

Applications can write to these registers to define the area and then
enable the monitor. One enabled, the monitor can't be disabled, and
the ADDR_CPU_MON_FIRST and ADDR_CPU_MON_LAST registers can't be
changes. This means that an application that wants to use the monitor
must define the area first before enabling the monitor.

Once enabled, if the CPU tries to read an instruction from the defined
area, the core will force the CPU to instead read an all zero, illegal
instruction. This illegal instruction will trigger the CPU to enter
its TRAP state, from which it can't returned unless the TKey is power
cycled.

The FW will not write to these registers as part of loading an
app. The app developer must define the area and enable the monitor to
get the protection.

Note that there is a second memory area that is under the protection
of the execution monitor - the FW_RAM. The execution protection of
this memory is always anabled and the definition of the area is hard
coded into the FPGA design.

One feature not obvious from the API is that when the CPU traps, the
core will detect that and start flashing the RGB LED with a red
light - indicating that the CPU is its trap state and no further
execution is possible.

## SPI-master

The TK1 includes a minimal SPI-master that provides access to the
Winbond Flash memory mounted on the board. The SPI-master is byte
oriented and very minimalistic.

In order to transfer more than a single byte, SW must read status and
write commands needed to send a sequence of bytes. In order to read
out a sequence of bytes from the memory, SW must send as many dummy
bytes as the data being read from the memory.

The SPI-master is controlled using a few API
addresses:

```
localparam ADDR_SPI_EN		= 8'h80;
localparam ADDR_SPI_XFER	= 8'h81;
localparam ADDR_SPI_DATA	= 8'h82;
```

**ADDR_SPI_EN** enables and disabled the SPI-master. Writing a 0x01 will
lower the SPI chip select to the memory. Writing a 0x00 will raise the
chip select.

Writing to the **ADDR_SPI_XFER** starts a byte transfer. Reading from
the address returns the status for the SPI-master. If the return value
is not zero, the SPI-master is ready to send a byte.

**ADDR_SPI_DATA** is the address used to send and receive a byte.
data. The least significant byte will be sent to the memory during a
transfer. The byte returned from the memory will be presented to SW if
the address is read after a transfer has completed.

The sequence of operations needed to perform is thus:

1. Activate the SPI-master by writing a 0x00000001 to ADDR_SPI_EN
2. Write a byte to ADDR_SPI_DATA
3. Read ADDR_SPI_XFER to check status. Repeat until the read
   operation returns non-zero value
4. Write to ADDR_SPI_XFER
5. Read ADDR_SPI_XFER to check status. Repeat until the read operation
   returns a non-zero value
6. Read out the received byte frm ADDR_SPI_DATA
7. Repeat 2..6 as many times as needed to send a command and data to
   the memory and getting the expected status, data back.
8. Deactivate the SPI-master by writing 0x00000000 to ADDR_SPI_EN

The SPI connected memory on the board is the Winbond W25Q80. For
information about the memory including support commands and protocol,
see the datasheet:

https://www.mouser.se/datasheet/2/949/w25q80dv_dl_revh_10022015-1489677.pdf


## Implementation

The core is implemented as a single module. Future versions will
probably be separated into separate modules.


## Winbond Flash memory model

The testbench for the SPI master includes a memory model of the Flash
memory. The model is provided by Winbond and the copyright for the
model is:

```
/******************************************************************************
 Winbond Electronics Corporation
 Verilog Simulation for W25Q80DL Serial Flash Memory

 V1.01

 Copyright (c) 2001-2015 Winbond Electronics Corporation
 All Rights Reserved.


 Notes:

 Versions:
	12/09/2015		Initial Version
******************************************************************************/
```

---
