# Release notes

## Upcoming release

- NOTE WELL! Rewritten I/O functions with new signatures and
  semantics!
- `blake2s()` with new signature.
- System call support.

### BLAKE2s hash function

The `blake2s()` function no longer call the firmware.

- The `blake2s.h` header file has moved to `blake2s/blake2s.h`.

- The `blake2s()` hash function has changed signature. It's now defined
  as:

  ```
  // All-in-one convenience function.
  int blake2s(void *out, size_t outlen,   // return buffer for digest
      const void *key, size_t keylen,     // optional secret key
      const void *in, size_t inlen);      // data to be hashed

  ```

- The component functions `blake2s_init()`, `blake2s_update()`, and
  `blake2s_final()` are now available.

### I/O

The Castor TKey hardware supports more USB endpoints:

- CDC - the same thing as older versions.
- FIDO security token, for FIDO-like apps.
- CCID, smart card interface.
- DEBUG, a HID debug port.

The communication is still over a single UART. To differ between the
endpoints we use an internal USB Mode Protocol between programs
running on the PicoRV32 and the CH552 USB Controller.

The I/O functions has changed accordingly. Please use:

- `readselect()` with appropriate bitmask (e.g. `IO_CDC|IO_FIDO`) to
  see if there's anything to read in the endpoints you are interested
  in. Data from endpoints not mentioned in the bitmask will be
  discarded.

- `read()` is now non-blocking and returns the number of bytes read
  from the endpoint you specify, because more might not be available
  yet.

- `write()` now takes an endpoint destination.

- We also introduce generic `putchar()`, `puts()`, `puthex()`,
  `putinthex()`, and `hexdump()` functions that take a destination
  argument.

We recommend you use only these functions for I/O on Castor and going
forward.

For compatibility to develop device apps for the Bellatrix platform
and earlier, use the low-level, blocking function `uart_read()` for
reads and *only* the `IO_UART` and `IO_QEMU` destinations for output
functions like `write()`, `puts()`.

### Debug prints

The optionally built debug prints have changed. You now use
`debug_puts()` et cetera instead of `qemu_*()`.

You define the debug output endpoint when you compile your program by
including `debug.h` and defining `QEMU_DEBUG` for the qemu debug port
or `TKEY_DEBUG` for output on the DEBUG HID endpoint. If you don't
define either, they won't appear in your code.

Similiarly, `assert()` now also follows `QEMU_DEBUG` or `TKEY_DEBUG`,
and prints something on either before halting the CPU.

Note that on the Bellatrix platform only `QEMU_DEBUG` works.

## v0.1.2

From now on tkey-libs is licensed under the BSD-2-Clause license,
moving from the previous GPLv2-only.

Note: There is a possibility that this update may impact the generated
CDI for an app that relies on this library. It is recommended to
always check for potential CDI changes for each specific app with
every update. If the generated CDI does change, and if applicable, it
should be clearly communicated to end users to prevent unintentional
changes to their identity.

Changes:
- New license, BSD-2-Clause
- Reuse compliant, see https://reuse.software/
- Fix row alignment in qemu_hexdump
- Update memory map, tk1_mem.h, from canonical tillitis-key1 repo
- Added make target for creating compile_commands.json for clangd
- Added missing include in touch.h

Full changelog:
[v0.1.1...v0.1.2](https://github.com/tillitis/tkey-libs/compare/v0.1.1...v0.1.2)

## v0.1.1

This is a minor release correcting a mistake and syncing with the
latest HW release, TK1-24.03.


Note: There is a possibility that this update may impact the generated
CDI for an app that relies on this library. It is recommended to
always check for potential CDI changes for each specific app with
every update. If the generated CDI does change, and if applicable, it
should be clearly communicated to end users to prevent unintentional
changes to their identity.

Changes:
- Update memory map, tk1_mem.h, to match the latest TK1-24.03 release.
- Default to tkey-builder:4 for the podman target
- Default to have QEMU debug enabled in tkey-libs. Mistakenly removed
  in previous release.
- Revise readme accordingly

Full changelog:
[v0.1.0...v0.1.1](https://github.com/tillitis/tkey-libs/compare/v0.1.0...v0.1.1)

## v0.1.0

This release contains some changes that forces applications that use
tkey-libs to be updated to work with this release.

Note: It is highly likely that this update will affect the CDI of the
TKey. It is advised to always verify this for each specific app, for
every update. If the CDI changes, and it is applicable, it should be
stated clearly to end users to avoid unknowingly changing the TKey
identity.

Breaking changes:
- Check destination buffer's size for read(). To prevent writing
  outside of destination buffer.
- Renaming LED-functions to follow led_*().

Changes:
- New function, secure_wipe(), to clean memory of secret data.
- New function, touch_wait(). Waits for a touch by the user, with
  selectable timeout.
- New function, led_get(). Get the value of the applied LED color.
- Upgraded Monocypher to 4.0.2.
- Add variable AR in Makefile to enabling passing llvm-ar from command
  line.
- Update example app to use led.h.
- Don't have QEMU debug enabled by default.
- Minor tweaks and formatting.

Full changelog:
[v0.0.2...v0.1.0](https://github.com/tillitis/tkey-libs/compare/v0.0.2...v0.1.0)

## v0.0.2

This release contains some changes that forces applications that use
tkey-libs to be updated to work with this release.

Breaking changes:
- Introducing include hierarchy to make it less generic, e.g.,
  `#include <tkey/led.h>`.
- Use stdint.h/stddef.h infavor of types.h.
- Library .a files built on top level to simplify inclusion.
- Upgraded Monocypher to 4.0.1.
- QEMU debug behaviour changed, instead of defining `NODEBUG` to
  disable debug, one has to enable it by defining `QEMU_DEBUG`.

Changes:
- Introduce functions to control the LED, led.h and led.c.
- New function, assert() to make an illegal instruction and forcing
  the CPU to halt.
- Add functions memcpy_s(), wordcpy_s(), memeq() from firmware
- Adding `const` to MMIO variables and qemu_* functions.
- Minor tweaks, clean up and bugfixes.

Full changelog:
[v0.0.1...v0.0.2](https://github.com/tillitis/tkey-libs/compare/v0.0.1...v0.0.2)


## v0.0.1

Just ripped from

https://github.com/tillitis/tillitis-key1-apps

No semantic changes.
