# Tillitis Key software

## Definitions

  * Firmware -- software that is part of ROM, and is currently
    supplied via the FPGA bit stream.
  * Application -- software supplied by the host machine, which is
    received, loaded, and measured by the firmware (by hashing a
    digest over the binary).

Learn more about the concepts in the
[system_description.md](system_description.md).

## CPU

We use a PicoRV32, a 32-bit RISC-V system (RV32IMC), as the CPU for
running the firmware and the loaded app. The firmware and device app
both run in machine mode. All types are little-endian.

## Constraints

The application FPGA is a Lattice ICE40 UP5K, with the following
specifications:

  * 30 EBR[^1] x 4 Kbit => 120 Kbit. PicoRV32 uses ~4 EBRs internally
    => 13 KB for Firmware. We should probably aim for less; 8 KB
    should be the target.
  * 4 SPRAM[^2] x 32 KB => 128 KB RAM for application/software

[^1]: Embedded Block RAM (also BRAM) residing in the FPGA, can
    be configured as RAM or ROM.
[^2]: Single Port RAM (also SRAM).

## Introduction

The Tillitis Key has two modes of operation; firmware/loader mode and
application mode. The firmware mode has the responsibility of
receiving, measuring, loading, and starting the application.

The firmware and application uses a memory mapped IO for SoC
communication. This MMIO resides at `0xc000_0000`. *Nota bene*: Almost
all access to MMIO should be word (32 bit) aligned. See table below.

The application has a constrained variant of the firmware memory map,
which is outlined below. E.g. UDS isn't readable, and the `APP_{ADDR,
SIZE}` are not writable for the application.

The firmware (and optionally all software) on the Tillitis Key
communicates to the host via the `UART_{RX,TX}_{STATUS,DATA}`
registers, using the framing protocol described in [Framing
Protocol](../framing_protocol/framing_protocol.md).

The firmware defines a protocol (command/response interface) on top of
this framing layer which is used to bootstrap the application onto the
device. All commands are initiated by the host. All commands receive a
reply.

Applications define their own per-application protocol used for
communication with their host part. They may or may not be based on
the Framing Protocol.

## Firmware

The device has 128 KB RAM. The current firmware loads the app at the
upper 100 KB. The lower 28 KB is set up as stack for the app. A
smaller app that wants continuous memory may want to relocate itself
when starting.

The firmware is part of FPGA bitstream (ROM), and is loaded at
`0x0000_0000`.

### Reset

The PicoRV32 starts executing at `0x0000_0000`. Our firmware starts at
`_start` from `start.S` which initializes the `.data`, and `.bss` at
`0x4000_0000` and upwards. A stack is also initialized, starting at
0x4000_6ff0 and downwards. When the initialization is finished, the
firmware waits for incoming commands from the host, by busy-polling
the `UART_RX_{STATUS,DATA}` registers. When a complete command is
read, the firmware executes the command.

### Loading an application

The purpose of the firmware is to bootstrap an application. The host
will send a raw binary targeted to be loaded at `0x4000_7000` in the
device.

  1. The host sends the `FW_CMD_LOAD_APP` command with the size of the
     device app and the user-supplied secret as arguments and and gets
     a `FW_RSP_LOAD_APP` back.
  2. If the the host receive a sucessful response, it will send
     multiple `FW_CMD_LOAD_APP_DATA` commands, together containing the
     full application.
  3. On receiving`FW_CMD_LOAD_APP_DATA` commands the firmware places
     the data into `0x4000_7000` and upwards. The firmware replies
     with a `FW_RSP_LOAD_APP_DATA` response to the host for each
     received block except the last data block.
  4. When the final block of the application image is received with a
     `FW_CMD_LOAD_APP_DATA`, the firmware measure the application by
     computing a BLAKE2s digest over the entire application. Then
     firmware send back the `FW_RSP_LOAD_APP_DATA_READY` response
     containing the measurement.
  5. The Compound Device Identifier (CDI) is then computed by using
     the `UDS`, the measurement of the application, and the `USS`, and
     placed in the `CDI` register. Then the start address of the
     device app, `0x4000_7000`, is written to `APP_ADDR` and the size
     to `APP_SIZE` to let the device application know where it is
     loaded and how large it is, if it wants to relocate in RAM.
  6. The firmware now starts the application by switching to
     application mode by writing to the `SWITCH_APP` register. In this
     mode the MMIO region is restricted; e.g. some registers are
     removed (`UDS`), and some are switched from read/write to
     read-only. This is outlined in the memory map below.

     The firmware now executes assembler code that writes zeros to
     stack and data of the firmware, then jumps to what's in
     `APP_ADDR` which starts device app execution.

     There is now no other means of getting back from application mode
     to firmware mode than resetting/power cycling the device.

### User-supplied Secret (USS)

USS is a 32 bytes long secret provided by the user. Typically a host
program gets a passphrase from the user and then does KDF of some
sort, for instance a BLAKE2s, to get 32 bytes which it sends to the
firmware to be part of the CDI computation.

### Compound Device Identifier (CDI) computation

The CDI is computed by:

```
CDI = blake2s(UDS, blake2s(app), USS)
```

In an ideal world, software would never be able to read UDS at all and
we would have a BLAKE2s function in hardware that would be the only
thing able to read the UDS. Unfortunately, we couldn't fit a BLAKE2s
implementation in the FPGA at this time.

The firmware instead does the CDI computation using the special
firmware-only `FW_RAM` which is invisible after switching to app mode.
The `blake2s()` function in the firmware is fed with a buffer in
`FW_RAM` containing the UDS, the app digest, and the USS. It is also
fed with a context used for computations that is also part of the
`FW_RAM`.

After doing the computation `FW_RAM` is also cleared with zeroes.

### Firmware protocol definition

The firmware commands and responses are built on top of the [Framing
Protocol](../framing_protocol/framing_protocol.md).

The commands look like this:

| *name*           | *size (bytes)* | *comment*                                |
|------------------|----------------|------------------------------------------|
| Header           | 1              | Framing protocol header including length |
| Command/Response | 1              | Any of the below commands or responses   |
| Data             | n              | Any additional data                      |

The responses might include a one byte status field where 0 is
`STATUS_OK` and 1 is `STATUS_BAD`.

Note that the integer types are little-endian (LE).

#### `FW_CMD_NAME_VERSION` (0x01)

Get the name and version of the stick.

#### `FW_RSP_NAME_VERSION` (0x02)

| *name*  | *size (bytes)* | *comment*            |
|---------|----------------|----------------------|
| name0   | 4              | ASCII                |
| name1   | 4              | ASCII                |
| version | 4              | Integer version (LE) |

In a bad response the fields will be zeroed.

#### `FW_CMD_LOAD_APP` (0x03)

| *name*       | *size (bytes)* | *comment*           |
|--------------|----------------|---------------------|
| size         | 4              | Integer (LE)        |
| uss-provided | 1              | 0 = false, 1 = true |
| uss          | 32             | Ignored if above 0  |

Start an application loading session by setting the size of the
expected device application and a user-supplied secret, if
`uss-provided` is 1. Otherwise `USS` is ignored.

#### `FW_RSP_LOAD_APP` (0x04)

Response to `FW_CMD_LOAD_APP`

| *name* | *size (bytes)* | *comment*                   |
|--------|----------------|-----------------------------|
| status | 1              | `STATUS_OK` or `STATUS_BAD` |

#### `FW_CMD_LOAD_APP_DATA` (0x05)

| *name* | *size (bytes)* | *comment*           |
|--------|----------------|---------------------|
| data   | 127            | Raw binary app data |

Load 127 bytes of raw app binary into device RAM. Should be sent
consecutively over the complete raw binary.

#### `FW_RSP_LOAD_APP_DATA` (0x06)

Response to all but the ultimate `FW_CMD_LOAD_APP_DATA` commands.

| *name* | *size (bytes)* | *comment*                |
|--------|----------------|--------------------------|
| status | 1              | `STATUS_OK`/`STATUS_BAD` |

#### `FW_RSP_LOAD_APP_DATA_READY` (0x07)

The response to the last `FW_CMD_LOAD_APP_DATA` is an
`FW_RSP_LOAD_APP_DATA_READY` with the un-keyed hash digest for the
application that was loaded. It allows the host to verify that the
application was correctly loaded. This means that the CDI calculated
will be correct given that the UDS has not been modified.

| *name* | *size (bytes)* | *comment*                |
|--------|----------------|--------------------------|
| status | 1              | `STATUS_OK`/`STATUS_BAD` |
| digest | 32             | BLAKE2s(app)             |

#### `FW_CMD_GET_UDI` (0x08)

Ask for the Unique Device Identifier (UDI) of the device.

#### `FW_RSP_GET_UDI` (0x09)

Response to `FW_CMD_GET_UDI`.

| *name* | *size (bytes)* | *comment*                                           |
|--------|----------------|-----------------------------------------------------|
| status | 1              | `STATUS_OK`/`STATUS_BAD`                            |
| udi    | 4              | Integer (LE) with Reserved (4 bit), Vendor (2 byte),|
|        |                | Product (1 byte), Revision (4 bit)                  |
| udi    | 4              | Integer serial number (LE)                          |


#### Get the name and version of the device

```
host ->
  u8 CMD[1 + 1];

  CMD[0].len = 1    // command frame format
  CMD[1]     = 0x01 // FW_CMD_NAME_VERSION

host <-
  u8 RSP[1 + 32]

  RSP[0].len  = 32   // command frame format
  RSP[1]      = 0x02 // FW_RSP_NAME_VERSION

  RSP[2..6]   = NAME0
  RSP[6..10]  = NAME1
  RSP[10..14] = VERSION

  RSP[14..]   = 0
```

#### Load an application

```
host ->
  u8 CMD[1 + 128];

  CMD[0].len = 128  // command frame format
  CMD[1]     = 0x03 // FW_CMD_LOAD_APP

  CMD[2..6]  = APP_SIZE

  CMD[6]     = USS supplied? 0 = false, 1 = true
  CMD[7..39] = USS
  CMD[40..]  = 0

host <-
  u8 RSP[1 + 4];

  RSP[0].len = 4    // command frame format
  RSP[1]     = 0x04 // FW_RSP_LOAD_APP

  RSP[2]     = STATUS

  RSP[3..]   = 0

repeat ceil(APP_SIZE / 127) times:
host ->
  u8 CMD[1 + 128];

  CMD[0].len = 128  // command frame format
  CMD[1]     = 0x05 // FW_CMD_LOAD_APP_DATA

  CMD[2..]   = APP_DATA (127 bytes of app data, pad with zeros)

host <-
  u8 RSP[1 + 4]

  RSP[0].len = 4    // command frame format
  RSP[1]     = 0x06 // FW_RSP_LOAD_APP_DATA

  RSP[2]     = STATUS

  RSP[3..]   = 0
```

Except response from last chunk of app data which is:

```
host <-
  u8 RSP[1 + 4]

  RSP[0].len = 128   // command frame format
  RSP[1]     = 0x07 // FW_RSP_LOAD_APP_DATA_READY

  RSP[2]     = STATUS

  RSP[3..35]   = app digest
  RSP[36..]    = 0
```

### Memory map

Assigned top level prefixes:

| *name*   | *prefix* | *address length*                     |
|----------|----------|--------------------------------------|
| ROM      | 0b00     | 30 bit address                       |
| RAM      | 0b01     | 30 bit address                       |
| reserved | 0b10     |                                      |
| MMIO     | 0b11     | 6 bits for core select, 24 bits rest |

Addressing:

```
31st bit                              0th bit
v                                     v
0000 0000 0000 0000 0000 0000 0000 0000

- Bits [31 .. 30] (2 bits): Top level prefix (described above)
- Bits [29 .. 24] (6 bits): Core select. We want to support at least 16 cores
- Bits [23 ..  0] (24 bits): Memory/in-core address.
```

The memory exposes SoC functionality to the software when in firmware
mode. It is a set of memory mapped registers (MMIO), starting at base
address `0xc000_0000`. For specific offsets/bitmasks, see the file
[tk1_mem.h](../../hw/application_fpga/fw/tk1_mem.h) (in this repo).

Assigned core prefixes:

| *name* | *prefix* |
|--------|----------|
| ROM    | 0x00     |
| RAM    | 0x40     |
| TRNG   | 0xc0     |
| TIMER  | 0xc1     |
| UDS    | 0xc2     |
| UART   | 0xc3     |
| TOUCH  | 0xc4     |
| FW_RAM | 0xd0     |
| TK1    | 0xff     |

*Nota bene*: MMIO accesses should be 32 bit wide, e.g use `lw` and
`sw`. Exceptions are `FW_RAM` and `QEMU_DEBUG`.

| *name*            | *fw*  | *app*     | *size* | *type*   | *content* | *description*                                                          |
|-------------------|-------|-----------|--------|----------|-----------|------------------------------------------------------------------------|
| `TRNG_STATUS`     | r     | r         |        |          |           | TRNG_STATUS_READY_BIT is 1 when an entropy word is available.          |
| `TRNG_ENTROPY`    | r     | r         | 4B     | u32      |           | Entropy word. Reading a word will clear status.                        |
| `TIMER_CTRL`      | r/w   | r/w       |        |          |           | If TIMER_STATUS_READY_BIT in TIMER_STATUS is 1, writing anything here  |
|                   |       |           |        |          |           | starts the timer. If the same bit is 0 then writing stops the timer.   |
| `TIMER_STATUS`    | r     | r         |        |          |           | TIMER_STATUS_READY_BIT is 1 when timer is ready to start running.      |
| `TIMER_PRESCALER` | r/w   | r/w       | 4B     |          |           | Prescaler init value. Write blocked when running.                      |
| `TIMER_TIMER`     | r/w   | r/w       | 4B     |          |           | Timer init or current value while running. Write blocked when running. |
| `UDS_FIRST`       | r[^3] | invisible | 4B     | u8[32]   |           | First word of Unique Device Secret key.                                |
| `UDS_LAST`        |       | invisible |        |          |           | The last word of the UDS                                               |
| `UART_BITRATE`    | r/w   |           |        |          |           | TBD                                                                    |
| `UART_DATABITS`   | r/w   |           |        |          |           | TBD                                                                    |
| `UART_STOPBITS`   | r/w   |           |        |          |           | TBD                                                                    |
| `UART_RX_STATUS`  | r     | r         | 1B     | u8       |           | Non-zero when there is data to read                                    |
| `UART_RX_DATA`    | r     | r         | 1B     | u8       |           | Data to read. Only LSB contains data                                   |
| `UART_TX_STATUS`  | r     | r         | 1B     | u8       |           | Non-zero when it's OK to write data                                    |
| `UART_TX_DATA`    | w     | w         | 1B     | u8       |           | Data to send. Only LSB contains data                                   |
| `TOUCH_STATUS`    | r/w   | r/w       |        |          |           | TOUCH_STATUS_EVENT_BIT is 1 when touched. After detecting a touch      |
|                   |       |           |        |          |           | event (reading a 1), write anything here to acknowledge it.            |
| `FW_RAM`          | r/w   | invisible | 1 kiB  | u8[1024] |           | Firmware-only RAM.                                                     |
| `UDA`             | r     | invisible | 16B    | u8[16]   |           | Unique Device Authentication key.                                      |
| `UDI`             | r     | r         | 8B     | u64      |           | Unique Device ID (UDI).                                                |
| `QEMU_DEBUG`      | w     | w         |        | u8       |           | Debug console (only in QEMU)                                           |
| `NAME0`           | r     | r         | 4B     | char[4]  | "tk1 "    | ID of core/stick                                                       |
| `NAME1`           | r     | r         | 4B     | char[4]  | "mkdf"    | ID of core/stick                                                       |
| `VERSION`         | r     | r         | 4B     | u32      | 1         | Current version.                                                       |
| `SWITCH_APP`      | r/w   | r         | 1B     | u8       |           | Write anything here to trigger the switch to application mode. Reading |
|                   |       |           |        |          |           | returns 0 if device is in firmware mode, 0xffffffff if in app mode.    |
| `LED`             | w     | w         | 1B     | u8       |           |                                                                        |
| `GPIO`            |       |           |        |          |           |                                                                        |
| `APP_ADDR`        | r/w   | r         | 4B     | u32      |           | Firmware stores app load address here, so app can read its own location|
| `APP_SIZE`        | r/w   | r         | 4B     | u32      |           | Firmware stores app app size here, so app can read its own size        |
| `CDI_FIRST`       | r/w   | r         | 32B    | u8[32]   |           | Compound Device Identifier (CDI). UDS+measurement...                   |
| `CDI_LAST`        |       | r         |        |          |           | Last word of CDI                                                       |

[^3]: The UDS can only be read *once* per power-cycle.
