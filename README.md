
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


## Getting started
The official website is [tillitis.se](https://tillitis.se).

The Tkey can be purchased at
[shop.tillitis.se](https://shop.tillitis.se).

TKey software developer documentation is available in the [TKey
Developer Handbook](https://dev.tillitis.se).

Specific documentation regarding implementation is kept close to the
code/design in README files, typically in the same directory.

## Tkey Device Apps
Officially supported apps can be found at
[tillitis.se](https://tillitis.se/download/)

The source and other projects from us can be found here at our
[GitHub](https://github.com/tillitis).

Other known (but not all) projects can be found at
[dev.tillitis.se](https://dev.tillitis.se/projects/).

## PCB and programmer

The TKey PCB [KiCad](https://www.kicad.org/) design files are kept in
a separate repository:

https://github.com/tillitis/tk1-pcba

The TP1 (TKey programmer 1) PCB design files and firmware are kept in:

https://github.com/tillitis/tp1

## Other noteworthy links

* [Threat Model](doc/threat_model/threat_model.md)
* [Release Notes](doc/release_notes.md)
* [Quickstart for the DevKit](doc/quickstart.md). Initial programming
if you have the "old" DevKit.

Note that development is ongoing. To avoid unexpected changes of
derived key material, please use a tagged release. Read the [Release
Notes](doc/release_notes.md) to keep up to date with changes and new
releases.

## About this repository

This repository contains the FPGA design, firmware/bootloader, and the
USB controller firmware.

The PCB design files, device and client applications are kept in other
repositories. See:

https://github.com/tillitis

## Licensing

See [LICENSES](./LICENSES/README.md) for more information about
the projects' licenses.

All contributors must adhere to the [Developer Certificate of Origin](dco.md).
