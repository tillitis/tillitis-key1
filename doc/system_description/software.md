# Tillitis TKey software

**NOTE:** Documentation migrated to dev.tillitis.se, this is kept for
history. This is likely to be outdated.

## Introduction

This text is both an introduction to and a requirement specification
of the TKey firmware, its protocol, and an overview of how TKey
applications are supposed to work. For an overview of the TKey
concepts, see [System Description](system_description.md).

First, some definitions:

- Firmware - software in ROM responsible for loading applications. The
  firmware is included as part of the FPGA bit stream.
- Application or app - software supplied by the host machine which is
  received, loaded, measured, and started by the firmware.

The TKey has two modes of software operation: firmware mode and
application mode. The firmware mode has the responsibility of
receiving, measuring, loading, and starting the application. When the
firmware is about to start the application it switches to a more
constrained environment, the application mode.

The firmware and application uses a memory mapped input/output (MMIO)
for communication with the hardware. The memory map is constrained
when running in application mode, e.g. FW-RAM and UDS isn't readable,
and several MMIO addresses are either not readable or not writable for
the application.

See table in the [System
Description](system_description.md#memory-mapped-hardware-functions)
for details about access rules control in the memory system and MMIO.

The firmware (and optionally all software) on the TKey can communicate
to the host via the `UART_{RX,TX}_{STATUS,DATA}` registers, using the
framing protocol described in [Framing
Protocol](../framing_protocol/framing_protocol.md).

The firmware defines a protocol on top of this framing layer which is
used to bootstrap the application. All commands are initiated by the
host. All commands receive a reply. See [Firmware
protocol](#firmware-protocol) for specific details.

Applications define their own protocol used for communication with
their host part. They may or may not be based on the Framing Protocol.

## CPU

The CPU is a PicoRV32, a 32-bit RISC-V processor (arch: RV32IC\_Zmmul)
which runs the firmware and the application. The firmware and
application both run in RISC-V machine mode. All types are
little-endian.

## Constraints

- ROM: 6 kByte.
- RAM: 128 kByte.

## Firmware

The purpose of the firmware (FW) is to bootstrap itseld, set up the
application environment and then load and measure application to
generate the application compound device identifier (CD).

The TKey has 128 kilobyte RAM. The FW loads the application at the
start of RAM. The current C runtime (`crt0.S`) of apps in our [apps
repo](https://github.com/tillitis/tillitis-key1-apps) sets up the
application stack to start just below the end of RAM. This means that
a larger app comes at the expense of it having a smaller stack.

The FW binary is part of FPGA the bitstream as the initial values of
the Block RAMs used to construct the `FW_ROM`. The FW ROM start
address is located at `0x0000_0000` in the CPU memory map, which is
the CPU reset vector.

When reset is released, the CPU starts executing the FW. The FW begin
by clearing all CPU registers, and then sets up a stack for
itself. The FW then jumps to main().

Beginning at main(), the FW fills the RAM with pseudo random values,
thus whiping out any contents in the RAM. Finally the FW sets the RAM
access scrambling parameters to values read from the True Random
Number Generator (TRNG).

After these initalization steps, the FW wait for commands coming
in on `UART_RX`.

Typical use scenario:

  1. The host sends the `FW_CMD_LOAD_APP` command with the size of the
     device app and the optional 32 byte hash of the user-supplied
     secret (USS) as arguments and and gets a `FW_RSP_LOAD_APP`
     back. After using this it's not possible to restart the loading
     of an application.

  2. If the the host receive a sucessful response, it will send
     multiple `FW_CMD_LOAD_APP_DATA` commands, together containing the
     full application.

  3. On receiving `FW_CMD_LOAD_APP_DATA` commands the firmware places
     the data into `0x4000_0000` and upwards. The firmware replies
     with a `FW_RSP_LOAD_APP_DATA` response to the host for each
     received block except the last data block.

  4. When the final block of the application image is received with a
     `FW_CMD_LOAD_APP_DATA`, the firmware measure the application by
     computing a BLAKE2s digest over the entire application. Then
     firmware send back the `FW_RSP_LOAD_APP_DATA_READY` response
     containing the measurement.

  5. The Compound Device Identifier (CDI) is then computed by using
     the `UDS`, application digest, and the `USS`, and placed in
     `CDI`. (see [Compound Device Identifier
     computation](#compound-device-identifier-computation)) Then the
     start address of the device app, `0x4000_0000`, is written to
     `APP_ADDR` and the size to `APP_SIZE` to let the device
     application know where it is loaded and how large it is, if it
     wants to relocate in RAM.

  6. The firmware now clears the special `FW_RAM` where it keeps it
     stack. After this it does no more function calls and uses no more
     automatic variables.

  7. Firmware starts the application by first switching to application
     mode by writing to the `SWITCH_APP` register. In this mode the
     MMIO region is restricted, e.g. some registers are removed
     (`UDS`), and some are switched from read/write to read-only (see
     [memory
     map](system_description.md#memory-mapped-hardware-functions)).

     Then the firmware jumps to what's in `APP_ADDR` which starts
     the application.

     There is now no other means of getting back from application mode
     to firmware mode than resetting/power cycling the device.

### Developing firmware

Standing in `hw/application_fpga/` you can run `make firmware.elf` to
build just the firmware. You don't need all the FPGA development tools
mentioned in [Toolchain setup](../toolchain_setup.md).

You need clang with 32 RISC-V support `-march=rv32iczmmul` which comes
in clang 15. If you don't have version 15 you might get by with
`-march=rv32imc` but things will break if you ever cause it to emit
`div` instructions.

You also need `llvm-ar`, `llvm-objcopy`, `llvm-size` and `lld`.
Typically these are all available in packages called "clang", "llvm",
"lld" or similar.

If your available `objcopy` and `size` commands is anything other than
the default `llvm-objcopy` and `llvm-size` define `OBJCOPY` and `SIZE`
to whatever they're called on your system before calling `make
firmware.elf`.

If you want to use our emulator, clone the `tk1` branch of [our
version of qemu](https://github.com/tillitis/qemu) and build:

```
$ git clone -b tk1 https://github.com/tillitis/qemu
$ mkdir qemu/build
$ cd qemu/build
$ ../configure --target-list=riscv32-softmmu --disable-werror
$ make -j $(nproc)
```

(Built with warnings-as-errors disabled, see [this
issue](https://github.com/tillitis/qemu/issues/3).)

Run it like this:

```
$ /path/to/qemu/build/qemu-system-riscv32 -nographic -M tk1,fifo=chrid -bios firmware.elf \
  -chardev pty,id=chrid
```

This attaches the FIFO to a tty, something like `/dev/pts/16` which
you can use with host software to talk to the firmware.

To quit QEMU you can use: `Ctrl-a x` (see `Ctrl-a ?` for other commands).

Debugging? Use the HTIF console by removing `-DNOCONSOLE` from the
`CFLAGS` and using the helper functions in `lib.c` like `htif_puts()`
`htif_putinthex()` `htif_hexdump()` and friends for printf-like
debugging.

You can add `-d guest_errors` to the qemu commandline to make QEMU
send errors from the TK1 machine to stderr, typically things like
memory writes outside of mapped regions.

You can also use the qemu monitor for debugging, e.g. `info
registers`, or run qemu with `-d in_asm` or `-d trace:riscv_trap`.

### Reset

The PicoRV32 starts executing at `0x0000_0000`. We allow no `.data` or
`.bss` sections. Our firmware starts at the `_start` symbol in
`start.S` which first clears all RAM for any remaining data from
previous applications, then initializes a stack starting at the top of
`FW_RAM` at `0xd000_0800` and downwards and then calls `main`.

When the initialization is finished, the firmware waits for incoming
commands from the host, by busy-polling the `UART_RX_{STATUS,DATA}`
registers. When a complete command is read, the firmware executes the
command.

### Firmware state machine

States:

- `initial` - At start.
- `loading` - Expect application data.
- `run` - Computes CDI and starts the application.
- `fail` - Stops waiting for commands, flashes LED forever.

Commands in state `initial`:

| *command*             | *next state* |
|-----------------------|--------------|
| `FW_CMD_NAME_VERSION` | unchanged    |
| `FW_CMD_GET_UDI`      | unchanged    |
| `FW_CMD_LOAD_APP`     | `loading`    |
|                       |              |

Commands in state `loading`:

| *command*              | *next state*                     |
|------------------------|----------------------------------|
| `FW_CMD_LOAD_APP_DATA` | unchanged or `run` on last chunk |

Commands in state `run`: None.

Commands in state `fail`: None.

See [Firmware protocol](#firmware-protocol) for the definition of the
specific commands and their responses.

### User-supplied Secret (USS)

USS is a 32 bytes long secret provided by the user. Typically a host
program gets a secret from the user and then does a key derivation
function of some sort, for instance a BLAKE2s, to get 32 bytes which
it sends to the firmware to be part of the CDI computation.

### Compound Device Identifier computation

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
We keep the entire firmware stack in `FW_RAM` and clear it just before
switching to app mode just in case.

We sleep for a random number of cycles before reading out the UDS,
call `blake2s_update()` with it and then immediately call
`blake2s_update()` again with the program digest, destroying the UDS
stored in the internal context buffer. UDS should now not be in
`FW_RAM` anymore. We can read UDS only once per power cycle so UDS
should now not be available to firmware at all.

Then we continue with the CDI computation by updating with an optional
USS and then finalizing the hash, storing the resulting digest in
`CDI`.

### Firmware protocol

The firmware commands and responses are built on top of the [Framing
Protocol](../framing_protocol/framing_protocol.md).

The commands look like this:

| *name*           | *size (bytes)* | *comment*                                |
|------------------|----------------|------------------------------------------|
| Header           | 1              | Framing protocol header including length |
|                  |                | of the rest of the frame                 |
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
consecutively over the complete raw binary. (128 == largest frame
length minus the command byte).

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
|        |                | Product ID (6 bit), Product Revision (6 bit)        |
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
  u8 RSP[1 + 128]

  RSP[0].len = 128  // command frame format
  RSP[1]     = 0x07 // FW_RSP_LOAD_APP_DATA_READY

  RSP[2]     = STATUS

  RSP[3..35]   = app digest
  RSP[36..]    = 0
```

### Firmware services

The firmware exposes a BLAKE2s function through a function pointer
located in MMIO `BLAKE2S` (see [memory
map](system_description.md#memory-mapped-hardware-functions)) with the
with function signature:

```c
int blake2s(void *out, unsigned long outlen, const void *key,
	    unsigned long keylen, const void *in, unsigned long inlen,
	    blake2s_ctx *ctx);

```

where `blake2s_ctx` is:

```c
typedef struct {
	uint8_t b[64]; // input buffer
	uint32_t h[8]; // chained state
	uint32_t t[2]; // total number of bytes
	size_t c;      // pointer for b[]
	size_t outlen; // digest size
} blake2s_ctx;
```

The `libcommon` library in
[tillitis-key1-apps](https://github.com/tillitis/tillitis-key1-apps/)
has a wrapper for using this function called `blake2s()`.

## Applications

See [our apps repo](https://github.com/tillitis/tillitis-key1-apps)
for examples of client and TKey apps as well as libraries for writing
both.
