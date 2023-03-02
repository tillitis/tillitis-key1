#!/usr/bin/env python
import hid_test  # type: ignore
import time


def reset_tk1() -> None:
    """ Reset a TK1 contained in a TP1 programmer

    Manipulate the GPIO lines on the TP1 to issue a hardware reset
    to the TK1. The result is that TK1 again will be in firmware
    mode, so a new app can be loaded.
    """
    d = hid_test.ice40_flasher()
    d.gpio_set_direction(14, True)
    d.gpio_put(14, False)
    d.gpio_set_direction(14, False)
    d.close()


reset_tk1()
