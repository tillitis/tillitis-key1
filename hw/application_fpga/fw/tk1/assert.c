// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <tkey/assert.h>

/*
 * The firmware's assert_halt(), strongly overriding the weak
 * default with app mode behavior in tkey-libs libcommon/assert.c.
 *
 * We are executing in firmware mode, which has higher privilege
 * than app mode, so we may wipe both the entire application RAM
 * (even when an app is loaded and executing its assert path through
 * a syscall into the firmware) and the full 4 KB FW_RAM, including
 * resetinfo, before halting the CPU.
 *
 * NOTE WELL: The wipe loops must only use registers, never the
 * stack. The firmware stack, .data, and .bss are in FW_RAM, which
 * is being cleared here. This code itself is in ROM and survives.
 *
 * noinline keeps exactly one copy in ROM.
 */
__attribute__((noinline)) void assert_halt(void)
{
	// Wipe the entire application RAM.
	// clang-format off
	asm volatile(
		"la a0, _sappram;"
		"la a1, _eappram;"
		"loop_wipe_appram:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop_wipe_appram;"
		:
		:
		: "a0", "a1", "memory");
	// clang-format on

	// Wipe the full 4 KB FW_RAM, also clearing resetinfo
	// clang-format off
	asm volatile(
		"la a0, _sfwram_all;"
		"la a1, _efwram_all;"
		"loop_wipe_fwram_all:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop_wipe_fwram_all;"
		:
		:
		: "a0", "a1", "memory");
	// clang-format on

	// Force illegal instruction to halt CPU
	asm volatile("unimp");

	// Not reached
	__builtin_unreachable();
}
