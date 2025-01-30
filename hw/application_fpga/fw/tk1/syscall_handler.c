/*
 * Copyright (C) 2025 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/assert.h"
#include "../tk1/led.h"
#include "../tk1/syscall_num.h"
#include "../tk1/types.h"

// clang-format off
static volatile uint32_t *system_reset = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
// clang-format on

int32_t syscall_handler(uint32_t number, uint32_t arg1)
{
	switch (number) {
	case TK1_SYSCALL_RESET:
		*system_reset = 1;
		return 0;
	case TK1_SYSCALL_SET_LED:
		set_led(arg1);
		return 0;
	default:
		assert(1 == 2);
	}

	assert(1 == 2);
	return -1; // This should never run
}
