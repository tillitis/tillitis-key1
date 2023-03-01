/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "led.h"
#include "../tk1_mem.h"
#include "types.h"

void forever_redflash()
{
	static volatile uint32_t *led = (volatile uint32_t *)TK1_MMIO_TK1_LED;

	int led_on = 0;
	for (;;) {
		*led = led_on ? LED_RED : LED_BLACK;
		for (volatile int i = 0; i < 800000; i++) {
		}
		led_on = !led_on;
	}
}
