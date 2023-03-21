/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ASSERT_H
#define ASSERT_H

#define assert(expr)                                                           \
	((expr) ? (void)(0) : assert_fail(#expr, __FILE__, __LINE__, __func__))

void assert_fail(const char *assertion, const char *file, unsigned int line,
		 const char *function);

#endif
