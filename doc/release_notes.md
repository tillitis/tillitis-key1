# Release Notes

Descriptions of the tagged TKey releases.

## Upcoming release: Castor

Overview of changes since [TK1-24.03
Bellatrix](https://github.com/tillitis/tillitis-key1/releases/tag/TK1-24.03)
for the Castor milestone so far.

Main branch in the repository contains Castor design and specifically
the tag `TK1-Castor-alpha-1` reflects the upcoming release.

**Note well**: BREAKING CHANGE! Older device apps WILL NOT WORK.

The introduction of the USB Mode Protocol between the programs running
on the PicoRV32 CPU and the CH552 means that device apps that have not
been changed to use the protocol will not have any way to communicate
with the outside world.

### How to test TK1-Castor-alpha

To test this alpha release you will need:

- Tkey Unlocked
- TKey Programmer Board
- CH55x Reset Controller from [Blinkinlabs](https://shop-nl.blinkinlabs.com/products/ch55x-reset-controller)

Read
[here](https://github.com/tillitis/tillitis-key1/blob/main/README.md#building--flashing)
for instructions.

### General

- Split repo:

  - tk1, mta1-usb-dev, mta-usb-v1 and mta1-library moves to
    <https://github.com/tillitis/tk1-pcba>

  - tp1, mta1-usb-programmer, mta1-library and KiCad-RP Pico moves to
    <https://github.com/tillitis/tp1>

For full change log [see](https://github.com/tillitis/tillitis-key1/compare/TK1-23.03.2...coming-tag)

### FPGA

- Make Security Monitor memory access checks more complete.

- Add SPI main controller mainly to access the flash chip.

- Add system reset API. Device apps can reset the FPGA and restart
  the firmware.

- Increase clock frequence to 24 MHz.

- Increase UART baudrate to 500,000.

- Fix UART baudrate counter issues noticable at higher baudrates.

- Fix missing clock cycles in timer core.

- Remove the UART runtime configuration API.

- Several minor clean ups of design and testbench.

- Add hardware clear to send (CTS) signals for communication between
  UART and CH552.

- Increase size of ROM and FW\_RAM.

- Make ROM non-executable in app mode.

- Remove MMIO address for access to the firmware blake2s() function
  from apps.

- Automatically leave firmware mode when execution leaves ROM and
  remove the now unnecessary APP\_MODE\_CTRL register.

- Change UDS read protection: When execution leaves ROM the first
  time, UDS is hardware protected from reads. The already existing
  protection that UDS is protected after the first read is also still
  available.

- Introduce interrupt handler for hardware-based privilege raising and
  automatically privelege lowering for system calls.

### Firmware

- At startup, fill RAM with random data using the xorwow PRNG, seeded
  by TRNG.

- Add support for the new USB Mode Protocol to communicate with
  different USB endpoints in the USB controller.

- Support a filesystem on flash: There's space for two pre-loaded
  apps and four storage areas for device apps.

  A typical use is that app slot 0 will contain a loader app for
  verified boot and app slot 1 contains the app to be verified.

- Automatically start an app in flash app slot 0 after power cycle and
  when instructed to by reset intentions.

  The automatically started app is trusted by the firmware by
  including an app digest in the firmware ROM. This means we extend
  the user's trust in the firmware to the first app, but only if it's
  measured to the correct digest by the firmware. Anything else is a
  hard error which halts the CPU.

- Support chaining of apps through soft resets, including support for
  verifying that the next app is the expected one (exact measured
  digest the previous app expected), and leaving data for the next app
  to use.

- Add a system call mechanism and system calls. See [firmware's
  README](../hw/application_fpga/fw/README.md) for documentation, but
  its probably easier to use the the syscall wrappers in libsyscall in
  [tkey-libs](https://github.com/tillitis/tkey-libs) if you're writing
  in C.

- Harmonize with [tkey-libs](https://github.com/tillitis/tkey-libs).
  Import tkey-libs to this repo for convenience.

- Rewrite test firmware to work with the new leaving ROM-scenario.
  Introduce a separate `testapp` for the app mode parts.

### Device apps

Introduce some device apps mostly for testing.

- `reset_test`: Test the different types of soft reset.

- `testapp`: Tests in app mode that used to live in `testfw`.

- `testloadapp`: A simple loader app for management and verification
  of a second app.

- `defaultapp`: An app that immediately resets the TKey to load an app
  from the client, just like earlier releases.

### CH552 firmware

- Use the new CTS signals for communication over the UART.

- Add support for two HID endpoints (security token and our debug
  HID).

- Add support for CCID endpoint.

- Add a protocol to communicate with the different endpoints: CDC,
  CCID, FIDO, debug.

- Change USB frame sending from a software timer to instead be
  controlled by the USB Controller Protocol.

Note that to update the CH552 firmware you will need something like
the Blinkinlabs CH55x Reset Controller:

<https://shop-nl.blinkinlabs.com/products/ch55x-reset-controller>

<https://github.com/Blinkinlabs/ch55x_programmer>

### Tooling

- Add tools to parse and generate partition tables and flash images.

- Add tool to compute a print a BLAKE2s digest, optionally as C code.

### tkey-builder

- New versions of:
  - clang (18.1.3, part of ubuntu 24.04)
  - icestorm (commit 738af822905fdcf0466e9dd784b9ae4b0b34987f
)
  - yosys (0.45)
  - nextpnr (0.7) + extra patches for RNG seed handling and early exit
  - iverilog (v12)
  - verilator (v5.028)
  - verible (v0.0-3795)
  - cocotb (v1.9.1)

- Remove TKey Programmer (TP) toolchain:

  - gcc-arm-none-eabi: Used for the TKey Programmer firmware, now
    moved to it's own repo.
  - libnewlib-arm-none-eabi
  - libstdc++-arm-none-eabi-newlib
  - pico-sdk

  TP1 is now in <https://github.com/tillitis/tp1>

- Remove Go compiler support.

- Introduce buildtools.sh for building upstream tools for inclusion
  in the image.

### Docs

- All docs now in READMEs close to the design or code.

- Protocol docs moved to [the Developer
  Handbook](https://dev.tillitis.se/)
  [repo](https://github.com/tillitis/dev-tillitis)

## TK1-24.03

This is an official release of the "Bellatrix" version of the Tillitis'
TKey. This version is ready for general use.

Using OCI image `ghcr.io/tillitis/tkey-builder:4`, built from
`../contrib/Dockerfile`, and the generic
`../hw/application_fpga/data/uds.hex` and
`../hw/application_fpga/data/udi.hex`, a clean build should generate
the following digest:

```
321924aa3b26507f2a02325750e63c83b306c7831f8e2c87e8d198cecf8cc1c1  application_fpga.bin
```

### FPGA

- Security Monitor now prevents access to RAM outside of the physical
  memory. If it detects an access outside of the RAM address space, it
  will halt the CPU.
- CPU Monitor changes name to Security monitor, which CPU Monitor is a
  part of. Prepare for more functions in the future.
- Support incremental builds for the bitstream, when changing UDS/UDI
  between builds. Requires tkey-builder:3 or higher.
- Update Verilog linter to Verilog-2005 and fixed warnings.
- Complete testbenches and add 9 tests for the FPGA cores.

### Firmware

- Protect zeroisation against compiler optimisation by using
  secure_wipe(), fixing a memset() that was removed during
  compilation.
- Make memeq() function side channel silent.
- Change memory constants to defines instead of an enum, to be
  compatible with ISO C.
- Deprecate TK1_MMIO_TK1_RAM_ASLR and introduce
  TK1_MMIO_TK1_RAM_ADDR_RAND instead to distinguish from OS-level
  ASLR.
- Use pedantic warnings while building firmware and fixed warnings.
- Use clang-tidy in CI.
- Fix warnings from splint.

### TP1

- New plastic clip o and update of BOM.
- Build TP1 firmware in CI.

### CH552

- Fixed a bug where a byte of data could in some rare circumstances be
  dropped, causing a client app to hang.
- General clean-up of code, translated all comments to English.

### TK1

- New injection moulded plastic case

### tkey-builder

- Updated to version 3. Bumping Ubuntu to 23.10, Yosys to 0.36 and
  nextpnr to 0.6.
- Updated to version 4. Bumping pico-sdk to 1.5.1, adding clang-tidy
  and splint.

### Docs

- Fixing broken links, cleaning up docs and READMEs.
- Clarify warm boot attack mitigations and scope for Bellatrix in
  threat model.

For full change log [see](https://github.com/tillitis/tillitis-key1/compare/TK1-23.03.2...TK1-24.03)

## TK1-23.03.2

This is the official release of the "Bellatrix" version of the
Tillitis TKey device. This version is ready for general use.

This release only contains a hardware update to tk1. Capacitor C8 is
not populated. A PCB spring contact, U11, is insted placed on the
footprint of C8.

## TK1-23.03.1

This is the official release of the "Bellatrix" version of
the Tillitis TKey device. This version is ready for general
use.

Given the OCI image `ghcr.io/tillitis/tkey-builder:2` built from
`../contrib/Dockerfile` and the generic UDS.hex and UDI.hex, a clean
build should generate the following digest:

```
sha256sum application_fpga.bin
d2970828269b3ba7f09fb73b8592b08814dfe8c8087b00b0659feb516bb00f33  application_fpga.bin
```

This bug fix release contains the following changes:

- Change the firmware protocol max frame size back to 128 bytes
- Correct a bug with the reading out of UDS

## TK1-23.03

This is the official release of the "Bellatrix" version of
the Tillitis TKey device. This version is ready for general
use.

Given the OCI image `ghcr.io/tillitis/tkey-builder:1` built from
`contrib/Dockerfile` and the generic UDS.hex and UDI.hex, a clean
build should generate the following digest:

```
shasum -a256 application_fpga.bin
f11d6b0f57c5405598206dcfea284008413391a2c51f124a2e2ae8600cb78f0b  application_fpga.bin
```

### New and improved functionality

- (ALL) The TKey HW design, FW, protocol and first applications has
  been audited by a third party. No major issues was found, but the
  audit has lead to several updates, changes and fixes to improve
  the security and robustness. The third party report will be
  published when completed.

- (APPS) Applications can now use the whole 128 kByte RAM.

- (FW) The firmware now use the `FW_RAM` for the stack. It keeps no
  .bss or .data segments and only uses RAM for loading the
  application.

- (FW) The firmware has been hardened and the state machine simplified
  to reduce the number of commands that can be used and in which
  order. It exits early on failure to a fail state indicated by the
  RGB LED blinking red on error in an eternal loop.

- (FW) Steady white LED while waiting for initial commands. LED off
  while loading app.

- (HW) The memory system now has an execution monitor. The monitor
  detects attempts at reading instructions from the firmware ram.
  The execution monitor can also, when enabled by an application,
  detect attempts at reading instructions from the application
  stack. If any such attempt is detected, the memory system will
  force the CPU to read an illegal instruction, triggering the
  trap state in the CPU.

  Note that the execution monitor can only be enabled, not
  disabled. The address range registers defining the region
  protected by the monitor can only be set when the monitor
  has not yet been enabled.

- (HW) The CPU trap signal is now connected to an illegal instruction
  trap indicator. When an illegal instruction is detected, the RGB LED
  will start flashing red. Note that the CPU will stay in the trap
  state until the TKey device is disconnected.

- (HW) The RAM memory now includes an address randomisation and data
  scrambling mechanism to make it harder for someone outside of the
  CPU to find assets generated by and stored in the RAM by
  applications. This randomisation and scrambling is set up by
  firmware before the application is loaded, and does not affect how
  applications executes.

- (HW) The UART Rx FIFO now allows applications to read out the
  number of bytes received and not yet consumed by the application.

- (HW) The FPGA bitstream can now be stored in the non volatile
  configuration memory (NVCM). This is done using of a new icestorm
  tool developed partly in the project and sponsored by Tillitis
  and Mullvad. The tool supports locking down NVCM access after
  writing the FPGA bitstream to the memory.

- (TOOLS) There is now an OCI image
  (`ghcr.io/tillitis/tkey-builder:1`) and Dockerfile setting up all
  tools as needed to build the bitstream.

- (TOOLS) There is now a version of iceprog able to write to the FPGA
  bitstream to the NVCM and lock the NVCM from external access

### Bugs fixed

- No known bugs have been fixed. Numerous issues has been closed.

### Limitations

- The RAM address and data scrambling in this release is not
  cryptographically secure. It his however randomized every time
  a TKey device is powered up.

## engineering-release-2

### New and improved functionality

- (HW) The rosc TRNG has now been completed and tested. The TRNG
  can now be used to generate seeds by applicaitons.

- (HW) The main clock frequency has been increased to 18 MHz.

- (HW) The FW now has a separate RAM used during loading and
  measurement of applications.

- (HW) The UART Rx FIFO is now able to handle 512 bytes.

- (HW) The UART default bitrate has been increased to 62500 bps.

- (HW) Support for division instruction (div) was removed from
  PicoRV32. Please compile your programs with the Zmmul extension,
  `-march=rv32iczmmul` for `clang`.

- (HW) The UDI is locked down and can only be accessed by firmware, to
  prevent applications from tracking a particular TKey.

- (HW) The timer MMIO API now takes separate start and stop bits for
  triggering the respective action, mitigating a time-of-check to
  time-of-use (TOCTOU) issue.

- (FW) The firmware has been restructured to be a Finite State
  Machine (FSM) with defined states for booting, loading
  applications, measure applications, calculate the CDI and
  start the loaded application.

  This change also changes the firmware protocol which now accepts a
  request to load a binary with an optional USS and automatically
  returns its digest and start the program when the last data chunk is
  received.

- (FW) A BLAKE2s hash function present in firmware is now exposed for use
  by TKey apps (through a function pointer located in MMIO `BLAKE2S`).
  See [software.md](system_description/software.md) for more
  information.

- (FW) To make warm boot attacks harder, the firmware sleeps for a
  random number of cycles before reading out the sensitive UDS into
  FW RAM.

## engineering-release-1

### Hardware

#### Limitations

- The entropy generated by the TRNG has not yet been thoroughly tested,
  and the generator has not been adjusted to generate good, unbiased
  randomness. Any application that wants to use the entropy source
  SHOULD NOT use the output directly, but only as seed to a Digital
  Random Bit Generator (DRBG), such as Hash_DRBG.

- The UART is currently running at 38400 bps. Future releases will
  increase the bitrate when communication at higher bitrates has
  been verified as stable and error free.

- The internal clock frequency is currently limited to 12 MHz.
  Future releases will increase the clock frequency to provide
  improved performance.

- The functionality in the firmware is currently not exposed to the
  applications via a stable name space, API. Future releases will
  provide access to FW functions such as the BLAKE2s hash function.

- The timer currently does not include a timeout interrupt. Applications
  using the timer must check the status in order to detect a timeout event.

- The timer currently does not provide a set of typical settings.
  Applications using the timer must set timer and prescaler as
  needed to get the desired time given the current clock speed.
