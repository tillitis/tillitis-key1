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

The API has a single address, and a single bit in that address:

```
	ADDR_STATUS: 0x09
	STATUS_EVENT_BIT: 0
```

SW should clear any stray attempts before signalling to the user that
a touch event is expected. Clearing an event is done by writing the
the status address, the value written does not matter.
