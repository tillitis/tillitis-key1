// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/led.h>

// clang-format off
static volatile uint32_t* const led = (volatile uint32_t *)TK1_MMIO_TK1_LED;
// clang-format on

void led_set(uint32_t ledvalue)
{
	*led = ledvalue;
}

uint32_t led_get()
{
	return *led;
}

void led_flash_forever(uint32_t ledvalue)
{
	int led_on = 0;

	for (;;) {
		*led = led_on ? ledvalue : LED_BLACK;
		for (volatile int i = 0; i < 800000; i++) {
		}
		led_on = !led_on;
	}
}
