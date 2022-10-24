# System Description

## Purpose and Revision
The purpose of this document is to provide a description of the
Tillitis Key 1 (TK1). What it is, what is supposed to be used for, by whom,
where and possible use cases. The document also provides a functional level
description of features and components of the mta1_mkdf.

Finally, the document acts as a requirement description. For the
requirements, the document follows
[RFC2119](https://datatracker.ietf.org/doc/html/rfc2119) to indicate
requirement levels.

The described functionality and requirements applies
to version one (v1) of the TK1.

The intended users of this document are:
- Implementors of the TK1 hardware, firmware and SDKs
- Developers of secure applications for the TK1
- Technically skilled third parties that wants to understand the
  TK1


## Introduction
The TK1 is a USB-connected, RISC-V based application platform. The
purpose of the TK1 is to provide a secure application environment
for applications that provides some security functionality needed by the
user. Some examples of such security functionality are:

- TOTP token generators
- Signing oracles
- SSH login dongles


### Measured Based Security
The key, unique feature of the TK1 is that it measures the secure
application when the application is being loaded onto the device. The
measurement (a hash digest), combined with a Unique Device Secret
(UDS) is used to derive secrets for the application.

The consequence of this is that if the application is altered, the keys
derived will also change. Conversely, if the keys derived are the same as
last time the application was loaded onto the same device, the
application can be trusted not to have been altered.

Note that since the UDS is per-device unique, the same application
loaded onto another TK1 device will cause a different set of keys
to be derived. This ties keys to a specific device.

The derivation can also be combined with a User Supplied Secret
(USS). This means that keys derived are both based on something the user
has - the specific device, and something the user knows (the USS). And
the keys are protected and can be trusted because of the measurement
being used in the derivation.


### Assets
The TK1 store and use the following assets internally:

- UDS - Unique Device Secret. Provisioned and stored during
  device manufacturing. Never to be replaced during the life time of
  a given device. Used to derive application secrets. Must never leave
  the device. Mullvad must NOT store a copy of the UDS.

- UDI - Unique Device ID. Provisioned and stored during
  device manufacturing. Never to be replaced or altered during the life
  time of a given device. May be copied, extracted, read from the device.

- UDA - Unique Device Authentication Secret. Provisioned and stored during
  device manufacturing. Never to be replaced during the life time of
  a given device. Used to authenticate a specific device. Must never
  leave the device. Mullvad MUST have a copy of the UDA.


Additionally the following asset could be provided from the host:

- USS - User Supplied Secret. Provisioned by the application. May
  possibly be replaced many times. Supplied from the host to the
  device. Should not be revealed to a third party.


### Subsystems and Components
The TK1 as a project, system and secure application platform
consists of a number of subsystems and components, modules, support
libraries etc. Roughly these can be divided into:

- TK1 boards. PCB designs for development and general usage

- USB to UART controller

- application_fpga. FPGA design with cores including CPU and memory

- application_fpga FW. The base software running on the CPU to boot, load
  applications, derive keys etc

- application_fpga secure application. One or more applications loaded
  into the application_fpga to provide some functionality to the user of
  the host

- host side application loader. Software that talks to the FW in the
  application_fpga to load a secure application

- host side boot, management. Support software to boot, authenticate the
  mta1_mkdf board connected to a host

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


## Application FPGA Hardware Functionality
The Application FPGA hardware should provide the following:

1. Fixed information
   - Unique Device ID (UDI)
      - 64 bits
      - Readable via API before application start
      - Generated and stored by Mullvad

   - Unique Device Authentication key (UDA)
      - At least 128 bits number
      - Readable by FW before application start
      - Generated and stored by Mullvad

    - Unique Device Secret (UDS)
      - 256 bits
      - Readable by HW before application start
      - Generated but NOT stored by Mullvad

    - NAME
      - 64 bits. ASCII string. "mta1_mkdf"
      - Readable via API before application start
      - Set by Mullvad as part of FPGA design

    - VERSION: version
      - 32 bits. 32 bit data, for example 1
      - Readable via API before application start
      - Set by Mullvad as part of FPGA design

2. Communication
    - Rx-FIFO with status (data_available)
      - 8 bit data in UART_RX_DATA address
      - Byte received status bit in UART_RX_STATUS address
      - Readable by FW and application

    - Tx-FIFO with capacity (fifo_ready)
      - 8 bit data in UART_RX_DATA address
      - Ready to store byte status bit in UART_TX_STATUS address
      - Status readable by FW and application
      - Data writable by FW and application

3. I/O
   - LED (RGB)
      - Status and control in LED address
      - Readable and writable by FW and application

4. Counter
    - One general purpose counter
      - Prescaler (for counting cycles and seconds)
	  - Start value, alternatively reset
	  - Saturating max, alternatively stop at zero
      - Readable and writable by FW and application

5. TRNG
    - ROSC based internal entropy source
      - Von Neumann decorrelation
      - Simple self-testing ability
      - 32 bit data
      - Status (data_ready, error)
      - Readable by FW and application

6. Introspection
	- Address och size of loaded application
      - Readable by FW and application


## Application FPGA Firmware Functionality
The firmware in the application should provide the following
functionality:

- Read access to fixed values:
  - application_fpga name and version strings
  - Unique Device ID (UDI)

- Read and write to test register used for debugging

- Respond to challenge/response based device authentication commands

- Receive and store a 32 byte User Supplied Secret (USS)

- Receive, store and measure a secure application

- Derive Compound Device Identifier (CDI) given measurement, UDS and USS

- Provide hashing using Blake2s

- Start a loaded application. This includes locking down access to UDS,
  UDA etc

## References

More detailed information about the software running on the device
(referred to firmware, SDK, and secure application), can be found in
the [software document](software.md).

## Work in Progress
TODOs and random notes, questions to be worked into the document. Or be
scratched.

- Possible technical solution - Could we reuse the button as a physical
    presence detect when injecting a bitstream from the interface_fpga
    to the application_fpga? Alternative have a strap, which would
    require opening the stick. The stick is the sold with nail polish to
    reseal it.

- Ideas - mitigating mechanisms for host bases threats
  - Push button
  - User Supplied Secret (USS)


- Open Questions to be investigated, handled
  - Terminology - naming things
  - How to create trust in the SDKs
