// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_LED_H
#define TKEY_LED_H

#include <stdint.h>
#include <tkey/tk1_mem.h>

// clang-format off
#define LED_BLACK 0
#define LED_RED   (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE  (1 << TK1_MMIO_TK1_LED_B_BIT)
#define LED_WHITE (LED_RED | LED_GREEN | LED_BLUE)
// clang-format on

uint32_t led_get(void);
void led_set(uint32_t ledvalue);
void led_flash_forever(uint32_t ledvalue);
#endif
