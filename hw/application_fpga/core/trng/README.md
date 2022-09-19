# trng
Implementation of the FiGaRO TRNG for FPGAs

## Introduction
# figaro


## Status
First version completed. In testing. Use with caution.


## Introduction
This is a an implementation of the FiGaRO true random
number generator (TRNG) [1]. The main FPGA target is Lattice iCE40
UltraPlus, but adaption to other FPGAs should be easy to do.


## Implementation details
The implementation instantiates four FiRO and four GaRO modules. The
modules includes state sampling. The polynomials used for the
oscillators are given by equotions (9)..(16) in paper [1]. The eight
outputs are then XORed together to form a one bit random value.

The random bit value is sampled at a rate controlled by a 24 bit
divisor.

## References
[1] [True Random Number Generator Based on Fibonacci-Galois
Ring Oscillators for FPGA](https://www.mdpi.com/2076-3417/11/8/3330/pdf)
