/*
 * Copyright (C) 2022-2024 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lib.h"
#include "assert.h"
#include <stddef.h>
#include <stdint.h>

void *memset(void *dest, int c, unsigned n)
{
	uint8_t *s = dest;

	for (; n; n--, s++)
		*s = (uint8_t)c;

	/*@ -temptrans @*/
	return dest;
}

void memcpy_s(void *dest, size_t destsize, const void *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert(destsize >= n);

	uint8_t *src_byte = (uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;

	for (size_t i = 0; i < n; i++) {
		/*@ -nullderef @*/
		/* splint complains that dest_byte and src_byte can be
		 * NULL, but it seems it doesn't understand assert.
		 * See above.
		 */
		dest_byte[i] = src_byte[i];
	}
}

void wordcpy_s(void *dest, size_t destsize, const void *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert(destsize >= n);

	uint32_t *src_word = (uint32_t *)src;
	uint32_t *dest_word = (uint32_t *)dest;

	for (size_t i = 0; i < n; i++) {
		dest_word[i] = src_word[i];
	}
}

int memeq(void *dest, const void *src, size_t n)
{
	uint8_t *src_byte = (uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;
	int res = -1;

	for (size_t i = 0; i < n; i++) {
		if (dest_byte[i] != src_byte[i]) {
			res = 0;
		}
	}

	return res;
}

void secure_wipe(void *v, size_t n)
{
	volatile uint8_t *p = (volatile uint8_t *)v;
	while (n--)
		*p++ = 0;
}
