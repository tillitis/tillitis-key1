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

```bash
$ tillitis-iceprog -R 1M dump.bin
$ ./tkeyimage -i dump.bin -f
INFO: main.Flash struct is 1048576 byte long
Partition Table Storage
  Partition Table
    Header
      Version          : 1
    Preloaded App 0
      Size             : 50780
      Digest           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      Signature        : 00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
      Name             : 00000000000000000000000000000000
      Pubkey           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      MAC              : 00000000000000000000000000000000
                         00000000000000000000000000000000
    Preloaded App 1
      Size             : 2988
      Digest           : cc271cec985be7ebc248faf3f8817d98
                         2e731ca9030b4dd4de9de719221b9740
      Signature        : d6cbbea868e4f5f8b39b8557198d014e
                         e1c2bce3e4d61417f9f07b5b897160b7
                         fd98ac066041eeaba835fd106c10a38e
                         90993fe0d21de61dfc8e524d1c844103
      Name             : 74657374696e67000000000000000000
      Pubkey           : 9b62773323ef41a11834824194e55164
                         d325eb9cdcc10ddda7d10ade4fbd8f6d
      MAC              : 551b7b8d1ac8a61d6c220d646f4466c4
                         5f8acaa14717a9a8a6b14f15c744d27d
  Digest               : cdc6eb7616ac419e9cda84725b6428ddf8ae88fd07361d9fd848cdf88ff80b10
```

Read only the first copy of the partition table from flash to file,
then inspect:

```bash
$ tillitis-iceprog -o 128k -r partition.bin
$ ./tkeyimage -i partition.bin
INFO: main.PartTableStorage struct is 525 byte long
Partition Table Storage
  Partition Table
    Header
      Version          : 1
    Preloaded App 0
      Size             : 50780
      Digest           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      Signature        : 00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
                         00000000000000000000000000000000
      Name             : 00000000000000000000000000000000
      Pubkey           : 00000000000000000000000000000000
                         00000000000000000000000000000000
      MAC              : 00000000000000000000000000000000
                         00000000000000000000000000000000
    Preloaded App 1
      Size             : 2988
      Digest           : cc271cec985be7ebc248faf3f8817d98
                         2e731ca9030b4dd4de9de719221b9740
      Signature        : 25ebacdb5bb70cf19307a64a1e18cc53
                         c39b87a8dfb13059200b7f67d99103a0
                         42abc0d97f1b2c758022734678b071ac
                         c20fe035182e3cffdee2f9f187a2ac0e
      Name             : 6170705f610000000000000000000000
      Pubkey           : 9b62773323ef41a11834824194e55164
                         d325eb9cdcc10ddda7d10ade4fbd8f6d
      MAC              : 226b6f873cf347955fb16027b8b53dc9
                         e1fa6a9f1014f63f91c6a9f182f3f1c6
  Digest               : 35d5d99fe55427ea63b975aff6f7afb237cbec96086022e47a662417df4edcce
```

### Generate a partition table

If you want to generate just a partition table:

```bash
./tkeyimage -o partition.bin
```

With an app in slot 0, filling in the size in the partition table:

```bash
./tkeyimage -o partition.bin -app0 ../../apps/testloadapp.bin
```

With an app in slot 0 and a signed app in slot 1:

```bash
./tkeyimage -o partition.bin -app0 app0.bin -app1 app1.bin -app1cert app1_cert.json
```

### Generate flash image

The program can also generate an entire flash image for use either
with real hardware or qemu.

Generate like this:

```bash
./tkeyimage -o flash.bin -f -app0 ../../apps/testloadapp.bin
```

Using `-app0` is mandatory because TKey firmware won't start without
an app in slot 0.

The qemu args to use to run with `flash.bin` as the flash are:

```bash
-drive file=flash.bin,if=mtd,format=raw,index=0
```

A complete command example:

```bash
qemu-system-riscv32 -nographic -M tk1-castor,fifo=chrid \
-bios qemu_firmware.elf -chardev pty,id=chrid -s -d guest_errors \
-drive file=flash.bin,if=mtd,format=raw,index=0
```

### Distribution certificate

Find more information about the certificate file format in
[tkey-boot-verifier](https://github.com/tillitis/tkey-boot-verifier/).

Note that it is up to the app that will verify the signature to
specify what signing algorithm to use. `tkeyimage` does not enforce a
specific algorithm, nor does it verify the signature.
