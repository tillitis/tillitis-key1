# System Description

**NOTE:** Documentation migrated to dev.tillitis.se, this is kept for
history. This is likely to be outdated.

## Purpose and Revision

The purpose of this document is to provide a description of the
Tillitis TKey. What it is, what is supposed to be used for, by whom,
where and possible use cases. The document also provides a functional
level description of features and components of the TKey.

Finally, the document acts as a requirement description. For the
requirements, the document follows
[RFC2119](https://datatracker.ietf.org/doc/html/rfc2119) to indicate
requirement levels.

The described functionality and requirements applies to version 1 of
the TKey (TK1)

The intended users of this document are:
- Implementors of the TKey hardware, firmware and SDKs
- Developers of secure applications for the TKey
- Technically skilled third parties that wants to understand the
  TKey


## Introduction
The TKey is a USB-connected, RISC-V based application platform. The
purpose of the TKey is to provide a secure environment for TKey device
apps that provides some security functionality to the client as needed
by the use case and device user. Some examples of such security
functionality are:

- TOTP token generators
- Signing oracles
- SSH login dongles


## TKey Security Features

### Measured Based Security
The key, unique feature of the TKey is that it measures the secure
application when the application is being loaded onto the device. The
measurement (a hash digest), combined with a Unique Device Secret
(UDS) is used to derive a base secret for the application.

The consequence of this is that if the application is altered,
the base secret derived will also change. Conversely, if the keys
derived from the base secret are the same as the last time the
application was loaded onto the same device, the application can
be trusted not to have been altered.

Note that since the UDS is per-device unique, the same application
loaded onto another TKey device will derive a different set of keys.
This ties keys to a specific device.

The derivation can also be combined with a User Supplied Secret
(USS). This means that keys derived are both based on something the user
has - the specific device, and something the user knows (the USS). And
the derived can be trusted because of the measurement being used
by the derivation, thereby verifying the integrity of the application.

### Execution monitor

The purpose of the The Tillitis TKey execution monitor is to ensure
that execution of instructions does not happen from memory areas
containing application data och the stack.

The monitor continuously observes the address from which the CPU wants
to fetch the next instruction. If that address falls within a defined
address range from which execution is not allowed, the monitor will
force the CPU to read an illegal instruction. This will cause the CPU
to enter its trap state. There is no way out of this state, and the
user must perform a power cycle of the TKey device.

Currently the following rules are implemented by the execution monitor
(future releases may add more rules):

- Execution from the firmware RAM (fw\_ram) is always blocked by the
  monitor
- Applications can define the area within RAM from which execution
  should be blocked

The application can define its no execution area to the
ADDR\_CPU\_MON\_FIRST and ADDR\_CPU\_MON\_LAST registers in the tk1 core.
When the registers have been set the application can enable the
monitor for the area by writing to the ADDR\_CPU\_MON\_CTRL register.
Note that once the monitor has been enabled it can't be disabled and
the addresses defining the area can't be changed.

### Illegal instruction monitor

Execution of illegal instructions will cause the CPU to enter its trap
state from which it can't exit. The hardware in the TKey will monitor
the CPU state. If the CPU enters the trap state, the hardware will
start flashing the RED led, signalling that the TKey is stuck in an
error state.

### RAM memory protection

The TKey hardware includes a simple form of RAM memory protection. The
purpose of the RAM memory protection is to make it somewhat harder and
more time consuming to extract application assets by dumping the RAM
contents from a TKey device. The memory protection is not based on
encryption and should not be confused with real encryption. The
protection is randomised between power cycles. The randomisation
should make it infeasible to improve asset extraction by observing
multiple memory dumps from the same TKey device. The attack should
also not directly scale to multiple TKey devices.

The memory protection is based on two separate mechanisms:

1. Address randomisation
2. Address dependent data randomization 

The address randomization is implemented by XORing the CPU address
with the contents of the ADDR\_RAM\_ADDR\_RAND register in the tk1
core. The result is used as the RAM address

The data randomization is implemented by XORing the data written to the
RAM with the contents of the ADDR\_RAM\_DATA\_RAND register in the tk1
core as well as XORing with the CPU address. This means that the same
data written to two different addresses will be scrambled differently.
The same pair or XOR operations is also performed on the data read out
from the RAM.

The memory protection is setup by the firmware. Access to the memory
protection controls is disabled for applications. Before the memory
protecetion is enabled, the RAM is filled with randomised data using
Xorwow. So during boot the firmware perform the following steps to
setup the memory protection:

1. Get a random 32-bit value from the TRNG to use as data state for
   Xorwow.
2. Get a random 32-bit value from the TRNG to use as accumulator
   for Xorwow.
3. Fill RAM with a random sequence of values by writing to all RAM
   addresses. For each address use Xorwow to generate a new state,
   using the accumulator.
4. Write a random 32-bit value from the TRNG into the
   ADDR\_RAM\_ADDR\_RAND register.
5. Write a random 32-bit value from the TRNG into the
   ADDR\_RAM\_DATA\_RAND register.
6. Receive the application sent from the client and write it in
   sequence into RAM.

Future TKey devices may implement a more secure ASLR mechanism, and
use real encryption (for example PRINCE) for memory content
protection. From the application point of view such a change will be
transparent.

## Assets

The TKey store and use the following assets internally:

- UDS - Unique Device Secret. 256 bits. Provisioned and stored during
  device manufacturing. Never to be replaced during the life time of
  a given device. Used to derive application secrets. Must never leave
  the device. Tillitis will NOT store a copy of the UDS. Can be read
  by firmware once between power cycling

- UDI - Unique Device ID. 64 bits. Provisioned and stored during
  device manufacturing. Only accessible by FW. Never to be replaced or
  altered during the life time of a given device. May be copied,
  extracted, read from the device.

- CDI - Compound Device Identity. Computed by the FW when an
  application is loaded using the UDS and the application binary. Used
  by the application to derive secrets, keys as needed. The CDI should
  never be exposed.

Additionally the following asset could be provided from the host:

- USS - User Supplied Secret. May possibly be replaced many times.
  Supplied from the host to the device. Should not be revealed to a
  third party.

## Memory

Addressing:

```
31st bit                              0th bit
v                                     v
0000 0000 0000 0000 0000 0000 0000 0000

- Bits [31 .. 30] (2 bits): Top level prefix (described below)
- Bits [29 .. 24] (6 bits): Core select. We want to support at least 16 cores
- Bits [23 ..  0] (24 bits): Memory/in-core address.
```

Assigned top level prefixes:

| *name*   | *prefix* | *address length*                     |
|----------|----------|--------------------------------------|
| ROM      | 0b00     | 30 bit address                       |
| RAM      | 0b01     | 30 bit address                       |
| reserved | 0b10     |                                      |
| MMIO     | 0b11     | 6 bits for core select, 24 bits rest |

| *memory* | *first byte* | *last byte*                  |
|----------|--------------|------------------------------|
| ROM      | 0x0000\_0000 | 0x0000\_17ff                 |
| RAM      | 0x4000\_0000 | 0x4001\_ffff                 |
| MMIO     | 0xc000\_0000 | 0xffff\_ffff (last possible) |

### Memory mapped hardware functions

Hardware functions, assets, and input/output are memory mapped (MMIO)
starting at base address `0xc000_0000`. For specific offsets/bitmasks,
see the file [tk1_mem.h](../../hw/application_fpga/fw/tk1_mem.h) (in
this repo).

Assigned core prefixes:

| *name* | *address prefix* |
|--------|------------------|
| TRNG   | 0xc0             |
| TIMER  | 0xc1             |
| UDS    | 0xc2             |
| UART   | 0xc3             |
| TOUCH  | 0xc4             |
| FW_RAM | 0xd0             |
| TK1    | 0xff             |

*Nota bene*: MMIO accesses should be 32 bit wide, e.g use `lw` and
`sw`. Exceptions are `UDS`, `FW_RAM` and `QEMU_DEBUG`.

| *name*            | *fw*  | *app*     | *size* | *type*   | *content* | *description*                                                           |
|-------------------|-------|-----------|--------|----------|-----------|-------------------------------------------------------------------------|
| `TRNG_STATUS`     | r     | r         |        |          |           | TRNG_STATUS_READY_BIT is 1 when an entropy word is available.           |
| `TRNG_ENTROPY`    | r     | r         | 4B     | u32      |           | Entropy word. Reading a word will clear status.                         |
| `TIMER_CTRL`      | r/w   | r/w       |        |          |           | If TIMER_STATUS_RUNNING_BIT in TIMER_STATUS is 0, setting               |
|                   |       |           |        |          |           | TIMER_CTRL_START_BIT here starts the timer.                             |
|                   |       |           |        |          |           | If TIMER_STATUS_RUNNING_BIT in TIMER_STATUS is 1, setting               |
|                   |       |           |        |          |           | TIMER_CTRL_STOP_BIT here stops the timer.                               |
| `TIMER_STATUS`    | r     | r         |        |          |           | TIMER_STATUS_RUNNING_BIT is 1 when the timer is running.                |
| `TIMER_PRESCALER` | r/w   | r/w       | 4B     |          |           | Prescaler init value. Write blocked when running.                       |
| `TIMER_TIMER`     | r/w   | r/w       | 4B     |          |           | Timer init or current value while running. Write blocked when running.  |
| `UDS_FIRST`       | r[^3] | invisible | 4B     | u8[32]   |           | First word of Unique Device Secret key. Note: Read once per power up.   |
| `UDS_LAST`        |       | invisible |        |          |           | The last word of the UDS. Note: Read once per power up.                 |
| `UART_BITRATE`    | r/w   |           |        |          |           | TBD                                                                     |
| `UART_DATABITS`   | r/w   |           |        |          |           | TBD                                                                     |
| `UART_STOPBITS`   | r/w   |           |        |          |           | TBD                                                                     |
| `UART_RX_STATUS`  | r     | r         | 1B     | u8       |           | Non-zero when there is data to read                                     |
| `UART_RX_DATA`    | r     | r         | 1B     | u8       |           | Data to read. Only LSB contains data                                    |
| `UART_RX_BYTES`   | r     | r         | 4B     | u32      |           | Number of bytes received from the host and not yet read by SW, FW.      |
| `UART_TX_STATUS`  | r     | r         | 1B     | u8       |           | Non-zero when it's OK to write data to send.                            |
| `UART_TX_DATA`    | w     | w         | 1B     | u8       |           | Data to send. Only LSB contains data                                    |
| `TOUCH_STATUS`    | r/w   | r/w       |        |          |           | TOUCH_STATUS_EVENT_BIT is 1 when touched. After detecting a touch       |
|                   |       |           |        |          |           | event (reading a 1), write anything here to acknowledge it.             |
| `FW_RAM`          | r/w   | invisible | 2 kiB  | u8[2048] |           | Firmware-only RAM.                                                      |
| `UDI`             | r     | invisible | 8B     | u64      |           | Unique Device ID (UDI).                                                 |
| `QEMU_DEBUG`      | w     | w         |        | u8       |           | Debug console (only in QEMU)                                            |
| `NAME0`           | r     | r         | 4B     | char[4]  | "tk1 "    | ID of core/stick                                                        |
| `NAME1`           | r     | r         | 4B     | char[4]  | "mkdf"    | ID of core/stick                                                        |
| `VERSION`         | r     | r         | 4B     | u32      | 1         | Current version.                                                        |
| `SWITCH_APP`      | r/w   | r         | 1B     | u8       |           | Write anything here to trigger the switch to application mode. Reading  |
|                   |       |           |        |          |           | returns 0 if device is in firmware mode, 0xffffffff if in app mode.     |
| `LED`             | r/w   | r/w       | 1B     | u8       |           | Control of the color LEDs in RBG LED on the board.                      |
|                   |       |           |        |          |           | Bit 0 is Blue, bit 1 is Green, and bit 2 is Red LED.                    |
| `GPIO`            | r/w   | r/w       | 1B     | u8       |           | Bits 0 and 1 contain the input level of GPIO 1 and 2.                   |
|                   |       |           |        | u8       |           | Bits 3 and 4 store the output level of GPIO 3 and 4.                    |
| `APP_ADDR`        | r/w   | r         | 4B     | u32      |           | Firmware stores app load address here, so app can read its own location |
| `APP_SIZE`        | r/w   | r         | 4B     | u32      |           | Firmware stores app app size here, so app can read its own size         |
| `BLAKE2S`         | r/w   | r         | 4B     | u32      |           | Function pointer to a BLAKE2S function in the firmware                  |
| `CDI_FIRST`       | r/w   | r         | 32B    | u8[32]   |           | Compound Device Identifier (CDI). UDS+measurement...                    |
| `CDI_LAST`        |       | r         |        |          |           | Last word of CDI                                                        |
| `RAM_ASLR`        | w     | invisible | 4B     | u32      |           | Address Space Randomization seed value for the RAM                      |
| `RAM_SCRAMBLE`    | w     | invisible | 4B     | u32      |           | Data scrambling seed value for the RAM                                  |
| `CPU_MON_CTRL`    | w     | w         | 4B     | u32      |           | Bit 0 enables CPU execution monitor. Can't be unset. Lock adresses      |
| `CPU_MON_FIRST`   | w     | w         | 4B     | u32      |           | First address of the area monitored for execution attempts |
| `CPU_MON_LAST`    | w     | w         | 4B     | u32      |           | Last address of the area monitored for execution attempts |

[^3]: The UDS can only be read *once* per power-cycle.

## Subsystems and Components

The TKey as a project, system and secure application platform
consists of a number of subsystems and components, modules, support
libraries etc. Roughly these can be divided into:

- TKey boards. PCB designs including schematics, Bill of Material
  (BOM) and layout, as needed for development, production and and
  general usage of the TKey devices

- TKey programmer. SW, PCB designs including schematics, Bill of
  Material (BOM) and layout, as needed for development, production
  and and provisioning, programming general usage

- USB to UART controller. FW for the MCU implementing the USB host
  interface on the TKey

- application\_fpga. FPGA design with cores including CPU and memory that
  implements the secure application platform

- application\_fpga FW. The base software running on the CPU as needed to
  boot, load applications, measure applications, dderive base secret etc

- One or more applications loaded onto the application\_fpga to provide
  some functionality to the user of the host

- host side application loader. Software that talks to the FW in the
  application\_fpga to load a secure application

- host side boot, management. Support software to boot, authenticate
  the TKey device connected to a host

- host side secure application. Software that communicates with the
  secure application running in the application\_fpga as needed to solve
  a security objective

- application\_fpga FW SDK. Tools, libraries, documentation and examples
  to support development of the application\_fpga firmware

- secure application SDK. Tools, libraries, documentation and examples
  to support development of the secure applications to be loaded onto
  the application\_fpga

- host side secure application SDK. Tools, libraries, documentation and
  examples to support development of the host applications


## References
More detailed information about the firmware running on the device can
be found in the
[hw/application_fpga/fw/README.md](hw/application_fpga/fw/README.md).
