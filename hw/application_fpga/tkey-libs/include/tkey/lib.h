// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_LIB_H
#define TKEY_LIB_H

#include <stddef.h>
#include <stdint.h>

void *memset(void *dest, int c, unsigned n);
void *memcpy(void *dest, const void *src, unsigned n);
void memcpy_s(void *dest, size_t destsize, const void *src, size_t n);
void *wordcpy(void *dest, const void *src, unsigned n);
void wordcpy_s(void *dest, size_t destsize, const void *src, size_t n);
int memeq(void *dest, const void *src, size_t n);
void secure_wipe(void *v, size_t n);
size_t strlen(const char *str);
#endif
