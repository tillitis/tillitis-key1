// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

void memcpy_s(void *dest, size_t destsize, const void *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert(destsize >= n);

	const uint8_t *src_byte = (const uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;

	for (size_t i = 0; i < n; i++) {
		dest_byte[i] = src_byte[i];
	}
}

__attribute__((used)) void *wordcpy(void *dest, const void *src, unsigned n)
{
	const uint32_t *src_word = (const uint32_t *)src;
	uint32_t *dest_word = (uint32_t *)dest;

	for (int i = 0; i < n; i++) {
		dest_word[i] = src_word[i];
	}

	return dest;
}

// Copies n 32-bit words from src to dest.
// destsize specifies the size of dest in 32-bit words.
// Halts if destsize is less than n.
void wordcpy_s(void *dest, size_t destsize, const void *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert(destsize >= n);

	const uint32_t *src_word = (const uint32_t *)src;
	uint32_t *dest_word = (uint32_t *)dest;

	for (size_t i = 0; i < n; i++) {
		dest_word[i] = src_word[i];
	}
}

// Copies n 32-bit words from src to dest.
// destsize specifies the size of dest in 32-bit words.
// Halts if destsize is less than n.
// The data in src and dest are accessed through volatile references.
void wordcpy_s_volatile(volatile void *dest, size_t destsize,
			volatile const void *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert(destsize >= n);

	volatile const uint32_t *src_word = (volatile const uint32_t *)src;
	volatile uint32_t *dest_word = (volatile uint32_t *)dest;

	for (size_t i = 0; i < n; i++) {
		dest_word[i] = src_word[i];
	}
}

int memeq(void *dest, const void *src, size_t n)
{
	const uint8_t *src_byte = (const uint8_t *)src;
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
