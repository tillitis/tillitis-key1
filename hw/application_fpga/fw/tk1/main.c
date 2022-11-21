/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1_mem.h"
#include "blake2s/blake2s.h"
#include "lib.h"
#include "proto.h"
#include "types.h"

// clang-format off
static volatile uint32_t *uds =        (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
static volatile uint32_t *switch_app = (volatile uint32_t *)TK1_MMIO_TK1_SWITCH_APP;
static volatile uint32_t *name0 =      (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
static volatile uint32_t *name1 =      (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
static volatile uint32_t *ver =        (volatile uint32_t *)TK1_MMIO_TK1_VERSION;
static volatile uint32_t *udi =        (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
static volatile uint32_t *cdi =        (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
static volatile uint32_t *app_addr =   (volatile uint32_t *)TK1_MMIO_TK1_APP_ADDR;
static volatile uint32_t *app_size =   (volatile uint32_t *)TK1_MMIO_TK1_APP_SIZE;
static volatile uint8_t  *fw_ram =     (volatile uint8_t  *)TK1_MMIO_FW_RAM_BASE;

#define LED_RED   (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE  (1 << TK1_MMIO_TK1_LED_B_BIT)
#define LED_WHITE (LED_RED | LED_GREEN | LED_BLUE)
// clang-format on

static void print_hw_version(uint32_t name0, uint32_t name1, uint32_t ver)
{
	puts("Hello, I'm ");
	putc(name0 >> 24);
	putc(name0 >> 16);
	putc(name0 >> 8);
	putc(name0);

	putc('-');

	putc(name1 >> 24);
	putc(name1 >> 16);
	putc(name1 >> 8);
	putc(name1);

	putc(' ');
	putinthex(ver);
	lf();
}

static void print_digest(uint8_t *md)
{
	puts("The app digest:\n");
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 8; i++) {
			puthex(md[i + 8 * j]);
		}
		lf();
	}
	lf();
}

// CDI = blake2s(uds, blake2s(app), uss)
static void compute_cdi(uint8_t digest[32], uint8_t uss[32])
{
	uint32_t local_cdi[8];

	// To protect UDS we use a special firmware-only RAM for both
	// the in parameter to blake2s and the blake2s context.

	// Only word aligned access to UDS
	wordcpy((void *)fw_ram, (void *)uds, 8);
	memcpy((void *)fw_ram + 32, digest, 32);
	memcpy((void *)fw_ram + 64, uss, 32);

	blake2s_ctx *secure_ctx = (blake2s_ctx *)(fw_ram + 96);

	blake2s((void *)local_cdi, 32, NULL, 0, (const void *)fw_ram, 96,
		secure_ctx);

	// Write over the firmware-only RAM
	memset((void *)fw_ram, 0, TK1_MMIO_FW_RAM_SIZE);

	// Only word aligned access to CDI
	wordcpy((void *)cdi, (void *)local_cdi, 8);
}

int main()
{
	uint32_t local_name0 = *name0;
	uint32_t local_name1 = *name1;
	uint32_t local_ver = *ver;
	struct frame_header hdr; // Used in both directions
	uint8_t cmd[CMDLEN_MAXBYTES];
	uint8_t rsp[CMDLEN_MAXBYTES];
	uint8_t *loadaddr = (uint8_t *)TK1_APP_ADDR;
	int left = 0; // Bytes left to receive
	uint8_t uss[32] = {0};
	uint8_t digest[32] = {0};

	print_hw_version(local_name0, local_name1, local_ver);

	for (;;) {
		// blocking; fw flashing white while waiting for cmd
		uint8_t in = readbyte_ledflash(LED_WHITE, 800000);

		if (parseframe(in, &hdr) == -1) {
			puts("Couldn't parse header\n");
			continue;
		}

		memset(cmd, 0, CMDLEN_MAXBYTES);
		// Read firmware command: Blocks!
		read(cmd, hdr.len);

		// Is it for us?
		if (hdr.endpoint != DST_FW) {
			puts("Message not meant for us\n");
			continue;
		}

		// Reset response buffer
		memset(rsp, 0, CMDLEN_MAXBYTES);

		// Min length is 1 byte so this should always be here
		switch (cmd[0]) {
		case FW_CMD_NAME_VERSION:
			puts("cmd: name-version\n");

			if (hdr.len != 1) {
				// Bad length - give them an empty response
				fwreply(hdr, FW_RSP_NAME_VERSION, rsp);
				break;
			}

			memcpy(rsp, (uint8_t *)&local_name0, 4);
			memcpy(rsp + 4, (uint8_t *)&local_name1, 4);
			memcpy(rsp + 8, (uint8_t *)&local_ver, 4);

			fwreply(hdr, FW_RSP_NAME_VERSION, rsp);
			break;

		case FW_CMD_GET_UDI:
			puts("FW_CMD_GET_UDI\n");
			if (hdr.len != 1) {
				// Bad cmd length
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_GET_UDI, rsp);
				break;
			}
			rsp[0] = STATUS_OK;
			uint32_t udi_words[2];
			wordcpy(udi_words, (void *)udi, 2);
			memcpy(rsp + 1, udi_words, 2 * 4);
			fwreply(hdr, FW_RSP_GET_UDI, rsp);
			break;

		case FW_CMD_LOAD_USS:
			puts("cmd: load-uss\n");

			if (hdr.len != 128) {
				// Bad cmd length
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_USS, rsp);
				break;
			}

			memcpy(uss, cmd + 1, 32);

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_USS, rsp);
			break;

		case FW_CMD_LOAD_APP_SIZE:
			puts("cmd: load-app-size\n");

			if (hdr.len != 32) {
				// Bad length
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP_SIZE, rsp);
				break;
			}

			// cmd[1..4] contains the size.
			uint32_t local_app_size = cmd[1] + (cmd[2] << 8) +
						  (cmd[3] << 16) +
						  (cmd[4] << 24);

			puts("app size: ");
			putinthex(local_app_size);
			lf();

			if (local_app_size > TK1_APP_MAX_SIZE) {
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP_SIZE, rsp);
				break;
			}

			*app_size = local_app_size;
			*app_addr = 0;
			// Clear digest as GET_APP_DIGEST returns it even if it
			// has not been calculated
			memset(digest, 0, 32);

			// Reset where to start loading the program
			loadaddr = (uint8_t *)TK1_APP_ADDR;
			left = *app_size;

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_APP_SIZE, rsp);
			break;

		case FW_CMD_LOAD_APP_DATA:
			puts("cmd: load-app-data\n");

			if (hdr.len != 128 || *app_size == 0 ||
			    *app_addr != 0) {
				// Bad length, or app_size not yet set, or
				// app_addr already set (fully loaded!)
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP_DATA, rsp);
				break;
			}

			int nbytes;
			if (left > 127) {
				nbytes = 127;
			} else {
				nbytes = left;
			}
			memcpy(loadaddr, cmd + 1, nbytes);
			loadaddr += nbytes;
			left -= nbytes;

			if (left == 0) {
				puts("Fully loaded ");
				putinthex(*app_size);
				lf();

				*app_addr = TK1_APP_ADDR;
				// Get the Blake2S digest of the app - store it
				// for later queries
				blake2s_ctx ctx;

				blake2s(digest, 32, NULL, 0,
					(const void *)*app_addr, *app_size,
					&ctx);
				print_digest(digest);

				// CDI = hash(uds, hash(app), uss)
				compute_cdi(digest, uss);
			}

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_APP_DATA, rsp);
			break;

		case FW_CMD_RUN_APP:
			puts("cmd: run-app\n");

			if (hdr.len != 1 || *app_size == 0 || *app_addr == 0) {
				// Bad cmd length, or app_size or app_addr are
				// not yet set
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_RUN_APP, rsp);
				break;
			}

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_RUN_APP, rsp);

			// Flip over to application mode
			*switch_app = 1;

			// Jump to app - doesn't return
			// First clears memory of firmware remains
			puts("Jumping to ");
			putinthex(*app_addr);
			lf();
			// clang-format off
			asm volatile(
				// Clear the stack
				"li a0, 0x40000000;" // TK1_RAM_BASE
				"li a1, 0x40007000;" // TK1_APP_ADDR
				"loop:;"
				"sw zero, 0(a0);"
				"addi a0, a0, 4;"
				"blt a0, a1, loop;"
				// Get value at TK1_MMIO_TK1_APP_ADDR
				"lui a0,0xff000;"
				"lw a0,0x030(a0);"
				"jalr x0,0(a0);"
				::: "memory");
			// clang-format on
			break; // This is never reached!

		case FW_CMD_GET_APP_DIGEST:
			puts("cmd: get-app-digest\n");

			memcpy(rsp, &digest, 32);
			fwreply(hdr, FW_RSP_GET_APP_DIGEST, rsp);
			break;

		default:
			puts("Got unknown firmware cmd: 0x");
			puthex(cmd[0]);
			lf();
		}
	}

	return (int)0xcafebabe;
}
