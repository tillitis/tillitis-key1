// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/led.h>
#include <tkey/timer.h>

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

// Indicate busy
//
// On platforms with multicolor LEDs ledvalue sets the color.
void led_indicate_busy(uint32_t ledvalue)
{
	led_set(ledvalue);
}

// Indicate idle
void led_indicate_idle()
{
	led_set(LED_BLACK);
}

// Indicate recoverable error
//
// Blink LED count times before returning.
// On platforms with multicolor LEDs ledvalue sets the color.
void led_indicate_error_alert(int count, uint32_t ledvalue)
{
	uint32_t orig_color = *led;

	led_set(LED_BLACK);
	while (count > 0) {
		timer_wait_ms(600);
		led_set(LED_RED);
		timer_wait_ms(600);
		led_set(LED_BLACK);
		count--;
	}
	timer_wait_ms(600);

	led_set(orig_color);
}
