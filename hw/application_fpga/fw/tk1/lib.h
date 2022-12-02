/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LIB_H
#define LIB_H

#include "types.h"

void htif_putc(int ch);
void htif_lf();
void htif_puthex(uint8_t c);
void htif_putinthex(const uint32_t n);
int htif_puts(const char *s);
void htif_hexdump(uint8_t *buf, int len);
void *memset(void *dest, int c, unsigned n);
void *memcpy(void *dest, const void *src, unsigned n);
void *wordcpy(void *dest, const void *src, unsigned n);
int memeq(void *dest, const void *src, unsigned n);

#endif
