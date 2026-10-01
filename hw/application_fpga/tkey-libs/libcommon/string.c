// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-FileCopyrightText: 2025 Glasklar Teknik AB <glasklarteknik.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <string.h>

size_t strlen(const char *str)
{
	const char *s;

	for (s = str; *s; ++s)
		;

	return (s - str);
}

int strcmp(const char *a, const char *b)
{
	while (*a == *b && *a != '\0') {
		a++;
		b++;
	}
	return (int)a - (int)b;
}

void *memset(void *dest, int c, size_t n)
{
	uint8_t *s = dest;

	for (; n; n--, s++)
		*s = c;

	return dest;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	const uint8_t *src_byte = src;
	uint8_t *dest_byte = dest;

	for (size_t i = 0; i < n; i++) {
		dest_byte[i] = src_byte[i];
	}

	return dest;
}

int memcmp(const void *a, const void *b, size_t n)
{
	const uint8_t *a_byte = a;
	const uint8_t *b_byte = b;
	size_t i;

	for (i = 0; i < n; i++) {
		int d = a_byte[i] - b_byte[i];
		if (d != 0)
			return d;
	}
	return 0;
}

void *memchr(const void *p, int c, size_t n)
{
	const uint8_t *s = p;
	for (size_t i = 0; i < n; i++)
		if (s[i] == (uint8_t)c)
			return (void *)&s[i];
	return NULL;
}
