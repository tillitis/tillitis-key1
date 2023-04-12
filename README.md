
[![ci](https://github.com/tillitis/tillitis-key1/actions/workflows/ci.yaml/badge.svg?branch=main&event=push)](https://github.com/tillitis/tillitis-key1/actions/workflows/ci.yaml)

# Tillitis TKey

## Introduction

The Tillitis TKey is a new kind of USB security token. What makes the
TKey unique is that it allows a user to load and run applications on
the device, while still providing security. This allow for open-ended,
flexible usage. Given the right application, the TKey can support use
cases such as SSH login, Ed25519 signing, Root of Trust, FIDO2, TOTP,
Passkey, and more.

During the load operation, the device measures the application
(calculates a cryptographic hash digest over it) before running
it on the open hardware security processor. This measurement
is similar to [TCG DICE](https://trustedcomputinggroup.org/work-groups/dice-architectures/).

Each TKey device contains a Unique Device Secret (UDS), which
together with the application measurement, and an optional
User-Supplied Secret (USS), is used to derive key material unique to each
application. This guarantees that if the integrity of the application
loaded onto the device has been tampered with, the correct keys
needed for an authentication will not be generated.

Key derivation with a User-Supplied Secret allows users to build and
load their own apps, while ensuring that each app loaded will have
its own cryptographic identity, and can also be used for authentication
towards different services.

The TKey platform is based around a 32-bit RISC-V processor and has
128 KB of RAM. Firmware can load and start an app that is as large as
RAM.

All of the TKey software, firmware, FPGA Verilog source code, schematics
and PCB design files are open source. Like all trustworthy security software
and hardware should be. This in itself makes it different, as other
security tokens utilize at least some closed source hardware for its
security-critical operations.

![Tillitis Key 1 PCB, first implementation](doc/images/mta1-usb-v1.jpg)
*The TK1 PCB, the first implementation of the TKey.*


## Documentation

### Getting started

* [tillitis-key1-apps repository](https://github.com/tillitis/tillitis-key1-apps),
  with device apps and client apps for using the TKey
* [Quickstart](doc/quickstart.md) to initial programming of the TKey
  (only required for the DevKit)
* [Toolchain setup](doc/toolchain_setup.md)
* [Release Notes](doc/release_notes.md)

### In-depth technical information

* [System Description](doc/system_description/system_description.md)
* [Threat Model](doc/threat_model/threat_model.md)
* [Framing Protocol](doc/framing_protocol/framing_protocol.md)
* [Boards](doc/system_description/boards.md)
* [FPGA](doc/system_description/fpga.md)
* [Software](doc/system_description/software.md)
* [QEMU](https://github.com/tillitis/qemu/tree/tk1) (branch `tk1` in
  separate repository)

Note that development is ongoing. For example, changes might be made
to the measuring and derivation of key material, causing the
public/private keys of a signer app to change. To avoid unexpected
changes, please use a tagged release. Read the [Release
Notes](doc/release_notes.md) to keep up to date with changes and new
releases.

## About this repository

This repository contains hardware, software and utilities written as
part of the TKey. It is structured as monolithic repository, or
"monorepo", where all components live in one repository.

## Licensing

See [LICENSES](./LICENSES/README.md) for more information about
the projects' licenses.

All contributors must adhere to the [Developer Certificate of Origin](dco.md).
