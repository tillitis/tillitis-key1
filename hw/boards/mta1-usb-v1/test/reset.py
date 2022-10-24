#!/usr/bin/env python3
import hid_test
import time

def reset_mta1():
    """ Manipulate the GPIO lines on the MTA1-USB-CH552 Programmer to issue a hardware reset to the MTA1 """
    d = hid_test.ice40_flasher()
    d.gpio_set_direction(14, True)
    d.gpio_put(14, False)
    d.gpio_set_direction(14, False)
    d.close()

reset_mta1()
