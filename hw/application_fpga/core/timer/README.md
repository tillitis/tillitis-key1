# timer
A simple timer with prescaler.

## Introduction
This core implements a simple timer with a prescaler. The prescaler
allows measurement of time durations rather than cycles. If for
example setting the prescaler to the clock frequency in Hertz, the
timer will count seconds.

## API

The following addresses define the API for the timer:

```
	ADDR_CTRL: 0x08
	CTRL_START_BIT: 0
	CTRL_STOP_BIT:  1

	ADDR_STATUS: 0x09
	STATUS_RUNNING_BIT: 0

	ADDR_PRESCALER: 0x0a
	ADDR_TIMER:     0x0b
```


## Details
The core consists of the timer_core module (in timer_core.v) and a top
level wrapper, timer (in timer.v). The top level wrapper implements
the API, while the timer_core implements the actual timer
functionality.

The timer counter and the prescaler counter are both 32 bits.
When enabled the counter counts down one integer value per cycle.

The timer will stop when reaching final zero (given by prescaler times the initial value of the timer)
and the running flag will be lowered.
