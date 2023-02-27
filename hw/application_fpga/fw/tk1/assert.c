/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "assert.h"
#include "lib.h"

void __assert_fail(const char *__assertion, const char *__file,
		   unsigned int __line, const char *__function)
{
	htif_puts("assert: ");
	htif_puts(__assertion);
	htif_puts(" ");
	htif_puts(__file);
	htif_puts(":");
	htif_putinthex(__line);
	htif_puts(" ");
	htif_puts(__function);
	htif_lf();

	for (;;);
	// Not reached
}
