/*
 * Copyright (C) 2025 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/assert.h"
#include "../tk1/flash.h"
#include "../tk1/led.h"
#include "../tk1/syscall_nrs.h"
#include "../tk1/types.h"

// clang-format off
static volatile uint32_t *system_reset = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
static volatile uint32_t *udi          = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
// clang-format on

// Proof-of-concept firmware for handling syscalls.
// This is NOT a best-practice example of secure syscall implementation.

int32_t syscall_handler(uint32_t syscall_nr, uint32_t arg1)
{
	switch (syscall_nr) {
	case TK1_SYSCALL_RESET:
		*system_reset = 1;
		return 0;
	case TK1_SYSCALL_SET_LED:
		set_led(arg1);
		return 0;
	case TK1_SYSCALL_GET_FLASH_CAPACITY: {
		uint8_t jedec_id[3];
		flash_release_powerdown();
		flash_read_jedec_id(jedec_id);
		flash_powerdown();
		return jedec_id[2];

	case TK1_SYSCALL_GET_VIDPID:
		// UDI is 2 words: VID/PID & serial. Return just the
		// first word. Serial is kept secret to the device
		// app.
		return udi[0];
	}

	default:
		assert(1 == 2);
	}

	assert(1 == 2);
	return -1; // This should never run
}
