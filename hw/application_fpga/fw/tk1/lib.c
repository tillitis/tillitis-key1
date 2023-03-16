/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lib.h"
#include "assert.h"
#include "types.h"

#ifndef NOCONSOLE
struct {
	uint32_t arr[2];
} volatile tohost __attribute__((section(".htif")));
struct {
	uint32_t arr[2];
} volatile fromhost __attribute__((section(".htif")));

static void htif_send(uint8_t dev, uint8_t cmd, int64_t data)
{
	/* endian neutral encoding with ordered 32-bit writes */
	union {
		uint32_t arr[2];
		uint64_t val;
	} encode = {.val = (uint64_t)dev << 56 | (uint64_t)cmd << 48 | data};
	tohost.arr[0] = encode.arr[0];
	tohost.arr[1] = encode.arr[1];
}

static void htif_set_tohost(uint8_t dev, uint8_t cmd, int64_t data)
{
	/* send data with specified device and command */
	while (tohost.arr[0]) {
#ifndef S_SPLINT_S
		asm volatile("" : : "r"(fromhost.arr[0]));
		asm volatile("" : : "r"(fromhost.arr[1]));
#endif
	}
	htif_send(dev, cmd, data);
}

static int htif_putchar(int ch)
{
	htif_set_tohost(1, 1, ch & 0xff);
	return ch & 0xff;
}

int htif_puts(const char *s)
{
	while (*s)
		htif_putchar(*s++);
	return 0;
}

void htif_hexdump(void *buf, int len)
{
	uint8_t *byte_buf = (uint8_t *)buf;

	for (int i = 0; i < len; i++) {
		htif_puthex(byte_buf[i]);
		if (i % 2 == 1) {
			(void)htif_putchar(' ');
		}

		if (i != 1 && i % 16 == 1) {
			htif_lf();
		}
	}

	htif_lf();
}

void htif_putc(int ch)
{
	htif_putchar(ch);
}

void htif_lf()
{
	htif_putchar('\n');
}

void htif_puthex(uint8_t c)
{
	unsigned int upper = (c >> 4) & 0xf;
	unsigned int lower = c & 0xf;

	htif_putchar(upper < 10 ? '0' + upper : 'a' - 10 + upper);
	htif_putchar(lower < 10 ? '0' + lower : 'a' - 10 + lower);
}

void htif_putinthex(const uint32_t n)
{
	uint8_t *buf = (uint8_t *)&n;

	htif_puts("0x");
	for (int i = 3; i > -1; i--) {
		htif_puthex(buf[i]);
	}
}
#endif

void *memset(void *dest, int c, unsigned n)
{
	uint8_t *s = dest;

	for (; n; n--, s++)
		*s = c;

	return dest;
}

void memcpy_s(void *dest, size_t destsize, const void *src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	assert(destsize >= n);

	uint8_t *src_byte = (uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;

	for (int i = 0; i < n; i++) {
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

	for (int i = 0; i < n; i++) {
		dest_word[i] = src_word[i];
	}
}

int memeq(void *dest, const void *src, unsigned n)
{
	uint8_t *src_byte = (uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;

	for (int i = 0; i < n; i++) {
		if (dest_byte[i] != src_byte[i]) {
			return 0;
		}
	}

	return -1;
}
