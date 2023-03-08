#!/usr/bin/env python
"""Automatically reset a TK-1"""

from iceflasher import IceFlasher


def reset_tk1() -> None:
    """ Reset a TK1 contained in a TP1 programmer

    Manipulate the GPIO lines on the TP1 to issue a hardware reset
    to the TK1. The result is that TK1 again will be in firmware
    mode, so a new app can be loaded.
    """
    flasher = IceFlasher()
    flasher.gpio_set_direction(14, True)
    flasher.gpio_put(14, False)
    flasher.gpio_set_direction(14, False)


reset_tk1()
