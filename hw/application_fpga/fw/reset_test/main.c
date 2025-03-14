/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "../tk1/proto.h"
#include "../tk1/syscall_num.h"
#include "../tk1/resetinfo.h"
#include "../tk1/syscall.h"
#include "../tk1/syscall_num.h"

// Converts a single hex character to its integer value
static uint8_t hex_char_to_byte(uint8_t c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return 0; // Invalid character, should not happen if input is valid
}

// Converts a 64-char hex string into a 32-byte array
int hex_string_to_bytes(uint8_t *hex_str, uint8_t *out_bytes, size_t out_len)
{
	if (!hex_str || !out_bytes || out_len < 32)
		return -1; // Error handling

	for (size_t i = 0; i < 32; i++) {
		out_bytes[i] = (hex_char_to_byte(hex_str[i * 2]) << 4) | hex_char_to_byte(hex_str[i * 2 + 1]);
	}

	return 0; // Success
}
//---------------------------------------

#define BUFSIZE 32

int main(void)
{
	uint8_t available = 0;
	uint8_t cmdbuf[BUFSIZE] = { 0 };
	enum ioend endpoint = IO_NONE;
	led_set(LED_BLUE);

	while(1) {

		debug_puts("Waiting for command\n");

		memset(cmdbuf, 0, BUFSIZE);

		// Wait for data
		if (readselect(IO_TKEYCTRL, &endpoint, &available) < 0) {
			assert(1 == 2);
		}

		if (read(IO_TKEYCTRL, cmdbuf, BUFSIZE, available) < 0) {
			// read failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		switch (cmdbuf[0]) {
			case '1': {
				syscall(TK1_SYSCALL_RESET, 0);
			}
			break;

			case '2': {
				struct reset rst = { 0 };
				rst.type = LOAD_APP_FROM_HOST;
				syscall(TK1_SYSCALL_RESET_WITH_INTENT, (uint32_t)&rst);
			}
			break;

			case '3': {
				struct reset rst = { 0 };
				rst.type = LOAD_APP_FROM_FLASH;
				syscall(TK1_SYSCALL_RESET_WITH_INTENT, (uint32_t)&rst);
			}
			break;

			case '4': {
				struct reset rst = { 0 };
				uint8_t string[] = "83da11b65f9c3721879bc4d9cffa6eac2368dcd9562aedde4002e6108ac939b3";
				rst.type = LOAD_APP_FROM_HOST_WITH_DIGEST;
				hex_string_to_bytes(string, (uint8_t *)&rst.app_digest, sizeof(rst.app_digest));
				syscall(TK1_SYSCALL_RESET_WITH_INTENT, (uint32_t)&rst);
			}
			break;

			case '5': {
				struct reset rst = { 0 };
				uint8_t string[] = "ef1337a922945fd87683b71ed275e02af44b3489057a29d14fd78daff8b73a28";
				rst.type = LOAD_APP_FROM_HOST_WITH_DIGEST;
				hex_string_to_bytes(string, (uint8_t *)&rst.app_digest, sizeof(rst.app_digest));
				syscall(TK1_SYSCALL_RESET_WITH_INTENT, (uint32_t)&rst);
			}
			break;

			default: {
			}
			break;
		}
	}
}
