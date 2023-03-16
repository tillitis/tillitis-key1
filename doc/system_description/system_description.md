# System Description

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
- Implementors of the TKkey hardware, firmware and SDKs
- Developers of secure applications for the TKey
- Technically skilled third parties that wants to understand the
  TKey


## Introduction
The TKey is a USB-connected, RISC-V based application platform. The
purpose of the TKey is to provide a secure environment
for applications that provides some security functionality needed by the
device user. Some examples of such security functionality are:

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
by the derivation, thereby verifying the intergrity od the application.

### Execution monitor
The purpose of the The Tillitis TKey execution monitor is to ensure that execution of instructions does not happen from memory areas containing application data och the stack.

The monitor continuously observes the address from which the CPU wants to fetch the next instruction. If that address falls within a defined address range from which execution is not allowed, the monitor will force the CPU to read an illegal instruction. This will cause the CPU to enter its trap state. There is no way out of this state, and the user must perform a power cycle of the TKey device.

Currently the following rules are implemented by the execution monitor (future releases may add more rules):

- Execution from the firmware RAM (fw_ram) is always blocked by the monitor
- Applications can define the area within RAM from which execution should be blocked

The application can define its no execution area to the ADDR_CPU_MON_FIRST and ADDR_CPU_MON_LAST registers in the tk1 core. When the registers have been set the application can enable the monitor for the area by writing to the ADDR_CPU_MON_CTRL register. Note that once the monitor has been enabled it can't be disabled and the addresses defining the area can't be changed.


### Illegal instruction monitor
Execution of illegal instructions will cause the CPU to enter its trap state from which it can't exit. The hardware in the TKey will monitor the CPU state. If the CPU enters the trap state, the hardware will start flashing the RED led, signalling that the TKey is stuck in an error state.


### RAM memory protection
The TKey hardware includes a simple form of RAM memory protection. The purpose of the RAM memory protection is to make it somewhat harder and more time consuming to extract application assets by dumping the RAM contents from a TKey device. The memory protection is not based on encryption and should not be confused with real encryption. But the protection is randomised between power cycles. The randomisation should make it infeasible to improve asset extraction by observing multiple memory dumps from the same TKey device. The attack should also not directly scale to multiple TKey devices.

The memory protection is based on two separate mechanisms:
1. Address Space Layout Randomisation (ASLR)
2. Adress dependent data scrambling

The ASLR is implemented by XORing the CPU address with the contents of the ADDR_RAM_ASLR register in the tk1 core. The result is used as the RAM address

The data scrambling is implemented by XORing the data written to the RAM with the contents of the ADDR_RAM_SCRAMBLE register in the tk1 core as well as XORing with the CPU address. This means that the same data written to two different addresses will be scrambled differently. The same pair or XOR operations is also performed on the data read out from the RAM.

The memory protection is setup by the firmware. Access to the memory protection controls is disabled for applications. During boot the firmware perform the following steps to setup the memory protection:

1. Write a random 32-bit value from the TRNG into the ADDR_RAM_ASLR register.
2. Write a random 32-bit value from the TRNG into the ADDR_RAM_SCRAMBLE register.
3. Get a random 32-bit value from the TRNG to use as data value.
4. Get a random 32-bit value from the TRNG to use as accumulator value.
5. Fill the RAM with sequence of value by writing to all RAM addresses in sequence. For each address add the accumulator value to the current data value.
6. Write a new random 32-bit value from the TRNG into the ADDR_RAM_ASLR register.
7. Write a new random 32-bit value from the TRNG into the ADDR_RAM_SCRAMBLE register.
8. Receive the application sent from the client and write it in sequence into RAM.

This means that the RAM is pre-filled with somewhat randomised data. The application is then written into RAM using different ASLR and data scrambling than what was used to pre-fill the memory. This should make it harder to identify where in RAM the application was written, and how the application was scrambled.

Future TKey devices may implement a more secure ASLR mechanism, and use real encryption (for example PRINCE) for memory content protection. From the application point of view such a change will transparent.


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

- CDI - Compound Device Identity. Dervied by the FW when an application
  is loaded using the UDS and the application binary. Used by the
  application to derive secrets, keys as needed. The CDI should never
  be exposed outside of the application_fpga


Additionally the following asset could be provided from the host:

- USS - User Supplied Secret. May possibly be replaced many times.
  Supplied from the host to the device. Should not be revealed to a
  third party.


## Subsystems and Components
The TKey as a project, system and secure application platform
consists of a number of subsystems and components, modules, support
libraries etc. Roughly these can be divided into:

- TKey boards. PCB designs including schematics, Bill of Material (BOM)
  and layout, as needed for development, production and and general usage
  of the TKey devices

- TKey programmer. SW, PCB designs including schematics, Bill of
  Material (BOM) and layout, as needed for development, production
  and and provisioning, programming general usage

- USB to UART controller. FW for the MCU implementing the USB host
  interface on the TKey

- application_fpga. FPGA design with cores including CPU and memory that
  implements the secure application platform

- application_fpga FW. The base software running on the CPU as needed to
  boot, load applications, measure applications, dderive base secret etc

- One or more applications loaded onto the application_fpga to provide
  some functionality to the user of the host

- host side application loader. Software that talks to the FW in the
  application_fpga to load a secure application

- host side boot, management. Support software to boot, authenticate
  the TKey device connected to a host

- host side secure application. Software that communicates with the
  secure application running in the application_fpga as needed to solve
  a security objective

- application_fpga FW SDK. Tools, libraries, documentation and examples
  to support development of the application_fpga firmware

- secure application SDK. Tools, libraries, documentation and examples
  to support development of the secure applications to be loaded onto
  the application_fpga

- host side secure application SDK. Tools, libraries, documentation and
  examples to support development of the host applications


## References
More detailed information about the software running on the device
(referred to firmware, SDK, and secure application), can be found in
the [software document](software.md).
