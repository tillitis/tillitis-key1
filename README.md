[![ci](https://github.com/tillitis/tillitis-key1/actions/workflows/ci.yaml/badge.svg?branch=main&event=push)](https://github.com/tillitis/tillitis-key1/actions/workflows/ci.yaml)

# Tillitis TKey

![TK1 PCB](doc/images/tkey-open-lid.png) *The TK1 PCB, also known as
TKey.*

## Introduction

The Tillitis TKey is an open source, open hardware FPGA-based USB
security token using
[DICE-like](https://trustedcomputinggroup.org/work-groups/dice-architectures/)
unconditional measured boot that can run generic applications while
still guaranteeing the security of its cryptographic assets.

[TKey Threat Model](doc/threat_model/threat_model.md).

With the right application, the TKey can be used for:

- authentication,
- cryptographic signing,
- encryption,
- root of trust,
- and more: it's a general computer!

If you want to know more about Tillitis and the TKey, visit:

- Main web: https://tillitis.se/
- Shop: https://shop.tillitis.se/
- Developer Handbook: https://dev.tillitis.se/
- Officially supported apps: https://tillitis.se/download/
- Other known apps: https://dev.tillitis.se/projects/

All of the TKey software, firmware, FPGA Verilog code, schematics and
PCB design files are open source, just like all trustworthy security
software and hardware should be.

## Licensing

See [LICENSES](./LICENSES/README.md) for more information about
the projects' licenses.

## Repositories

This repository contains the FPGA design, the source of the
firmware/bootloader, and the source of the USB controller firmware.

Specific documentation regarding implementation is kept close to the
code/design in README files, typically in the same directory.

Note that development is ongoing. To avoid unexpected changes of
derived key material, please use a tagged release. Read the [Release
Notes](doc/release_notes.md) to keep up to date with changes and new
releases.

The TKey PCB [KiCad](https://www.kicad.org/) design files are kept in
a separate repository:

https://github.com/tillitis/tk1-pcba

The TP1 (TKey programmer 1) PCB design files and the firmware sources
are kept in:

https://github.com/tillitis/tp1

Note that the TP1 is only used for provisioning the FPGA bitstream
into flash or the FPGA configuration memory. It's not necessary if you
just want to develop apps for the TKey.

## Measured boot

The key behind guaranteeing security even as a general computer is the
unconditional measured boot. This means that we have a small,
unchangeable, trusted firmware in ROM that creates a unique identity
before starting the application. This identity is used as a seed for
all later cryptographic keys.

We call this identity the Compound Device Identity (CDI). The CDI is a
cryptographic mix of:

1. the Unique Device Secret (UDS), a hardware secret, unique per
   device, something the user *has*,
2. the hash digest of the TKey device application that has been
  loaded, the *integrity* of the application, and,
3. an optional User Supplied Secret (USS), something the user *knows*.

CDI is computed using the BLAKE2s hash function:

CDI = BLAKE2s(UDS, BLAKE2s(application loaded in RAM), USS)

When firmware is about to start the device application it changes the
TKey to a less permissive hardware mode, application mode. In
application mode the UDS and the User Supplied Secret are no longer
available, but the device application can use the CDI as a seed to
deterministically generate any cryptographic keys it needs.

- If the wrong application has been loaded, or the original
  application has been tampered with, the generated keys will be
  different.
- If the USS is not the same, the generated keys will be different.
- If the same USS and device application is used on a different TKey,
  the generated keys will be different.

The TKey unconditional measured boot is inspired by, but not exactly
the same as part of [TCG
DICE](https://trustedcomputinggroup.org/work-groups/dice-architectures/).
