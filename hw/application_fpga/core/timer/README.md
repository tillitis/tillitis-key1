# timer
Timer with prescaler and support for detecting when a target time has
been reached.w

## Introduction
This core implements a simple timer with a prescaler and ability to
signal when a given time has been reached.

The prescaler allows measurement of time duration rather than
cycles. If for example setting the prescaler to the clock frequency in
Hertz, the timer will count seconds.

When started the timer will set the STATUS_RUNNING_BIT. The timer will
not stop until the CTRL_STOP has been asserted.

When timer has been started, after the set (prescaler * timer) number
of cycles, the timer will set the STATUS_REACHED bit.

## API
The following addresses define the API for the timer:

```
	ADDR_CTRL:          0x08
	CTRL_START_BIT:     0
	CTRL_STOP_BIT:      1

	ADDR_STATUS:        0x09
	STATUS_RUNNING_BIT: 0
	STATUS_REACHED:     1

	ADDR_PRESCALER:     0x0a
	ADDR_TIMER:         0x0b
```

ADDR_PRESCALER and ADDR_TIMER registers should be set to a
non-zero value. Default values for the these registers are one (1).
Note that these registers can't be changed when the timer is running.
