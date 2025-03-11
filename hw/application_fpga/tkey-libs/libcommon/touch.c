// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <tkey/debug.h>
#include <tkey/led.h>
#include <tkey/touch.h>

// CPU clock frequenzy in Hz
#define CPUFREQ 18000000

// clang-format off
static volatile uint32_t *timer		  = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_status	  = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
static volatile uint32_t *timer_ctrl	  = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
static volatile uint32_t *touch		  = (volatile uint32_t *)TK1_MMIO_TOUCH_STATUS;
// clang-format on

// Returns !0 if touch sensor has been touched
#define touched() (*touch & (1 << TK1_MMIO_TOUCH_STATUS_EVENT_BIT))

bool touch_wait(int color, int timeout_s)
{
	int ledon = 0;
	int orig_color = led_get();
	uint32_t time = 0;
	uint32_t lasttime = 0;

	// Tick once every decisecond
	*timer_prescaler = CPUFREQ / 10;
	*timer = timeout_s * 10; // Seconds

	// Start timer
	*timer_ctrl |= (1 << TK1_MMIO_TIMER_CTRL_START_BIT);

	// Acknowledge any stray touch events before waiting for real
	// touch
	*touch = 0;

	// Blink until either the touch sensor has been touched or the
	// timer hits 0.
	while (!touched() && *timer_status != 0) {
		time = *timer;
		if (time % 2 == 0 && time != lasttime) {
			lasttime = time;
			ledon = !ledon;
			led_set(ledon ? color : LED_BLACK);
		}
	}

	// Restore LED
	led_set(orig_color);

	// Do we have a timeout?
	if (*timer_status == 0) {
		return false;
	}

	// Stop timer
	*timer_ctrl |= (1 << TK1_MMIO_TIMER_CTRL_STOP_BIT);

	// Confirm touch event
	*touch = 0;

	return true;
}
