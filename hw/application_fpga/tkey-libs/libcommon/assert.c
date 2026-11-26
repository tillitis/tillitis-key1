// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <tkey/assert.h>
#include <tkey/io.h>
#include <tkey/lib.h>

void assert_fail(enum ioend dest, const char *assertion, const char *file,
		 unsigned int line, const char *function)
{
	puts(dest, "assert: ");
	puts(dest, assertion);
	puts(dest, " ");
	puts(dest, file);
	puts(dest, ":");
	putinthex(dest, line);
	puts(dest, " ");
	puts(dest, function);
	puts(dest, "\n");

	// Wipe RAM and halt
	assert_halt();

	// Not reached
	__builtin_unreachable();
}

// This is the default assert_halt() that implements the app mode assert
// behavior: wipe the calling app's .data, .bss, and stack sections, one
// by one, before halting the CPU with an illegal instruction.
//
// The section bounds come from the markers defined by our app.lds:
// _sdata, _edata, _sbss, _ebss, _sstack and _estack.
//
// This definition is weak so a program running with higher privilege,
// such as the TKey firmware executing in firmware mode, can replace
// it with a strong definition that also wipes memory not available
// in app mode. For an example see fw/tk1/assert.c in tillitis-key1
// which wipes all of app RAM and the full FW_RAM.
//
// noinline stops the linker from inlining a copy of this function
// into callers.
__attribute__((weak, noinline)) void assert_halt(void)
{
	// The wipe loops only use registers, never the stack they
	// are about to clear, and this code lives in .text which is
	// left intact.

	// Wipe .data
	// clang-format off
	asm volatile(
		"la a0, _sdata;"
		"la a1, _edata;"
		"loop_wipe_data:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop_wipe_data;"
		:
		:
		: "a0", "a1", "memory");
	// clang-format on

	// Wipe .bss
	// clang-format off
	asm volatile(
		"la a0, _sbss;"
		"la a1, _ebss;"
		"loop_wipe_bss:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop_wipe_bss;"
		:
		:
		: "a0", "a1", "memory");
	// clang-format on

	// Wipe the stack, from the bottom of the stack to the
	// initial stack pointer.
	// clang-format off
	asm volatile(
		"la a0, _sstack;"
		"la a1, _estack;"
		"loop_wipe_stack:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop_wipe_stack;"
		:
		:
		: "a0", "a1", "memory");
	// clang-format on

	// Force illegal instruction to halt CPU
	asm volatile("unimp");

	// Not reached
	__builtin_unreachable();
}
