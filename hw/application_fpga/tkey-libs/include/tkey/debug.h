// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_DEBUG_H
#define TKEY_DEBUG_H

#include <stdint.h>

#include "io.h"

#if defined(QEMU_DEBUG)
#define debug_putchar(ch) putchar(IO_QEMU, ch)
#define debug_lf() putchar(IO_QEMU, '\n')
#define debug_putinthex(ch) putinthex(IO_QEMU, ch)
#define debug_puts(s) puts(IO_QEMU, s)
#define debug_puthex(ch) puthex(IO_QEMU, ch)
#define debug_hexdump(buf, len) hexdump(IO_QEMU, buf, len)

#elif defined(TKEY_DEBUG)

#define debug_putchar(ch) putchar(IO_DEBUG, ch)
#define debug_lf() putchar(IO_DEBUG, '\n')
#define debug_putinthex(ch) putinthex(IO_DEBUG, ch)
#define debug_puts(s) puts(IO_DEBUG, s)
#define debug_puthex(ch) puthex(IO_DEBUG, ch)
#define debug_hexdump(buf, len) hexdump(IO_DEBUG, buf, len)

#else

#define debug_putchar(ch)
#define debug_lf()
#define debug_putinthex(n)
#define debug_puts(s)
#define debug_puthex(ch)
#define debug_hexdump(buf, len)

#endif

#endif
