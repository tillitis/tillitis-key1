// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "ch554.h"

// Set pin p1.4 and p1.5 to GPIO output mode.
void gpio_init()
{
    // p1.4
    P1_MOD_OC &= ~0x10;
    P1_DIR_PU |= 0x10;

    // p1.5
    P1_MOD_OC &= ~0x20;
    P1_DIR_PU |= 0x20;
}

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

// Set pin p1.4 to GPIO input mode. (FPGA_CTS)
void gpio_init_p1_4_in()
{
    // p1.4
    P1_MOD_OC &= ~0x10; // Output Mode: 0 = Push-pull output, 1 = Open-drain output
    P1_DIR_PU &= ~0x10; // Port Direction Control and Pull-up Enable Register:
                        //    Push-pull output mode:
                        //        0 = Input.
                        //        1 = Output
                        //    Open-drain output mode:
                        //        0 = Pull-up resistor disabled
                        //        1 = Pull-up resistor enabled
}

// Read status of pin 1.4
uint8_t gpio_p1_4_get(void)
{
    return (P1 & 0x10); // p1.4
}

// Set pin p1.5 to GPIO output mode. (CH552_CTS)
void gpio_init_p1_5_out()
{
    // p1.5
    P1_MOD_OC &= ~0x20; // Output Mode: 0 = Push-pull output, 1 = Open-drain output
    P1_DIR_PU |=  0x20; // Port Direction Control and Pull-up Enable Register:
                        //    Push-pull output mode:
                        //        0 = Input.
                        //        1 = Output
                        //    Open-drain output mode:
                        //        0 = Pull-up resistor disabled
                        //        1 = Pull-up resistor enabled
}

// Set pin 1.5 high
void gpio_p1_5_set(void)
{
    P1 |= 0x20;
}

// Set pin 1.5 low
void gpio_p1_5_unset(void)
{
    P1 &= ~0x20;
}
