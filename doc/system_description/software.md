
# NOTE: this document is outdated, an update is pending.

# MTA1-MKDF software

## Definitions
  * Firmware -- software that is part of ROM, and is supplied via the
    FPGA bit stream
  * Secure Application (short: Application) -- software supplied by
    the host machine, which is received, measured, and loaded by the
    firmware.

## Types
The PicoRV32 is a 32-bit RISC-V system. All types are little-endian.

## Constraints
The application FPGA is a Lattice UP5K, with the following
specifications:
  * 32KB x 4 SPRAM => 128KB for Application
  * 4Kb x 30 EBR => 120Kb, PicoRV32 uses ~4 EBR internally => 13KB for
    Firmware. We should probably aim for less; 8KB should be the
    target.

## Introduction

The MTA1_MKDF has two modes of operation; firmware/loader mode and
application mode. The firmware mode has the responsibility of receive,
measure, and load the application.

The firmware and application uses a memory mapped IO for SoC
communication. The memory map resides at `0x9000_0000`. The
application has a constrained variant of the firmware memory map,
which is outlined below. E.g. UID and UDA are not readable, and the
`APP_{ADDR, SIZE}` are not writable for the application.

The MTA1_MKDF software communicates to the host via the `{RX,TX}_FIFO`
registers, using the framing protocol described in [Framing
Protocol](../framing_protocol/framing_protocol.md).

The firmware defines a protocol (command/response interface) on top of
the framing layer, which is used to bootstrap the application onto the
device.

On the framing layer, it's required that each frame the device
receives, a responding frame must be sent back to the host, in a
ping-pong manner.

Applications define a per-application protocol, which is the contract
between the host and the device.

## Firmware

The firware is part of FPGA bitstream (ROM), and is loaded at
`0x0000_1000`.

### Reset

The PicoRV32 executes `_start` from `crt0.S` `.text` at `0x0000_1000`,
which initializes the stack, `.data`, and `.bss` at
`0x8000_0000`. When the initialization is finished, the firmware waits
for incoming commands from the host, by busy-polling the
`RX_FIFO_{AVAILABLE,DATA}`registers. When a complete command is read,
the firmware executes the command.

### Loading an application

The purpose of the firmware is to bootstrapping an application.
  1. The host sends a raw binary, targeted to be loaded at
     `0x8000_0000` in the device. The host starts off by sending the
     binary size using the `FW_CMD_LOAD_APP_SIZE` command.
  2. The firmware executes `FW_CMD_LOAD_APP_SIZE` command, which
     stores the application size into `APP_SIZE`, and sets `APP_ADDR`
     to zero. A `FW_RSP_LOAD_APP_SIZE` reponse is sent back to the
     host, with the status of the action (ok/fail).
  3. If the the host receive a sucessful command, it will send
     multiple `FW_CMD_LOAD_APP_DATA` commands, containing the full
     application.
  4. For each received `FW_CMD_LOAD_APP_DATA` commands, the firmware
     measures (XXX define how blake2s is used) the data, and places it
     into `0x8000_0000`. The firmware response with
     `FW_RSP_LOAD_APP_DATA` response to the host for each received
     block.
  5. When the final block of the application image is received,
     `0x8000_0000` is written to `APP_ADDR`. The `CDI` is computed by
     used the `UDS` and measurement from the application, and placed
     in the `CDI` register. The final `FW_RSP_LOAD_APP_DATA` response
     is sent to the host, completing the loading.

NOTE: The firmware uses SPRAM for data and stack. We need to make sure
that the application image does not overwrite the firmware's running
state. The application should probably do a similar relocation for
stack/data at reset, as the firmware does. Further; the firmware need
to check application image is sane. The shared firmware data area
(e.g. `.data` and the stack must be cleared prior launching the
application.

### Starting an application

Starting an application includes the "switch to application mode"
step, which is done by writing to the `SWITCH_APP` regiester. The
switch from firmware mode to application mode is a mode switch, and
context switch. Enter application mode, means the the MMIO region is
restricted; E.g. some registers are removed (`UDS`), and some are
switched from read/write to read-only. This is outlined in the memory
map below.

There is no other means of getting back from application mode to
firmware mode, than resetting/power cycling the device.

Prerequisites: `APP_SIZE` and `APP_ADDR` has to be non-zero.
  1. The host sends `FW_CMD_RUN_APP` to the device.
  2. The firmware respons with `FW_RSP_RUN_APP`
  3. The firmware writes a non-zero to `SWITCH_APP`, and executes
  ```
    // a0 = 0x9000_0000 + 0x420 (APP_ADDR address)
    lw   a0,1056(a0)
    jalr x0,0(a0)
  ```
  4. The device is now in application mode, and executes the code from
     `0x8000_0000`.

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

XXX Should we think a bit more about versioning/possiblity to extend?
Is 1B enough for a command/response range?


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

  CMD[0].len = 5    // command frame format
  CMD[1]     = 0x03 // FW_CMD_LOAD_APP_SIZE

  CMD[2..6]  = APP_SIZE

  CMD[6..]   = 0

host <-
  u8 RSP[1 + 4];

  RSP[0].len = 5    // command frame format
  RSP[1]     = 0x04 // FW_RSP_LOAD_APP_SIZE

  RSP[2]     = STATUS

  RSP[3..]   = 0


repeat ceil(APP_SIZE / 63) times:
host ->
  u8 CMD[1 + 64];

  CMD[0].len = 65   // command frame format
  CMD[1]     = 0x05 // FW_CMD_LOAD_APP_DATA

  CMD[2..]   = APP_DATA (pad with zeros)

host <-
  u8 RSP[1 + 4]

  RSP[0].len = 5    // command frame format
  RSP[1]     = 0x06 // FW_RSP_LOAD_APP_DATA

  RSP[2]     = STATUS

  RSP[3..]   = 0
```

### Memory map

The memory map exposes SoC functionality to the software, when in
firmware mode (privileged mode) It is s set of memory mapped
registers, starting at base address `0x9000_0000`.

| *name*             | *r/w* | *offset* | *size* | *type*  | *content* | *description*                                           |
|--------------------|-------|----------|--------|---------|-----------|---------------------------------------------------------|
| UDS[^1]            | r     | 0x0      | 32B    | u8[32]  |           | Unique Device Secret key.                               |
| UDA                | r     | 0x20     | 16B    | u8[16]  |           | Unique Device Authentication key.                       |
| SWITCH_APP         | w     | 0x30     | 1B     | u8      |           | Switch to application mode. Write non-zero to trigger.  |
| XXX 460 bytes hole |       |          |        |         |           |                                                         |
| UDI                | r     | 0x200    | 8B     | u64     |           | Unique Device ID (UDI).                                 |
| NAME0              | r     | 0x208    | 4B     | char[4] | "mta1"    |                                                         |
| NAME1              | r     | 0x20c    | 4B     | char[4] | "mkdf"    |                                                         |
| VERSION            | r     | 0x210    | 4B     | u32     | 1         | Current version.                                        |
| RX_FIFO_AVAILABLE  | r     | 0x214    | 1B     | u8      |           | Non-zero if a valid byte can be read from RX_FIFO_DATA. |
| RX_FIFO_DATA       | r     | 0x215    | 1B     | u8      |           | FIFO Rx data.                                           |
| TX_FIFO_AVAILABLE  | r     | 0x216    | 1B     | u8      |           | Non-zero if a valid byte can be written to TX_FIFO_DATA |
| TX_FIFO_DATA       | w     | 0x217    | 1B     | u8      |           | FIFO Tx data.                                           |
| LED                | r/w   | 0x218    | 4B     | u32     |           | LED                                                     |
| COUNTER            | r     | 0x21c    | 4B     | u32     |           | Counter                                                 |
| TRNG_STATUS        | r     | 0x220    | 4B     | u32     |           | data_ready/error                                        |
| TRNG_DATA          | r     | 0x224    | 4B     | u32     |           | TRNG data                                               |
| XXX 472 bytes hole |       |          |        |         |           |                                                         |
| CDI                | r/w   | 0x400    | 32B    | u8[32]  |           | Compound Device Identifier (CDI). UDS+measurement...    |
| APP_ADDR           | r/w   | 0x420    | 4B     | u32     |           | Application address (0x8000_0000)                       |
| APP_SIZE           | r/w   | 0x424    | 4B     | u32     |           | Application size                                        |

[^1]: The UDS can only be read *once* per power-cycle.

## Application

### Memory map
See the [Memory model](./memory_model.md) for information about the
memory map and how access to memory areas work.
