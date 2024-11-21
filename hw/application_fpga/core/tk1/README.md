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

The execution mode consists of two modes, firmware mode and
application mode. These modes have certain privileges. The execution
mode is dynamically controlled in the hardware, depending on current
location of execution. After a reset the device starts in firmware
mode, as soon as the executions moves outside of ROM the mode is
changed to application mode.

There also exists an API to access two different function pointers to
functions located in ROM, i.e., in firmware. These are the syscall
function and the Blake2s function. Since these operates in ROM, they
need a higher privilege compared to application mode. The only way
back into a raised privilege, is through the blake2s or syscall API.

The syscall function operates in firmware mode, except for not being
able to write to the sensitive assets, see the list of sensitive
assets further down.

The blake2s function operates in application mode, but ROM is
executable during the function call.

For a complete overview, see the modes and privileges depicted in the
table below:

| *name*          |  *ROM* | *FW RAM*  | *SPI*  | *Sensitive assets* |
|-----------------|--------|-----------|--------|--------------------|
| Firmware mode   | r/x    | r/w       |  r/w   | r/w*               |
| app mode        | r      | i         |  i     | r                  |
| syscall         | r/x    | r/w       |  r/w   | r                  |
| blake2s         | r/x    | i         |  i     | r                  |

Legend:
r = readable
w = writeable
x = executable
i = invisible
* = only writeable during first time in firmware mode


These sensitive assets are only readable and/or writeable in firmware
mode, before the first switch to app mode:
- ADDR_APP_START
- ADDR_APP_SIZE
- ADDR_BLAKE2S
- ADDR_SYSCALL
- ADDR_CDI_FIRST
- ADDR_CDI_LAST
- ADDR_RAM_ADDR_RAND
- ADDR_RAM_DATA_RAND
- ADDR_UDI_FIRST
- ADDR_UDI_LAST

Note that these assets have different properties, some are read-only
and some are write-only. The list above only shows if they are
restricted in app mode. See each individual API further down to find
out more about their properties.


```
ADDR_SYSTEM_MODE_CTRL: 0x08
```

This register is read-only and shows if the device is executing in FW
mode or in App mode. If set the device is in app mode.


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

These registers provide read-only information to the loaded app to
itself - where it was loaded and its size. The values are written by
FW as part of the loading of the app. The registers can't be written
after the `ADDR_SYSTEM_MODE_CTRL` has been set for the first time.


### Access to Blake2s

```
ADDR_BLAKE2S: 0x10
```

This register provides the 32-bit function pointer address to the
Blake2s hash function in the FW. It is written by FW during boot. The
register can't be written after the `ADDR_SYSTEM_MODE_CTRL` has been
set for the first time.

This register will default to an illegal address, so if it is left
unset by firmware and an application tries to call it the CPU will
halt.

### Syscall access

```
ADDR_SYSCALL: 0x12
```

This register provides the 32-bit function pointer address to the
syscall function in the FW. The syscall function provides access to
high privilege tasks in a secure manner, such as access to the SPI
flash.

The register is written by FW during boot. The register can't be
written after the `ADDR_SYSTEM_MODE_CTRL` has been set for the first
time.

This register will default to an illegal address, so if it is left
unset by firmware and an application tries to call it the CPU will
halt.


### Access to CDI

```
ADDR_CDI_FIRST: 0x20
ADDR_CDI_LAST:  0x27
```

These registers provide access to the 256-bit compound device secret
calculated by the FW as part of loading an application. The registers
are written by the FW. The register can't be written to after the
`ADDR_SYSTEM_MODE_CTRL` has been set for the first time. The CDI is
readable by apps, which can then use it as a base secret to generate
any other secrets required to carry out their intended use case.


### Access to UDI

```
ADDR_UDI_FIRST: 0x30
ADDR_UDI_LAST:  0x31
```

These read-only registers provide access to the 64-bit Unique Device
Identity (UDI). The register can only be read in firmware mode before
the first switch to app mode.

The two UDI words are stored using 32 named SB\_LUT4 FPGA multiplexer
(MUX) instances, identified in the source code as "udi\_rom\_idx". One
instance for each bit in core read_data output bus.

Each SB\_LUT4 MUX is able to store 16 bits of data, in total 512 bits.
But since the UDI is 64 bits, we only use the two LSBs in each MUX.
Note that only the LSB address of the SB\_LUT4 instances are connected
to the CPU address. This means that only the two LSBs in each MUX can
be addressed.

During build of the FPGA design, the UDI is set to a known bit
pattern, which means that the SB\_LUT4 instantiations are initialized
to a fixed bit pattern.

The tool 'patch\_uds\_udi.py' is used to replace the fixed bit pattern
with a unique bit pattern before generating the per device unique FPGA
bitstream. This allows us to generate these device unique FPGA
bitstreams without having to do a full FPGA build.


### RAM memory protection

```
ADDR_RAM_ADDR_RAND: 0x40
ADDR_RAM_DATA_RAND: 0x41
```

These write only registers control how the data in the RAM is
randomized as a way of protecting the data. The randomization is
implemented using a pseudo random number generator with a state
initialized by a seed.

The `ADDR_RAM_ADDR_RAND` store the seed for how the addresses are
randomized over the memory space. The `ADDR_RAM_DATA_RAND` store the
seed for how the data itself is randomized. FW writes random seed
values to these registers during boot.


### Security monitor

```
ADDR_CPU_MON_CTRL: 0x60
ADDR_CPU_MON_FIRST:0x61
ADDR_CPU_MON_LAST: 0x62
```

Monitors events and state changes in the SoC and handles security
violations. Currently checks for:

1. Trying to execute instructions in FW\_RAM. *Always enabled*
2. Trying to access RAM outside of the physical memory. *Always enabled*
3. Trying to execute instructions in ROM, while ROM being marked as
   non-executable. Except for the first instruction set in
   `ADDR_SYSCALL` and `ADDR_BLAKE2S`. *Always enabled*
4. Trying to execute instructions from a memory area in RAM defined by
   the application.

Number 1, 2 and 3 are always enabled. Number 4 is set and enabled by
the device application.

An application must write to the `ADDR_CPU_MON_FIRST` and
`ADDR_CPU_MON_LAST` first, before enabling the monitor by writing to
`ADDR_CPU_MON_CTRL`. This effectively marks the region defined as
data-only, and will protect against execution. Typically a application
developer will set this protection to cover the application stack
and/or heap. Once enabled the monitor can't be disabled, and the
registers can't be written.

Once enabled, if the CPU tries to read an instruction from the defined
area, the core will force the CPU to instead read an all zero
instruction, which is an illegal instruction. This will trigger the
CPU to enter its TRAP state, from which it can't return unless the
TKey is power cycled.

Another feature is that when the CPU traps the core will detect it and
start flashing the status LED with a red light, indicating that the
CPU is in a trapped state and no further execution is possible.

## SPI-master

The TK1 includes a minimal SPI-master that provides access to the
Winbond Flash memory mounted on the board. The SPI-master is byte
oriented and very minimalistic.

The SPI master can only be used in firmware mode.

In order to transfer more than a single byte, SW must read status and
write commands needed to send a sequence of bytes. In order to read
out a sequence of bytes from the memory, SW must send as many dummy
bytes as the data being read from the memory.

The SPI-master is controlled using a few API
addresses:

```
ADDR_SPI_EN:   0x80
ADDR_SPI_XFER: 0x81
ADDR_SPI_DATA: 0x82
```

`ADDR_SPI_EN` enables and disabled the SPI-master. Writing a 0x01 will
lower the SPI chip select to the memory. Writing a 0x00 will raise the
chip select.

Writing to the `ADDR_SPI_XFER` starts a byte transfer. Reading from
the address returns the status for the SPI-master. If the return value
is not zero, the SPI-master is ready to send a byte.

`ADDR_SPI_DATA` is the address used to send and receive a byte. data.
The least significant byte will be sent to the memory during a
transfer. The byte returned from the memory will be presented to SW if
the address is read after a transfer has completed.

The sequence of operations needed to perform is thus:

1. Activate the SPI-master by writing a 0x00000001 to ADDR_SPI_EN
2. Write a byte to `ADDR_SPI_DATA`
3. Read `ADDR_SPI_XFER` to check status. Repeat until the read
   operation returns non-zero value
4. Write to `ADDR_SPI_XFER`
5. Read `ADDR_SPI_XFER` to check status. Repeat until the read operation
   returns a non-zero value
6. Read out the received byte from `ADDR_SPI_DATA`
7. Repeat 2..6 as many times as needed to send a command and data to
   the memory and getting the expected status, data back.
8. Deactivate the SPI-master by writing 0x00000000 to `ADDR_SPI_EN`

The SPI connected memory on the board is the Winbond W25Q80. For
information about the memory including support commands and protocol,
see the datasheet:

https://www.mouser.se/datasheet/2/949/w25q80dv_dl_revh_10022015-1489677.pdf


## System Reset

The TK1 includes an ability for FW and applications to trigger a
hardware reset of the FPGA by writing to an API address.

The hardware reset will force all registers that are in the Tkey FPGA
design reset circuit to be reset to their default values. Basically
this is all registers in the Tkey FPGA design.

The reset will not clear the RAM. However since the CPU program
counter is reset to its reset vector, the CPU will unconditionally
start executing the FW. As part of the device initialization, the FW
will fill the RAM with random data, overwriting any app and
data present in the RAM before the reset was triggered.

The hardware reset will not force the FPGA to read its
configuration. That requires a power cycle of the Tkey device.

The reset is controlled by the following API address. Note that
any value written to the address will trigger the reset.

```
ADDR_SYSTEM_RESET: 0x70
```


## Implementation

The core is implemented as a single module. Future versions will
probably be separated into separate modules.


## Winbond Flash memory model

The testbench for the SPI master requires a memory model of the
Winbond Flash memory. The model [can be downloaded from
Winbond](https://www.winbond.com/hq/support/documentation/downloadV2022.jsp?__locale=en&xmlPath=/support/resources/.content/item/DA02-KAG049.html&level=2)
by providing the requested information and supplying the received
verification code.

From the downloaded file 'W25Q80DL.zip', please extract the file
W25Q80DL.v and place it in the 'tb' directory before building the
simulation model.

---
