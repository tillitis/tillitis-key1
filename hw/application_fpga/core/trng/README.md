# trng

Implementation of the Tillitis True Random Number Generator (TRNG).

## Introduction

Applications running on the Tillitis TKey device may have a need of
random numbers.  For unpredictable initial vectors, challnges, random
tokens etc.

The Tillitis TRNG supports these applications by providing a hardware
based source of entropy (digital noise) with a uniform distribution.

Note that the data provided by the TRNG is entropy, not processed
random numbers.  The data should NOT be used directly, but used as
seed for a (cryptographically safe) random number generator (CSPRNG)
algorithm.


## Status

First version of TKey completed. The TRNG has been tested and provides
good entropy suitable as seed for a CSPRNG.


## API

The TRNG API provides to readable addresses:

```
	ADDR_STATUS:      0x09
	STATUS_READY_BIT: 0
	ADDR_ENTROPY:     0x20
```

The STATUS_READY_BIT in the status register indicates that there is a
new word of entropy available to read out. When the word is read from
ADDR_ENTROPY register, the ready bit is cleared.

Applications requiring multiple words of entropy MUST wait for the
ready bit to be set again before reading ut subseqent words. Not
waiting for the ready bit to be set will lead to reading out (at least
parts of) the same entropy data more than once.

Applications that need cryptographically safe random number should use
the output from the TRNG as seed to a Digital Random Bit Generator
(DRBG), for example a Hash_DRBG.


## Implementation details

The implementation is based on free running digital oscillators. The
implementation creates two sets of oscillators by instantiating a
number if LCs configured as one bit inverter gates, where the output
of the inverter is connected to its own input. The oscillators will
have a toggle rate based on the given internal gate delay and the wire
delay through given by the feedback circuit.

After a given number of clock cycles (4096) the outputs from the
oscillators in each group are XOR combined and sampled into two
separate registers. This process is repeated a second time, producing
two more bits, one for each group. These two sets of two bits are then
XOR combined to produce a single entropy bit. This means that an
entropy bit is the XOR combined result from two oscillator groups over
two sampling events.

Entropy bits are collected into an entropy word. When at least 32 bits
have been collected, the ready bit is set, indicating to SW that a new
entropy word is available.

Note that the entropy word is not held for the SW to read
out. Sampling and collection is running continuosly, and the word read
by SW will contain the latest 32 bits collected. Entropy bits not read
by SW will be discarded at the same rate as new bits are collected.

Currently the following build time parameters are used to configure
the implementation:

- 4096 cycles between sampling
- 16 oscillators in each group
- 64 bits collected before setting the ready flag

With the TKey device running at 18 MHz this means that we sample bits
at 4.3 kbps. Since we sample twice to produce a single bit, the
effective raw bitrate is 2.1 kbps.

The 64 bits collected means that there is a separation of at least 32
collected entropy bits between bits in the words read out.

---
