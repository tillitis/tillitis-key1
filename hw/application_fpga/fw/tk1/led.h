/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LED_H
#define LED_H

#include "../tk1_mem.h"
#include "types.h"

// clang-format off
#define LED_BLACK 0
#define LED_RED   (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE  (1 << TK1_MMIO_TK1_LED_B_BIT)
#define LED_WHITE (LED_RED | LED_GREEN | LED_BLUE)
// clang-format on

void set_led(uint32_t led_value);
void forever_redflash();
#endif
