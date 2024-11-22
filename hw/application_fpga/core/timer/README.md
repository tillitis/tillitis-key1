# timer
A simple timer with prescaler.

## Introduction
This core implements a simple timer with a prescaler. The prescaler
can be used to divide the system clock frequency to yield a specific
amount of clock frequency ticks for each timer tick. This allows
measurement of time rather than cycles. If for example setting the
prescaler to the clock frequency in Hertz, the timer will count
seconds.

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
the API, while the timer_core implements the timer functionality.

The timer counter and the prescaler counter are both 32 bits. Both the
`prescaler` and the `timer` register can be set through the API above,
and should be a non-zero value, the defaults are one. These values
cannot be changed when the timer is running.

Bit zero of the `status` register shows if the timer is running, one
indicates a running timer.

Start the timer by writing 1 to bit zero of the `ctrl` register, write
1 to bit one to stop it.

Time elapsed can be calculated using:

```
time = clock_freq / (prescaler * timer)
```


