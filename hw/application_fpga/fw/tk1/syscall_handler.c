/*
 * Copyright (C) 2025 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/led.h>
#include <tkey/lib.h>

#include "partition_table.h"
#include "storage.h"

#include "../tk1/resetinfo.h"
#include "../tk1/syscall_num.h"

// clang-format off
static volatile uint32_t *system_reset = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
static volatile uint32_t *udi          = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
static volatile uint8_t  *resetinfo    = (volatile uint8_t *) TK1_MMIO_RESETINFO_BASE;
// clang-format on

extern struct partition_table part_table;

int32_t syscall_handler(uint32_t number, uint32_t arg1, uint32_t arg2,
			uint32_t arg3)
{
	switch (number) {
	case TK1_SYSCALL_RESET:
		// TODO: Take length from user
		memcpy((uint8_t *)resetinfo, (uint8_t *)arg1, sizeof(struct reset));
		*system_reset = 1;

		return 0;
	case TK1_SYSCALL_ALLOC_AREA:
		if (storage_allocate_area(&part_table) < 0) {
			debug_puts("couldn't allocate storage area\n");
			return -1;
		}

		return 0;
	case TK1_SYSCALL_DEALLOC_AREA:
		if (storage_deallocate_area(&part_table) < 0) {
			debug_puts("couldn't deallocate storage area\n");
			return -1;
		}

		return 0;
	case TK1_SYSCALL_WRITE_DATA:
		if (storage_write_data(&part_table, arg1, (uint8_t *)arg2,
				       arg3) < 0) {
			debug_puts("couldn't write storage area\n");
			return -1;
		}

		return 0;
	case TK1_SYSCALL_READ_DATA:
		if (storage_read_data(&part_table, arg1, (uint8_t *)arg2,
				       arg3) < 0) {
			debug_puts("couldn't read storage area\n");
			return -1;
		}

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
