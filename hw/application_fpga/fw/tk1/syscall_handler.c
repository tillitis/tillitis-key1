/*
 * Copyright (C) 2025 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include "../tk1/resetinfo.h"
#include "../tk1/syscall_num.h"

// clang-format off
static volatile uint32_t *system_reset = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
static volatile uint32_t *udi          = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
static volatile uint8_t *resetinfo     = (volatile uint8_t *)TK1_MMIO_RESETINFO_BASE;
// clang-format on

int32_t syscall_handler(uint32_t number, uint32_t arg1)
{
	switch (number) {
	case TK1_SYSCALL_RESET:
		*system_reset = 1;
		return 0;
	case TK1_SYSCALL_RESET_WITH_INTENT:
		memcpy((uint8_t *)resetinfo, (uint8_t *)arg1, sizeof(struct reset));
		*system_reset = 1;
		return 0;
	case TK1_SYSCALL_SET_LED:
		led_set(arg1);
		return 0;
	case TK1_SYSCALL_GET_VIDPID:
		// UDI is 2 words: VID/PID & serial. Return just the
		// first word. Serial is kept secret to the device
		// app.
		return udi[0];
	default:
		assert(1 == 2);
	}

	assert(1 == 2);
	return -1; // This should never run
}
