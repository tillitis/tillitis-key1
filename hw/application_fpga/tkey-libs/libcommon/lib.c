// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

void *memset(void *dest, int c, unsigned n)
{
	uint8_t *s = dest;

	for (; n; n--, s++)
		*s = c;

	return dest;
}

__attribute__((used)) void *memcpy(void *dest, const void *src, unsigned n)
{
	uint8_t *src_byte = (uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;

	for (int i = 0; i < n; i++) {
		dest_byte[i] = src_byte[i];
	}

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
		dest_byte[i] = src_byte[i];
	}
}

__attribute__((used)) void *wordcpy(void *dest, const void *src, unsigned n)
{
	uint32_t *src_word = (uint32_t *)src;
	uint32_t *dest_word = (uint32_t *)dest;

	for (int i = 0; i < n; i++) {
		dest_word[i] = src_word[i];
	}

	return dest;
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

size_t strlen(const char *str)
{
	const char *s;

	for (s = str; *s; ++s)
		;

	return (s - str);
}
