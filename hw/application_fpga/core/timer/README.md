# timer
Timer with prescaler and support for free running mode.

## Introduction
This core implements a simple timer with a prescaler and support for a
free running mode.

The prescaler allows measurement of time durations rather than
cycles. If for example setting the prescaler to the clock frequency in
Hertz, the timer will count seconds. After (prescaler * timer) number
of cycles the timer will stop. Checking status of the timer can be
done by reading the STATUS_RUNNING_BIT. If set to zero, the timer has
completed.

If the free running mode is set (default off), the counter will not
stup when the number of cycles defined by (prescaler * timer) has been
reached. Instead the timer continues until the CTRL_STOP_BIT is
asserted. Note that in free running mode, the ADDR_PRESCALER shall be
set to one (1).


## API
The following addresses define the API for the timer:

```
	ADDR_CTRL:          0x08
	CTRL_START_BIT:     0
	CTRL_STOP_BIT:      1

	ADDR_STATUS:        0x09
	STATUS_RUNNING_BIT: 0

	ADDR_PRESCALER:     0x0a
	ADDR_TIMER:         0x0b

	ADDR_FREE_RUNNING:  0x0c
	FREE_RUNNING_BIT    0

```

ADDR_PRESCALER and ADDR_TIMER should be set to a non-negative value (default are one.)
Note that these values can't be changed when the timer is running.
