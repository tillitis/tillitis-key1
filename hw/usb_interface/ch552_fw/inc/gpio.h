// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>

#define PIN_P1_4 0x10
#define PIN_P1_5 0x20

#define PIN_FPGA_CTS PIN_P1_5
#define PIN_CH552_CTS PIN_P1_4

void gpio_set(uint8_t pin);
void gpio_unset(uint8_t pin);
uint8_t gpio_get(uint8_t pin);

void gpio_dir_in(uint8_t pin);
void gpio_dir_out(uint8_t pin);
#endif
