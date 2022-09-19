# Tillitis Key software

## Definitions

  * Firmware -- software that is part of ROM, and is currently
    supplied via the FPGA bit stream.
  * Application -- software supplied by the host machine, which is
    received, loaded, and measured by the firmware.

## CPU

We use a PicoRV32, a 32-bit RISC-V system, as the CPU for running the
firmware and the loaded app. All types are little-endian.

## Constraints

The application FPGA is a Lattice UP5K, with the following
specifications:

  * 32KB x 4 SPRAM => 128KB for Application
  * 4Kb x 30 EBR => 120Kb, PicoRV32 uses ~4 EBR internally => 13KB for
    Firmware. We should probably aim for less; 8KB should be the
    target.

## Introduction

The Tillitis Key has two modes of operation; firmware/loader mode and
application mode. The firmware mode has the responsibility of
receiving, measuring, and loading the application.

The firmware and application uses a memory mapped IO for SoC
communication. This MMIO resides at `0xc000_0000`. *Nota bene*: All
access to MMIO should be word (32 bit) aligned.

The application has a constrained variant of the firmware memory map,
which is outlined below. E.g. UDS isn't readable, and the `APP_{ADDR,
SIZE}` are not writable for the application.

The software on the Tillitis Key communicates to the host via the
`{RX,TX}_FIFO` registers, using the framing protocol described in
[Framing Protocol](../framing_protocol/framing_protocol.md).

The firmware defines a protocol (command/response interface) on top of
the framing layer, which is used to bootstrap the application onto the
device.

On the framing layer, it's required that each frame the device
receives, a responding frame must be sent back to the host, in a
ping-pong manner.

Applications define a per-application protocol, which is the contract
between the host and the device.

## Firmware

The device has 128 kB RAM. The current firmware loads the app at the
upper 64 kB. The lower 64 kB is currently set up as stack for the app.

The firmware is part of FPGA bitstream (ROM), and is loaded at
`0x0000_0000`.

### Reset

The PicoRV32 starts executing at `0x0000_0000`. Our firmware starts at
`_start` from `start.S` which initializes the `.data`, and `.bss` at
`0x4000_0000` and upwards. A stack is also initialized, starting at
0x4000_fff0 and downwards. When the initialization is finished, the
firmware waits for incoming commands from the host, by busy-polling
the `RX_FIFO_{AVAILABLE,DATA}`registers. When a complete command is
read, the firmware executes the command.

### Loading an application

The purpose of the firmware is to bootstrap an application. The host
will send a raw binary targeted to be loaded at `0x4001_0000` in the
device.

  1. The host sends sets the size of the app by using the
     `FW_CMD_LOAD_APP_SIZE` command.
  2. The firmware executes `FW_CMD_LOAD_APP_SIZE` command, which
     stores the application size into `APP_SIZE`, and sets `APP_ADDR`
     to zero. A `FW_RSP_LOAD_APP_SIZE` reponse is sent back to the
     host, with the status of the action (ok/fail).
  3. If the the host receive a sucessful command, it will send
     multiple `FW_CMD_LOAD_APP_DATA` commands, containing the full
     application.
  4. For each received `FW_CMD_LOAD_APP_DATA` command the firmware
     places the data into `0x4001_0000` and upwards. The firmware
     response with `FW_RSP_LOAD_APP_DATA` response to the host for
     each received block.
  5. When the final block of the application image is received, we
     measure the application by computing a BLAKE2s digest over the
     entire application,

     The Compound Device Identifier is computed by using the `UDS` and
     the measurement of the application, and placed in the `CDI`
     register. Then `0x4001_0000` is written to `APP_ADDR`. The final
     `FW_RSP_LOAD_APP_DATA` response is sent to the host, completing
     the loading.

NOTE: The firmware uses SPRAM for data and stack. We need to make sure
that the application image does not overwrite the firmware's running
state. The application should probably do a similar relocation for
stack/data at reset, as the firmware does. Further; the firmware need
to check application image is sane. The shared firmware data area
(e.g. `.data` and the stack must be cleared prior launching the
application.

### Starting an application

Starting an application includes the "switch to application mode"
step, which is done by writing to the `SWITCH_APP` register. The
switch from firmware mode to application mode is a mode switch, and
context switch. When entering application mode the MMIO region is
restricted; e.g. some registers are removed (`UDS`), and some are
switched from read/write to read-only. This is outlined in the memory
map below.

There is no other means of getting back from application mode to
firmware mode than resetting/power cycling the device.

Prerequisites: `APP_SIZE` and `APP_ADDR` has to be non-zero.
Procedure:

  1. The host sends `FW_CMD_RUN_APP` to the device.
  2. The firmware responds with `FW_RSP_RUN_APP`
  3. The firmware writes a non-zero to `SWITCH_APP`, and executes
     assembler code that writes zeros to stack and data of the
     firmware, then jumps to what's in APP_ADDR.
  4. The device is now in application mode and is executing the
     application.

### Protocol definition

Available commands/reponses:

#### `FW_{CMD,RSP}_LOAD_APP_SIZE`
#### `FW_{CMD,RSP}_LOAD_APP_DATA`
#### `FW_{CMD,RSP}_RUN_APP`
#### `FW_{CMD,RSP}_NAME_VERSION`
#### `FW_{CMD,RSP}_UID`
#### `FW_{CMD,RSP}_TRNG_DATA`
#### `FW_{CMD,RSP}_TRNG_STATUS`

#### `FW_{CMD,RSP}_VERIFY_DEVICE`

Verification that the device is an authentic Mullvad
device. Implemented using challenge/response.

#### `FW_{CMD,RSP}_GET_APPLICATION_DIGEST`

This command returns the un-keyed hash digest for the application that
was loaded. It allows the host to verify that the application was
correctly loaded. This means that the CDI calculated will be correct
given that the UDS has not been modified.

#### Get the name and version of the device

```
host ->
  u8 CMD[1 + 1];

  CMD[0].len = 1    // command frame format
  CMD[1]     = 0x01 // FW_CMD_NAME_VERSION

host <-
  u8 RSP[1 + 32]

  RSP[0].len  = 33   // command frame format
  RSP[1]      = 0x02 // FW_RSP_NAME_VERSION

  RSP[2..6]   = NAME0
  RSP[6..10]  = NAME1
  RSP[10..14] = VERSION

  RSP[14..]   = 0
```

#### Load an application

```
host ->
  u8 CMD[1 + 32];

  CMD[0].len = 4    // command frame format
  CMD[1]     = 0x03 // FW_CMD_LOAD_APP_SIZE

  CMD[2..6]  = APP_SIZE

  CMD[6..]   = 0

host <-
  u8 RSP[1 + 4];

  RSP[0].len = 5    // command frame format
  RSP[1]     = 0x04 // FW_RSP_LOAD_APP_SIZE

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
[mta1_mkdf_mem.h](../../hw/application_fpga/fw/mta1_mkdf_mem.h) (in
this repo).

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
| MTA1   | 0xff     |
|        |          |

Every core has a NAME1, NAME2, and VERSION to identify it.

Examples:

| *address*  | *description*   |
|------------|-----------------|
| 0xc3000004 | NAME1 in UART   |
| 0xff000000 | NAME0 in MTA1   |
| 0xff000008 | VERSION in MTA1 |

*Nota bene*: All MMIO accesses should be 32 bit wide, e.g use `lw` and `sw`.

| *name*             | *fw* | *app       | *size* | *type*  | *content* | *description*                                          |
|--------------------|------|------------|--------|---------|-----------|--------------------------------------------------------|
| `TRNG_NAME0`       | r    | r          | 4B     | char[4] |           | ID of core                                             |
| `TRNG_NAME1`       | r    | r          | 4B     | char[4] |           | ID of core                                             |
| `TRNG_VERSION`     | r    | r          | 4B     | u32     |           | Version of core                                        |
| `TRNG_STATUS`      | r    | r          |        |         |           | TBD                                                    |
| `TRNG_SAMPLE_RATE` |      | r          |        |         |           | TBD                                                    |
| `TRNG_ENTROPY`     |      |            |        |         |           | TBD                                                    |
| `TIMER_NAME0`      | r    | r          |        |         |           | ID of core                                             |
| `TIMER_NAME1`      | r    | r          |        |         |           | ID of core                                             |
| `TIMER_VERSION`    | r    | r          |        |         |           | Version of core                                        |
| `TIMER_CTRL`       |      |            |        |         |           | TBD                                                    |
| `TIMER_STATUS`     | r    |            |        |         |           | TBD                                                    |
| `TIMER_PRESCALER`  |      | r/w        |        |         |           | TBD                                                    |
| `TIMER_TIMER`      |      | r          |        |         |           | TBD                                                    |
| `UDS_NAME0`        | r    | invisible  |        |         |           | ID of core                                             |
| `UDS_NAME1`        | r    | invisible  |        |         |           | ID of core                                             |
| `UDS_VERSION`      | r    | invisible  |        |         |           | Version of core                                        |
| `UDS_START`        | r[^1]| invisible  | 4B     | u8[32]  |           | First word of Unique Device Secret key.                |
| `UDS_LAST`         |      | invisible  |        |         |           | The last word of the UDS                               |
| `UART_NAME0`       | r    | r          |        |         |           | ID of core                                             |
| `UART_NAME1`       | r    | r          |        |         |           | ID of core                                             |
| `UART_VERSION`     | r    | r          |        |         |           | Version of core                                        |
| `UART_BITRATE`     | r/w  |            |        |         |           | TBD                                                    |
| `UART_DATABITS`    | r/w  |            |        |         |           | TBD                                                    |
| `UART_STOPBITS`    | r/w  |            |        |         |           | TBD                                                    |
| `UART_RX_STATUS`   | r    | r          | 1B     | u8      |           | Non-zero when there is data to read                    |
| `UART_RX_DATA`     | r    | r          | 1B     | u8      |           | Data to read. Only LSB contains data                   |
| `UART_TX_STATUS`   | r    | r          | 1B     | u8      |           | Non-zero when it's OK to write data                    |
| `UART_TX_DATA`     | w    | w          | 1B     | u8      |           | Data to send. Only LSB contains data                   |
| `TOUCH_NAME0`      | r    | r          |        |         |           | ID of core                                             |
| `TOUCH_NAME1`      | r    | r          |        |         |           | ID of core                                             |
| `TOUCH_VERSION`    | r    | r          |        |         |           | Version of core                                        |
| `TOUCH_STATUS`     | r/w  | r/w        |        |         |           | STATUS_EVENT_BIT set 1 when touched; write to it after |
| `UDA`              | r    |            | 16B    | u8[16]  |           | Unique Device Authentication key.                      |
| `UDI`              | r    |            | 8B     | u64     |           | Unique Device ID (UDI).                                |
| `QEMU_DEBUG`       | w    | w          |        | u8      |           | Debug console (only in QEMU)                           |
| `NAME0`            | r    | r          | 4B     | char[4] | "mta1"    | ID of core/stick                                       |
| `NAME1`            | r    | r          | 4B     | char[4] | "mkdf"    | ID of core/stick                                       |
| `VERSION`          | r    | r          | 4B     | u32     | 1         | Current version.                                       |
| `SWITCH_APP`       | w    | invisible? | 1B     | u8      |           | Switch to application mode. Write non-zero to trigger. |
| `LED`              | w    | w          | 1B     | u8      |           |                                                        |
| `GPIO`             |      |            |        |         |           |                                                        |
| `APP_ADDR`         | r/w  | r          | 4B     | u32     |           | Application address (0x4000_0000)                      |
| `APP_SIZE`         | r/w  | r          | 4B     | u32     |           | Application size                                       |
| `DEBUG`            |      |            |        |         |           | TBD                                                    |
| `CDI_START`        | r/w  | r          | 32B    | u8[32]  |           | Compound Device Identifier (CDI). UDS+measurement...   |
| `CDI_LAST`         |      | r          |        |         |           | Last word of CDI                                       |

[^1]: The UDS can only be read *once* per power-cycle.
