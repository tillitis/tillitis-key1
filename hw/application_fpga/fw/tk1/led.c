/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "led.h"
#include "../tk1_mem.h"
#include "types.h"

static volatile uint32_t *led = (volatile uint32_t *)TK1_MMIO_TK1_LED;

void set_led(uint32_t led_value)
{
	*led = led_value;
}
