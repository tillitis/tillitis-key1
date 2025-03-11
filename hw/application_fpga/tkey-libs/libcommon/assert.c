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

	// Force illegal instruction to halt CPU
	asm volatile("unimp");

	// Not reached
	__builtin_unreachable();
}

void assert_halt(void)
{
	// Force illegal instruction to halt CPU
	asm volatile("unimp");

	// Not reached
	__builtin_unreachable();
}
