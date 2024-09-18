/*
 * Copyright (C) 2022-2024 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void *memset(void *dest, int c, unsigned n);
void memcpy_s(void *dest, size_t destsize, const void *src, size_t n);
void wordcpy_s(void *dest, size_t destsize, const void *src, size_t n);
bool memeq(void *dest, const void *src, size_t n);
void secure_wipe(void *v, size_t n);

#endif
