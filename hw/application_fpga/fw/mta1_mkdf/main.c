/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../mta1_mkdf_mem.h"
#include "blake2s/blake2s.h"
#include "lib.h"
#include "proto.h"
#include "types.h"

// In RAM + above the stack (0x40010000)
#define APP_RAM_ADDR (MTA1_MKDF_RAM_BASE + 0x10000)
#define APP_MAX_SIZE 65536

// clang-format off
static volatile uint32_t *uds =        (volatile uint32_t *)MTA1_MKDF_MMIO_UDS_FIRST;
static volatile uint32_t *switch_app = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_SWITCH_APP;
static volatile uint32_t *name0 =      (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_NAME0;
static volatile uint32_t *name1 =      (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_NAME1;
static volatile uint32_t *ver =        (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_VERSION;
static volatile uint32_t *cdi =        (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_CDI_FIRST;
static volatile uint32_t *app_addr =   (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_APP_ADDR;
static volatile uint32_t *app_size =   (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_APP_SIZE;

#define LED_RED (1 << MTA1_MKDF_MMIO_MTA1_LED_R_BIT)
#define LED_GREEN (1 << MTA1_MKDF_MMIO_MTA1_LED_G_BIT)
#define LED_BLUE (1 << MTA1_MKDF_MMIO_MTA1_LED_B_BIT)

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
// clang-format on

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

int main()
{
	uint32_t local_name0 = *name0;
	uint32_t local_name1 = *name1;
	uint32_t local_ver = *ver;
	struct frame_header hdr; // Used in both directions
	uint8_t cmd[CMDLEN_MAXBYTES];
	uint8_t rsp[CMDLEN_MAXBYTES];
	uint8_t *loadaddr = (uint8_t *)APP_RAM_ADDR;
	int left = 0;	// Bytes left to read
	int nbytes = 0; // Bytes to write to memory
	uint8_t uss[32];
	uint32_t local_app_size = 0;
	uint8_t in;
	uint8_t digest[32];

	print_hw_version(local_name0, local_name1, local_ver);

	// If host does not load USS, we use an all zero USS
	memset(uss, 0, 32);

	for (;;) {
		// blocking; fw flashing white while waiting for cmd
		in = readbyte_ledflash(LED_RED | LED_BLUE | LED_GREEN, 800000);

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
			puts("request: name-version\n");

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

		case FW_CMD_LOAD_USS:
			puts("request: load-uss\n");

			if (hdr.len != 128 || *app_size != 0) {
				// Bad cmd length, or app_size already set
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_USS, rsp);
				break;
			}

			memcpy(uss, cmd + 1, 32);

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_USS, rsp);
			break;

		case FW_CMD_LOAD_APP_SIZE:
			puts("request: load-app-size\n");

			if (hdr.len != 32) {
				// Bad length
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP_SIZE, rsp);
				break;
			}

			// cmd[1..4] contains the size.
			local_app_size = cmd[1] + (cmd[2] << 8) +
					 (cmd[3] << 16) + (cmd[4] << 24);

			puts("app size: ");
			putinthex(local_app_size);
			lf();

			if (local_app_size > APP_MAX_SIZE) {
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP_SIZE, rsp);
				break;
			}

			*app_size = local_app_size;
			*app_addr = 0;

			// Reset where to start loading the program
			loadaddr = (uint8_t *)APP_RAM_ADDR;
			left = *app_size;

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_APP_SIZE, rsp);
			break;

		case FW_CMD_LOAD_APP_DATA:
			puts("request: load-app-data\n");

			if (hdr.len != 128 || *app_size == 0) {
				// Bad length of this command or bad app size -
				// they need to call FW_CMD_LOAD_APP_SIZE first
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP_DATA, rsp);
				break;
			}

			if (left > 127) {
				nbytes = 127;
			} else {
				nbytes = left;
			}
			memcpy(loadaddr, cmd + 1, nbytes);
			loadaddr += nbytes;
			left -= nbytes;

			if (left == 0) {
				uint8_t scratch[96];

				puts("Fully loaded ");
				putinthex(*app_size);
				lf();

				*app_addr = APP_RAM_ADDR;
				// Get the Blake2S digest of the app - store it
				// for later queries
				blake2s(digest, 32, NULL, 0,
					(const void *)*app_addr, *app_size);
				print_digest(digest);

				// CDI = hash(uds, hash(app), uss)
				uint32_t local_cdi[8];

				// Only word aligned access to UDS
				wordcpy(scratch, (void *)uds, 8);
				memcpy(scratch + 32, digest, 32);
				memcpy(scratch + 64, uss, 32);
				blake2s((void *)local_cdi, 32, NULL, 0,
					(const void *)scratch, 96);
				// Only word aligned access to CDI
				wordcpy((void *)cdi, (void *)local_cdi, 8);
			}

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_APP_DATA, rsp);
			break;

		case FW_CMD_RUN_APP:
			puts("request: run-app\n");

			if (hdr.len != 1 || *app_size == 0 || *app_addr == 0) {
				// Bad cmd length, or app_size and app_addr are
				// not both set
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
				"li a0, 0x40000000;" // MTA1_MKDF_RAM_BASE
				"li a1, 0x40010000;"
				"loop:;"
				"sw zero, 0(a0);"
				"addi a0, a0, 4;"
				"blt a0, a1, loop;"
				// Get value at MTA1_MKDF_MMIO_MTA1_APP_ADDR
				"lui a0,0xff000;"
				"lw a0,0x030(a0);"
				"jalr x0,0(a0);"
				::: "memory");
			// clang-format on
			break; // This is never reached!

		case FW_CMD_GET_APP_DIGEST:
			puts("request: get-app-digest\n");

			memcpy(rsp, &digest, 32);
			fwreply(hdr, FW_RSP_GET_APP_DIGEST, rsp);
			break;

		default:
			puts("Received unknown firmware command: 0x");
			puthex(cmd[0]);
			lf();
		}
	}

	return (int)0xcafebabe;
}
