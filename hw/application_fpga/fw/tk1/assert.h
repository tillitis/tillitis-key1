/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ASSERT_H
#define ASSERT_H

#define assert(expr)                                                           \
	((expr) ? (void)(0)                                                    \
		: __assert_fail(#expr, __FILE__, __LINE__, __func__))

void __assert_fail(const char *__assertion, const char *__file,
		   unsigned int __line, const char *__function);

#endif
