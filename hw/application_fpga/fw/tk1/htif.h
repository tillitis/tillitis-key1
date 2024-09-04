/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef HTIF_H
#define HTIF_H

#include <stdint.h>

#ifdef QEMU_CONSOLE
void htif_putc(char ch);
void htif_lf(void);
void htif_puthex(uint8_t c);
void htif_putinthex(const uint32_t n);
void htif_puts(const char *s);
void htif_hexdump(void *buf, int len);
#else
#define htif_putc(ch)
#define htif_lf(void)
#define htif_puthex(c)
#define htif_putinthex(n)
#define htif_puts(s)
#define htif_hexdump(buf, len)
#endif // #ifdef NOCONSOLE

#endif
