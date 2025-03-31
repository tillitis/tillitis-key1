# b2s

The firmware included a BLAKE2s digest of the expected device app in
the first app slot. The firmware refuses to start the app if the
computed digest differs from the constant.

To simplify computing the digest, use this tool with the `-c` flag for
including the digest in a C program:

## Building

`go build`

## Running

```
./b2s -m b2s -c
// BLAKE2s digest of b2s
uint8_t digest[32] = {
0x17, 0x36, 0xe9, 0x4e, 0xeb, 0x1b, 0xa2, 0x30, 0x89, 0xa9, 0xaa, 0xe, 0xf2, 0x6f, 0x35, 0xb2, 0xa9, 0x89, 0xac, 0x64, 0x63, 0xde, 0x38, 0x60, 0x47, 0x40, 0x91, 0x4e, 0xd7, 0x72, 0xa0, 0x58,
};
```

To print the digest in a more user friendly way, leave out the `-c`:

```
./b2s -m b2s
1736e94eeb1ba23089a9aa0ef26f35b2a989ac6463de38604740914ed772a058 b2s
```
