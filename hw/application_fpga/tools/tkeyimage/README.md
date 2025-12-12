# tkeyimage

A tool to parse or generate partition table or entire filesystems for
the TKey.

- Parse with `-i file.bin` for "input".
- Generate with `-o file.bin` for "output".

Add `-f` to parse or generate an entire flash image file.

For more options see `tkeyimage -h`.

## Usage

### Inspect a partition table dump

Dump the entire data from flash, then inspect:

```
$ tillitis-iceprog -R 1M dump.bin
$ ./tkeyimage -i dump.bin -f
INFO: main.Flash struct is 1048576 byte long
Partition Table Storage
  Partition Table
    Header
      Version          : 1
    Preloaded App 0
      Size             : 23796
      Digest           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      Signature        : 00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
      Pubkey           : 00000000000000000000000000000000
                         00000000000000000000000000000000
    Preloaded App 1
      Size             : 264
      Digest           : 96bb4c90603dbbbe09b9a1d7259b5e9e
                         61bedd89a897105c30c9d4bf66a98d97
      Signature        : ccb60c034e559b8c695f25233b80c245
                         e099316324e1a4e68a14c82d834eee58
                         5700cd5c29b64e74159a4dbf3fed030a
                         140e981fb3b6972c125afb4d4497da0a
      Pubkey           : 9b62773323ef41a11834824194e55164
                         d325eb9cdcc10ddda7d10ade4fbd8f6d
  Digest               : 4628f142764f724e45e05b20363960967705cfcee8285b2d9d207e04a46e275e
```

Read only the first copy of the partition table from flash to file,
then inspect:

```
$ tillitis-iceprog -o 128k -r partition.bin
$ ./tkeyimage -i partition.bin
INFO: main.PartTableStorage struct is 365 byte long
Partition Table Storage
  Partition Table
    Header
      Version          : 1
    Preloaded App 0
      Size             : 32652
      Digest           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      Signature        : 00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
      Pubkey           : 00000000000000000000000000000000
                         00000000000000000000000000000000
    Preloaded App 1
      Size             : 0
      Digest           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      Signature        : 00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
      Pubkey           : 55555555555555555555555555555555
                         55555555555555555555555555555555
  Digest               : 4b2446e67e35d3e214862e60cb8db62f93b54849d6d94f443ae0a25356a3c553
```

### Generate a partition table

If you want to generate just a partition table:

```
$ ./tkeyimage -o partition.bin
```

With an app in slot 0, filling in the size in the partition table:

```
$ ./tkeyimage -o partition.bin -app0 ../../apps/testloadapp.bin
```

With an app in slot 0 and a signed app in slot 1:

```
$ ./tkeyimage -o partition.bin -app0 app0.bin -app1 app1.bin -app1sig app1.bin.sig -app1pub pubkey
```

### Generate flash image

The program can also generate an entire flash image for use either
with real hardware or qemu.

Generate like this:

```
$ ./tkeyimage -o flash.bin -f -app0 ../../apps/testloadapp.bin
```

Using `-app0` is mandatory because TKey firmware won't start without
an app in slot 0.

The qemu args to use to run with `flash.bin` as the flash are:

```
-drive file=flash.bin,if=mtd,format=raw,index=0
```

A complete command example:

```
$ qemu-system-riscv32 -nographic -M tk1-castor,fifo=chrid \
-bios qemu_firmware.elf -chardev pty,id=chrid -s -d guest_errors \
-drive file=flash.bin,if=mtd,format=raw,index=0
```

### Signatures

The signature file format is similar to that of `tkey-sign`. Note that
it is up to the app that will verify the signature to specify what
signing algorithm to use. `tkeyimage` does not enforce a specific
algorithm.
