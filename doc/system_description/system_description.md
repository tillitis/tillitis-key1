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
purpose of the TKey is to provide a secure environment for TKey device
apps that provides some security functionality to the client as needed
by the use case and device user. Some examples of such security
functionality are:

- TOTP token generators
- Signing oracles
- SSH login dongles


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


### Assets
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


### Subsystems and Components
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

- application_fpga. FPGA design with cores including CPU, TRNG, UART
  FW ROM and RAM that together form the the secure application
  platform

- application_fpga FW. The base software running on the CPU as needed
  to boot the device, load applications, measure applications, derive
  the CDI base secret etc

- One or more TKey device apps loaded onto the application_fpga to
  provide some functionality to the user of the host

- host side application loader. Software that talks to the FW in the
  application_fpga to load a secure application

- host side boot, management. Support software to boot, authenticate
  the TKey device connected to a host

- host side secure application. Software that communicates with the
  secure application running in the application_fpga as needed to
  solve a security objective

- application_fpga FW SDK. Tools, libraries, documentation and
  examples to support development of the application_fpga firmware

- secure application SDK. Tools, libraries, documentation and examples
  to support development of the secure applications to be loaded onto
  the application_fpga

- host side secure application SDK. Tools, libraries, documentation and
  examples to support development of the host applications


## References
More detailed information about the software running on the device
(referred to firmware, SDK, and secure application), can be found in
the [software document](software.md).
