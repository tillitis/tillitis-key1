// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <fw/tk1/proto.h>
#include <fw/tk1/reset.h>
#include <fw/tk1/syscall_num.h>
#include <stdint.h>
#include <syscall.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

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
		out_bytes[i] = (hex_char_to_byte(hex_str[i * 2]) << 4) |
			       hex_char_to_byte(hex_str[i * 2 + 1]);
	}

	return 0; // Success
}
//---------------------------------------

#define BUFSIZE 32

int main(void)
{
	uint8_t available = 0;
	uint8_t cmdbuf[BUFSIZE] = {0};
	enum ioend endpoint = IO_NONE;
	led_set(LED_BLUE);
	struct reset rst = {0};

	while (1) {

		puts(IO_CDC, "reset_test: Waiting for command\r\n");

		memset(cmdbuf, 0, BUFSIZE);

		// Wait for data
		if (readselect(IO_CDC, &endpoint, &available) < 0) {
			assert(1 == 2);
		}

		if (read(IO_CDC, cmdbuf, BUFSIZE, available) < 0) {
			// read failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		led_set(LED_BLUE | LED_RED);

		switch (cmdbuf[0]) {
		case '1':
			// Reset into default state

			rst.type = START_DEFAULT;
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
			break;

		case '2':
			// Reset and load app from client

			rst.type = START_CLIENT;
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
			break;

		case '3':
			// Reset and load app from second flash slot

			rst.type = START_FLASH1;
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
			break;

		case '4': {
			// Reset and load app from client with verification
			// using an invalid digest.
			//
			// Should cause firmware to refuse to start app.

			uint8_t string[] = "0123456789abcdef0123456789abcdef012"
					   "3456789abcdef0123456789abcdef";
			rst.type = START_CLIENT_VER;
			hex_string_to_bytes(string, (uint8_t *)&rst.app_digest,
					    sizeof(rst.app_digest));
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
		} break;

		case '5': {
			// Reset and load app from client with verification
			// using a digest matching the example app (blue.bin)
			// from tkey-libs

			uint8_t tkeylibs_example_app_digest[] =
			    "96bb4c90603dbbbe09b9a1d7259b5e9e61bedd89a897105c30"
			    "c9d4bf66a98d97";
			rst.type = START_CLIENT_VER;
			hex_string_to_bytes(tkeylibs_example_app_digest,
					    (uint8_t *)&rst.app_digest,
					    sizeof(rst.app_digest));
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
		} break;

		case '6': {
			// Reset and load app from second flash slot with
			// verification using an invalid digest.
			//
			// Should cause firmware to refuse to start app.

			uint8_t string[] = "0123456789abcdef0123456789abcdef012"
					   "3456789abcdef0123456789abcdef";
			rst.type = START_FLASH1_VER;
			hex_string_to_bytes(string, (uint8_t *)&rst.app_digest,
					    sizeof(rst.app_digest));
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
		} break;

		case '7': {
			// Reset and load app from second flash slot with
			// verification using a digest matching the example app
			// (blue.bin) from tkey-libs
			//
			// Blue.bin has to be present on flash in the second
			// preloaded app slot (slot 1).

			uint8_t tkeylibs_example_app_digest[] =
			    "96bb4c90603dbbbe09b9a1d7259b5e9e61bedd89a897105c30"
			    "c9d4bf66a98d97";
			rst.type = START_FLASH1_VER;
			hex_string_to_bytes(tkeylibs_example_app_digest,
					    (uint8_t *)&rst.app_digest,
					    sizeof(rst.app_digest));
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
		} break;

		default:
			break;
		}
	}
}
