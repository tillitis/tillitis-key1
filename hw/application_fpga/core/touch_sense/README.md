# touch_sense

Core that handles touch sensor events and provides them to the SW via
an API.

## Introduction

This core implements a touch sensor handler. The core detects and
holds events for SW to read. The touch sensor input is expected to be
a change in level from low (0) to high (1). When an event is seen, the
core will set a status bit that SW can read.

SW must clear the captured event by writing to the status
register. The core will wait for the sensor input to become low again
before being able to detect another event.


## API
The API has two addresses.

```
	ADDR_STATUS: 0x09
	STATUS_EVENT_BIT: 0

	ADDR_PRESENT: 0x0a
	FINGER_PRESENT_BIT: 0
```

In order to detect an event, SW should clear any stray attempts before
signalling to the user that a touch event is expected. Clearing an
event is done by writing the the status address, the value written
does not matter.

When an event has been detected, that is the sampled input from the
sensor has gone from low to high, the STATUS_EVENT_BIT will be high
(set). When SW reads a high bit, the SW should as soon as possible
clear the event by writing to the status register. The value written
does not matter.

The FINGER_PRESENT bit is the sampled input from the sensor. The bit
will be high as long as a finger is present on the sensor. When a
finger is present the bit will be low.
