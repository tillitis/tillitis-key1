[![ci](https://github.com/tillitis/tkey-libs/actions/workflows/ci.yaml/badge.svg?branch=main&event=push)](https://github.com/tillitis/tkey-libs/actions/workflows/ci.yaml)

# Device libraries for the Tillitis TKey

- C runtime: libcrt0.
- System call support: libsyscall.
- Common C functions including protocol calls: libcommon.
- Cryptographic functions: libmonocypher. Based on
  [Monocypher](https://github.com/LoupVaillant/Monocypher) version
  4.0.2
- BLAKE2s hash function: libblake2s.

Release notes in [RELEASE.md](RELEASE.md).

## Licenses

Unless otherwise noted, the project sources are copyright Tillitis AB,
licensed under the terms and conditions of the "BSD-2-Clause" license.
See [LICENSE](LICENSE) for the full license text.

Until Oct 8, 2024, the license was GPL-2.0 Only.

External source code we have imported are isolated in their own
directories. They may be released under other licenses. This is noted
with a similar `LICENSE` file in every directory containing imported
sources.

Imported sources:

- [Monocypher](https://github.com/LoupVaillant/Monocypher) (BSD-2) by
  Loup Vaillant.

- blake2s (CC-0), originally based on the reference implementation in
  [RFC 7693](https://www.rfc-editor.org/rfc/rfc7693.html) written by
  Markku-Juhani O. Saarinen ([original
  repository](https://github.com/mjosaarinen/blake2_mjosref). Imported
  from [Joachim Strömbergson's
  fork](https://github.com/secworks/blake2s/) used as a model for a
  hardware implementation.

### SPDX tags

The project uses single-line references to Unique License Identifiers
as defined by the Linux Foundation's [SPDX project](https://spdx.org/)
on its own source files, but not necessarily imported files. The line
in each individual source file identifies the license applicable to
that file.

The current set of valid, predefined SPDX identifiers can be found on
the SPDX License List at:

https://spdx.org/licenses/

We attempt to follow the [REUSE
specification](https://reuse.software/).

## Hardware support

### Bellatrix and earlier

Please note that:

- For reading, only use the blocking `uart_read()`.

- Only `IO_UART` and `IO_QEMU` destinations are useful for writing as
  in `write(IO_UART, ...)`, `puts(IO_UART, ...)`, and so on.

- Defining `QEMU_DEBUG` works with all the `debug_*` functions, but
  `TKEY_DEBUG` does not.

## Building

In order to build, you must have the `make`, `clang`, `llvm`, and
`lld` packages installed.

Version 15 or higher of LLVM/Clang is necessary for the RV32IC\_Zmmul
architecture we are using. For more detailed information on the
supported build and development environment, please refer to the
[Developer Handbook](https://dev.tillitis.se/).
## Building using Podman

You can also build the libraries with our OCI image
`ghcr.io/tillitis/tkey-builder`.

The easiest way to build this is if you have `make` installed:

```
make podman
```

You can also specify a different image by using
`IMAGE=localhost/tkey-builder-local`.

Or use Podman directly:

```
podman run --rm --mount type=bind,source=.,target=/src -w /src -it ghcr.io/tillitis/tkey-builder:4 make -j
```

## Minimal application build

You will typically need to link at least the `libcrt0` C runtime
otherwise your program won't even reach `main()`.

We provide a linker script in `apps.lds` which shows the linker the
memory layout.

Minimal compilation would look something like:

```
clang -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 \
  -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common \
  -fno-builtin-printf -fno-builtin-putchar -nostdlib -mno-relax -flto \
  -Wall -Werror=implicit-function-declaration \
  -I ../tkey-libs/include \
  -I ../tkey-libs -c -o foo.o foo.c

clang -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 \
  -mcmodel=medany -static -ffast-math -fno-common -nostdlib \
  -T ../tkey-libs/app.lds \
  -L ../tkey-libs -lcrt0 \
  -I ../tkey-libs -o foo.elf foo.o

```

## Makefile example

See `example-app/Makefile` for an example Makefile for a simple device
application.

## Debug output

If you want to have debug prints in your program you can use the
`debug_putchar()`, `debug_puts()`, `debug_putinthex()`,
`debug_hexdump()` and friends. See `include/tkey/debug.h` for list of
functions.

These functions will be turned on if you define either of these when
compiling your program and linking with `libcommon`:

- `QEMU_DEBUG`: Uses the special debug port only available in qemu to
  print to the qemu console.
- `TKEY_DEBUG`: Uses the extra HID device.

Note that if you use `TKEY_DEBUG` you *must* have something listening
on the corresponding HID device. It's usually the last HID device
created. On Linux, for instance, this means the last reported hidraw
in `dmesg` is the one you should do `cat /dev/hidrawX` on.
