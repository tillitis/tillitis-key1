/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "assert.h"
#include "htif.h"
#include "lib.h"

void assert_fail(const char *assertion, const char *file, unsigned int line,
		 const char *function)
{
	htif_puts("assert: ");
	htif_puts(assertion);
	htif_puts(" ");
	htif_puts(file);
	htif_puts(":");
	htif_putinthex(line);
	htif_puts(" ");
	htif_puts(function);
	htif_lf();

#ifndef S_SPLINT_S
	// Force illegal instruction to halt CPU
	asm volatile("unimp");
#endif

	// Not reached
	__builtin_unreachable();
}
