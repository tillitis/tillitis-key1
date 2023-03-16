/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LIB_H
#define LIB_H

#include "types.h"

#ifdef NOCONSOLE
#define htif_putc(ch)
#define htif_lf()
#define htif_puthex(c)
#define htif_putinthex(n)
#define htif_puts(s) ((int)0)
#define htif_hexdump(buf, len)
#else
void htif_putc(int ch);
void htif_lf();
void htif_puthex(uint8_t c);
void htif_putinthex(const uint32_t n);
int htif_puts(const char *s);
void htif_hexdump(void *buf, int len);
#endif

void *memset(void *dest, int c, unsigned n);
void memcpy_s(void *dest, size_t destsize, const void *src, size_t n);
void wordcpy_s(void *dest, size_t destsize, const void *src, size_t n);
int memeq(void *dest, const void *src, unsigned n);

#endif
