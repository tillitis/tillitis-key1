// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/tk1_mem.h>

#include "memcheck.h"

bool in_app_ram(const void *p, size_t size)
{
	uintptr_t addr = (uintptr_t)p;

	if (TK1_RAM_BASE > UINTPTR_MAX) {
		assert(1 == 2);
	}

	if ((uintptr_t)TK1_RAM_BASE + TK1_RAM_SIZE < TK1_RAM_BASE) {
		assert(1 == 2);
	}

	if (addr < TK1_RAM_BASE) {
		return false;
	}

	if (size > TK1_RAM_SIZE) {
		return false;
	}

	// Checking 'addr + size > BASE + SIZE' without overflow
	if (addr > (uintptr_t)TK1_RAM_BASE + TK1_RAM_SIZE - size) {
		return false;
	}

	return true;
}
