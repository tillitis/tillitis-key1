// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "ch554.h"

void gpio_set(uint8_t pin)
{
    switch (pin) {
    case 0x10: // p1.4
        P1 |= 0x10;
        break;
    case 0x20: // p1.5
        P1 |= 0x20;
        break;
    default: // do nothing, unsupported pin.
        break;
    }
}

void gpio_unset(uint8_t pin)
{
    switch (pin) {
    case 0x10:
        P1 &= ~0x10;
        break;
    case 0x20:
        P1 &= ~0x20;
        break;
    default: // do nothing, unsupported pin.
        break;
    }
}

uint8_t gpio_get(uint8_t pin)
{
    uint8_t ret = 0;

    switch (pin) {
    case 0x10: // p1.4
        ret = P1 & 0x10;
        break;
    case 0x20: // p1.5
        ret = P1 & 0x20;
        break;
    default: // do nothing, unsupported pin.
        ret = 0xff;
        break;
    }

    return ret;
}

void gpio_dir_in(uint8_t pin)
{
    P1_MOD_OC &= ~pin; // Output Mode: 0 = Push-pull output, 1 = Open-drain output
    P1_DIR_PU &= ~pin; // Port Direction Control and Pull-up Enable Register:
                       //    Push-pull output mode:
                       //        0 = Input.
                       //        1 = Output
                       //    Open-drain output mode:
                       //        0 = Pull-up resistor disabled
                       //        1 = Pull-up resistor enabled
}

void gpio_dir_out(uint8_t pin)
{
    P1_MOD_OC &= ~pin; // Output Mode: 0 = Push-pull output, 1 = Open-drain output
    P1_DIR_PU |=  pin; // Port Direction Control and Pull-up Enable Register:
                       //    Push-pull output mode:
                       //        0 = Input.
                       //        1 = Output
                       //    Open-drain output mode:
                       //        0 = Pull-up resistor disabled
                       //        1 = Pull-up resistor enabled
}
