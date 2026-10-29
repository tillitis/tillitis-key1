// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <blake2s/blake2s.h>
#include <monocypher/monocypher-ed25519.h>
#include <stdint.h>
#include <string.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/syscall.h>
#include <tkey/tk1_mem.h>

#include "blink.h"
#include "syscall.h"

// clang-format off
static volatile uint32_t *cdi           = (volatile uint32_t *) TK1_MMIO_TK1_CDI_FIRST;
// clang-format on

int install_app(uint8_t secret_key[64])
{
	struct app_metadata metadata = {0};
	int ret = 0;

	ret = syscall(TK1_SYSCALL_PRELOAD_DELETE, (uint32_t)&metadata, 0, 0);

	if (ret != 0) {
		puts(IO_CDC, "couldn't delete preloaded app. error: 0x");
		putinthex(IO_CDC, ret);
		puts(IO_CDC, "\r\n");

		return -1;
	}

	ret = syscall(TK1_SYSCALL_PRELOAD_STORE, 0, (uint32_t)blink,
		      sizeof(blink));

	if (ret != 0) {
		puts(IO_CDC, "couldn't store app, error: 0x");
		putinthex(IO_CDC, ret);
		puts(IO_CDC, "\r\n");

		return -1;
	}

	// Update metadata with the new app, and write to set_metadata to
	// finalize. Currently skips metadata.mac
	metadata.size = sizeof(blink);

	puts(IO_CDC, "blink: ");
	putinthex(IO_CDC, (uint32_t)blink);
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "blink[0]: ");
	putinthex(IO_CDC, blink[0]);
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "sizeof(blink): ");
	putinthex(IO_CDC, sizeof(blink));
	puts(IO_CDC, "\r\n");

	if (blake2s(metadata.digest, 32, NULL, 0, blink, sizeof(blink)) != 0) {
		puts(IO_CDC, "couldn't compute digest\r\n");
		return -1;
	}

	crypto_ed25519_sign(metadata.signature, secret_key, metadata.digest,
			    sizeof(metadata.digest));

	puts(IO_CDC, "metadata.digest:\r\n");
	hexdump(IO_CDC, metadata.digest, sizeof(metadata.digest));
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "metadata.signature:\r\n");
	hexdump(IO_CDC, metadata.signature, sizeof(metadata.signature));
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "secret_key:\r\n");
	hexdump(IO_CDC, secret_key, 64);
	puts(IO_CDC, "\r\n");

	ret = syscall(TK1_SYSCALL_PRELOAD_SET_METADATA, (uint32_t)&metadata, 0,
		      0);

	if (ret != 0) {
		puts(IO_CDC, "couldn't finalize storing app, error:");
		putinthex(IO_CDC, ret);
		puts(IO_CDC, "\r\n");

		return -1;
	}

	return 0;
}

int verify(uint8_t pubkey[32])
{
	struct app_metadata metadata = {0};
	int ret = 0;

	// pubkey we already have
	// read signature
	// read digest
	ret = syscall(TK1_SYSCALL_PRELOAD_GET_METADATA, (uint32_t)&metadata, 0,
		      0);

	if (ret != 0) {
		puts(IO_CDC, "couldn't get metadata, error:");
		putinthex(IO_CDC, ret);
		puts(IO_CDC, "\r\n");

		return -1;
	}

	puts(IO_CDC, "metadata.digest:\r\n");
	hexdump(IO_CDC, metadata.digest, sizeof(metadata.digest));
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "metadata.signature:\r\n");
	hexdump(IO_CDC, metadata.signature, sizeof(metadata.signature));
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "metadata.pubkey:\r\n");
	hexdump(IO_CDC, metadata.pubkey, 32);
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "embedded pubkey:\r\n");
	hexdump(IO_CDC, pubkey, 32);
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "Checking signature against embedded pubkey...\r\n");

	if (crypto_ed25519_check(metadata.signature, pubkey, metadata.digest,
				 sizeof(metadata.digest)) != 0) {
		puts(IO_CDC, "signature check failed\r\n");

		return -1;
	}

	puts(IO_CDC, "Resetting into pre loaded app (slot 1)...\r\n");

	// syscall reset flash1_ver with metadata.digest
	struct reset rst;
	rst.type = START_FLASH1_VER;
	memcpy_s(rst.app_digest, sizeof(rst.app_digest), metadata.digest,
		 sizeof(metadata.digest));
	memset(rst.next_app_data, 0, sizeof(rst.next_app_data));

	syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);

	return -2;
}

void reset_from_client(void)
{
	struct reset rst = {0};

	rst.type = START_CLIENT;

	// Give the next in chain something to look at.
	memset(rst.next_app_data, 17, sizeof(rst.next_app_data));

	syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, sizeof(rst.next_app_data),
		0);
}

int main(void)
{
	uint8_t secret_key[64];
	uint8_t pubkey[32];
	enum ioend endpoint;
	uint8_t available;
	uint8_t in = 0;

	led_set(LED_BLUE);

	config_endpoints(IO_CDC);

	// Generate a key pair from CDI
	crypto_ed25519_key_pair(secret_key, pubkey, (uint8_t *)cdi);

	if (readselect(IO_CDC, false, &endpoint, &available) < 0) {
		// readselect failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	if (read(IO_CDC, &in, 1, 1) < 0) {
		// read failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	puts(IO_CDC, "Hello from testloadapp! 0 = install app in slot 1, 1 = "
		     "verify app, 2 == load app from client\r\n");

	for (;;) {
		if (readselect(IO_CDC, false, &endpoint, &available) < 0) {
			// readselect failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		if (read(IO_CDC, &in, 1, 1) < 0) {
			// read failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		switch (in) {
		case '0':
			if (install_app(secret_key) < 0) {
				puts(IO_CDC, "Failed to install app\r\n");
			} else {
				puts(IO_CDC, "Installed app!\r\n");
			}

			break;

		case '1':
			if (verify(pubkey) < 0) {
				puts(IO_CDC, "Failed to verify app\r\n");
			} else {
				puts(IO_CDC, "Verified app!\r\n");
			}

			break;

		case '2':
			reset_from_client();
			break;

		default:
			break;
		}
	}
}
