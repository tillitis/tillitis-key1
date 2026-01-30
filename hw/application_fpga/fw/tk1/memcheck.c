// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/tk1_mem.h>

#include "memcheck.h"

// Check if object at pointer p with size size is located in app RAM.
//
// Return true if object is in app RAM. Otherwise return false.
bool in_app_ram(const void *p, size_t size)
{
	uintptr_t addr = (uintptr_t)p;

	if (TK1_RAM_BASE > UINTPTR_MAX || TK1_RAM_SIZE > UINTPTR_MAX) {
		assert(1 == 2);
	}
	if (TK1_RAM_BASE > UINTPTR_MAX - TK1_RAM_SIZE) {
		assert(1 == 2);
	}
	uintptr_t stop = TK1_RAM_BASE + TK1_RAM_SIZE;

	// Lower than RAM?
	if (addr < TK1_RAM_BASE) {
		return false;
	}

	// Higher than RAM's end?
	if (addr >= stop) {
		return false;
	}

	// How much is left from addr to end of RAM?
	uintptr_t left = stop - addr;

	if (size > left) {
		return false;
	}

	return true;
}
