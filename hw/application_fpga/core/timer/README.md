# timer
A simple timer with prescaler.

## Introduction
This core implements a simple timer with a prescaler. The prescaler
allows measurement of time durations rather than cycles. If for
example setting the prescaler to the clock frequency in Hertz, the
timer will count seconds.


## Details
The timer counter and the prescaler counter are both 32 bits.
When enabled the counter counts down one integer value per cycle.
