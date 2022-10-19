# trng
Implementation of the Tillitis True Random Number Generator (TRNG).

## Introduction
Applications running on the Tillitis Key device may have a need of random numbers.
As unpredictable initial vectors, as challnges, random tokens etc.

The Tillitis TRNG supports these applications by providing a hardware based
source of entropy (digital noise) with a uniform distribution.

Note that the data provided by the TRNG is entropy, not processed random numbers.
The data should NOT be used directly, but used as seed for a (cryptographically safe)
random number generator algorithm.


## Status
First version completed. In testing. Use with caution.

## How to use
The ready bit in the status register indicates that there is a new word of
entropy available to read out. Applications requiring multiple words of
entropy MUST wait for the ready bit to be set before reading ut
subseqent words. Not waiting for the ready bit to be set will lead to reading out
the same entropy data more than once.

Applications that need cryptographically safe random number should use the output
from the TRNG as seed to a Digital Random Bit Generator (DRBG), for example a Hash_DRBG.


## Implementation details
The implementation is based on free running digital oscillators. The implementation creates
two sets of oscillators by instantiating a number if LCs configured as one bit inverter gates,
where the output of the inverter is connected to its own input. The oscillators will have a toggle
rate based on the given internal gate delay and the wire delay through given by the feedback
circuit.

After a given number of clock cycles the outputs from the oscillators in each group are
XOR combined and sampled into two separate registers. This process is repeated a second time,
producing two more bits, one for each group. These two sets of two bits are then XOR combined
to produce a single entropy bit. This means that an entropy bit is the XOR combined result
from two oscillator groups over two sampling events.

Entropy bits are collected into an entropy word. When at least 32 bits have been collected,
the ready bit is set, indicating to SW that a new entropy word is available.

Note that the entropy word is not held for the SW to read out. Sampling and collection is running
continuosly, and the word read by SW will contain the latest 32 bits collected. Entropy bits
not read by SW will be discarded at the same rate as new bits are collected.

Currently the following build time parameters are used to configure the implementation:

- 4096 cycles between sampling
- 16 oscillators in each group
- 64 bits collected before setting the ready flag


---
