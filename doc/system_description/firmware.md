# Tillitis Key firmware

## Build the firmware

You need Clang with 32 bit RISC-V support. You can check this with:

```
$ llc --version | grep riscv32
    riscv32    - 32-bit RISC-V
```

or just try building.

Build the FPGA bitstream with the firmware using `make` in the
`hw/application_fpga` directory.

If your available `objcopy` and `size` commands is anything other than
the default `llvm-objcopy` and `llvm-size` define `OBJCOPY` and `SIZE`
to whatever they're called on your system.

## Firmware state machine

States:

- `initial` - At start.
- `init_loading` - Reset app digest, size, `USS` and load address.
- `loading` - Expect more app data or a reset by `LoadApp()`.
- `run` - Computes CDI and starts the device app.

Commands in state `initial`:

| *command*          | *next state*   |
|--------------------|----------------|
| NameVersion()      | unchanged      |
| GetUDI()           | unchanged      |
| LoadApp(size, uss) | `init_loading` |
|                    |                |

Commands in `init_loading`:

| *command*          | *next state*   |
|--------------------|----------------|
| NameVersion()      | unchanged      |
| GetUDI()           | unchanged      |
| LoadApp(size, uss) | `init_loading` |
| LoadAppData(data)  | `loading`      |
|                    |                |

Commands in `loading`:

| *command*          | *next state*                     |
|--------------------|----------------------------------|
| NameVersion()      | unchanged                        |
| GetUDI()           | unchanged                        |
| LoadApp(size, uss) | `init_loading`                   |
| LoadAppData(data)  | `loading` or `run` on last chunk |
|                    |                                  |

Behaviour:

- NameVersion: identifies stick.
- GetUDI: returns the Unique Device ID with vendor id, product id,
  revision, serial number.
- LoadApp(size, uss): Start loading an app with this size and user
  supplied secret.
- LoadAppData(data): Load chunk of data of app. When last data chunk
  is received we compute and return the digest.

## Using QEMU

Checkout the `tk1` branch of [our version of the
qemu](https://github.com/tillitis/qemu) and build:

```
$ git clone -b tk1 https://github.com/tillitis/qemu
$ mkdir qemu/build
$ cd qemu/build
$ ../configure --target-list=riscv32-softmmu --disable-werror
$ make -j $(nproc)
```

(Built with warnings-as-errors disabled, see [this
issue](https://github.com/tillitis/qemu/issues/3).)

Run it like this:

```
$ /path/to/qemu/build/qemu-system-riscv32 -nographic -M tk1,fifo=chrid -bios firmware.elf \
  -chardev pty,id=chrid
```

This attaches the FIFO to a tty, something like `/dev/pts/16` which
you can use with host software to talk to the firmware.

To quit QEMU you can use: `Ctrl-a x` (see `Ctrl-a ?` for other commands).

Debugging? Use the HTIF console by removing `-DNOCONSOLE` from the
`CFLAGS` and using the helper functions in `lib.c` for printf-like
debugging.

You can also use the qemu monitor for debugging, e.g. `info
registers`, or run qemu with `-d in_asm` or `-d trace:riscv_trap`.

Happy hacking!
