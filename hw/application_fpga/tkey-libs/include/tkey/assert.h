// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_ASSERT_H
#define TKEY_ASSERT_H

#include <tkey/io.h>

#if defined(QEMU_DEBUG)
#define assert(expr)                                                           \
	((expr) ? (void)(0)                                                    \
		: assert_fail(IO_QEMU, #expr, __FILE__, __LINE__, __func__))

#elif defined(TKEY_DEBUG)

#define assert(expr)                                                           \
	((expr) ? (void)(0)                                                    \
		: assert_fail(IO_DEBUG, #expr, __FILE__, __LINE__, __func__))

#else

#define assert(expr) ((expr) ? (void)(0) : assert_halt())
#endif

void assert_fail(enum ioend dest, const char *assertion, const char *file,
		 unsigned int line, const char *function);
void assert_halt(void);
#endif
