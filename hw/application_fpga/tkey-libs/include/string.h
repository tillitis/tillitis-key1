// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-FileCopyrightText: 2025 Glasklar Teknik AB <glasklarteknik.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
void *memset(void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);
void *memchr(const void *p, int c, size_t n);

#endif /* STRING_H */
