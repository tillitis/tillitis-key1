# Tillitis Key 1

## Introduction

Tillitis Key 1 is a new kind of USB security token. All of its
software, FPGA logic, schematics, and PCB layout are open source, as
all security software and hardware should be. This in itself makes it
different, as other security tokens utilize closed source hardware for
its security-critical operations.

What makes the Tillitis Key 1 security token unique is that it doesn’t
verify applications, it measures them, before running them on its open
hardware security processor.

Each security token contains a Unique Device Secret (UDS),
which together with an application measurement, and an optional
user-provided seed, is used to derive key material unique to each
application. This allows users to build and load their own apps, while
ensuring that each app loaded will have its own cryptographic
identity. The design is similar to TCG DICE. The Tillitis Key 1
platform allows for applications up to 64 KB.

The first implementation is the Tillitis Key 1:
![The Tillitis Key 1 PCB](doc/images/mta1-usb-v1.jpg)


## Documentation

* [System Description](doc/system_description/system_description.md)
* [Threat Model](doc/threat_model/threat_model.md)
* [Framing Protocol](doc/framing_protocol/framing_protocol.md)
* [Boards](hw/boards/README.md)
* [Firmware](hw/application_fpga/fw/mta1_mkdf/README.md)
* [Toolchain setup](doc/toolchain_setup.md)
* [Quickstart](doc/quickstart.md) to program the Tillitis Key1

## About this repository

This repository contains hardware, software and utilities written as
part of the Tillitis Key 1 project. It is structured as monolithic
repository, or "monorepo", where all components live in one
repository.

The repository follows the [OpenTitan
layout](https://docs.opentitan.org/doc/ug/directory_structure/).

## Licensing

See [LICENSES](./LICENSES/README.md) for more information about
the projects' licenses.
