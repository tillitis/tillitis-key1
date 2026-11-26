// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_ASSERT_H
#define TKEY_ASSERT_H

#include <tkey/io.h>

/*
 * Assert behavior:
 *
 * On a failing assert the CPU is halted with an illegal instruction.
 * Before halting, the memory of the program taking the assert is
 * wiped.
 *
 * The default assert_halt() in libcommon implements the app mode
 * behavior: it wipes the calling app's .data, .bss, and stack
 * sections, one section at a time. The bounds are the section
 * markers _sdata, _edata, _sbss, _ebss, _sstack and _estack,
 * defined by our app.lds: the stack spans from the end of .bss
 * to just below the end of RAM. If you link your device app
 * with a custom linker script you must define these symbols
 * yourself.
 *
 * A program running with higher privilege, such as the TKey
 * firmware executing in firmware mode, can override assert_halt()
 * with a strong definition that also wipes memory that is not
 * accessible in app mode. For an example see fw/tk1/assert.c in
 * tillitis-key1, which clears all of app RAM and the full FW_RAM.
 *
 * assert_fail() calls assert_halt() after printing information
 * about the failing assert.
 */

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
		 unsigned int line, const char *function)
    __attribute__((noreturn));
void assert_halt(void) __attribute__((noreturn));
#endif
