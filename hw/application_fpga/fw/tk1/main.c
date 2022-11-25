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
static volatile uint32_t *led =        (volatile uint32_t *)TK1_MMIO_TK1_LED;

#define LED_RED   (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE  (1 << TK1_MMIO_TK1_LED_B_BIT)
#define LED_WHITE (LED_RED | LED_GREEN | LED_BLUE)
// clang-format on

struct namever {
	char name0[4];
	char name1[4];
	uint32_t version;
};

static void print_hw_version(struct namever namever)
{
	puts("Hello, I'm ");
	hexdump((uint8_t *)&namever.name0, 4);
	putc(namever.name0[0]);
	putc(namever.name0[1]);
	putc(namever.name0[2]);
	putc(namever.name0[3]);
	putc('-');
	putc(namever.name1[0]);
	putc(namever.name1[1]);
	putc(namever.name1[2]);
	putc(namever.name1[3]);
	putc(':');
	putinthex(namever.version);
	lf();
}

static struct namever get_hw_version(uint32_t name0, uint32_t name1,
				     uint32_t ver)
{
	struct namever namever;

	hexdump((uint8_t *)&name0, 4);
	putinthex(name0);
	lf();

	namever.name0[0] = name0 >> 24;
	namever.name0[1] = name0 >> 16;
	namever.name0[2] = name0 >> 8;
	namever.name0[3] = name0;

	namever.name1[0] = name1 >> 24;
	namever.name1[1] = name1 >> 16;
	namever.name1[2] = name1 >> 8;
	namever.name1[3] = name1;

	namever.version = ver;

	return namever;
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
static void compute_cdi(uint8_t digest[32], uint8_t use_uss, uint8_t uss[32])
{
	uint32_t local_cdi[8];
	int len;

	// To protect UDS we use a special firmware-only RAM for both
	// the in parameter to blake2s and the blake2s context.

	// Only word aligned access to UDS
	wordcpy((void *)fw_ram, (void *)uds, 8);
	memcpy((void *)fw_ram + 32, digest, 32);
	if (use_uss != 0) {
		memcpy((void *)fw_ram + 64, uss, 32);
		len = 96;
	} else {
		len = 64;
	}

	blake2s_ctx *secure_ctx = (blake2s_ctx *)(fw_ram + len);

	blake2s((void *)local_cdi, 32, NULL, 0, (const void *)fw_ram, len,
		secure_ctx);

	// Write over the firmware-only RAM
	memset((void *)fw_ram, 0, TK1_MMIO_FW_RAM_SIZE);

	// Only word aligned access to CDI
	wordcpy((void *)cdi, (void *)local_cdi, 8);
}

void forever_redflash()
{
	int led_on = 0;
	for (;;) {
		*led = led_on ? LED_RED : 0;
		for (volatile int i = 0; i < 800000; i++) {
		}
		led_on = !led_on;
	}
}

enum state {
	FW_STATE_INITIAL,
	FW_STATE_INIT_LOADING,
	FW_STATE_LOADING,
	FW_STATE_RUN
};

int main()
{
	struct namever namever = get_hw_version(*name0, *name1, *ver);
	struct frame_header hdr; // Used in both directions
	uint8_t cmd[CMDLEN_MAXBYTES];
	uint8_t rsp[CMDLEN_MAXBYTES];
	uint8_t *loadaddr = (uint8_t *)TK1_APP_ADDR;
	int left = 0; // Bytes left to receive
	uint8_t use_uss = FALSE;
	uint8_t uss[32] = {0};
	uint8_t digest[32] = {0};
	enum state state = FW_STATE_INITIAL;

	print_hw_version(namever);

	for (;;) {
		switch (state) {
		case FW_STATE_INITIAL:
			break;

		case FW_STATE_INIT_LOADING:
			*app_addr = 0;
			left = *app_size;

			// Reset where to start loading the program
			loadaddr = (uint8_t *)TK1_APP_ADDR;
			break;

		case FW_STATE_LOADING:
			break;

		case FW_STATE_RUN:
			*app_addr = TK1_APP_ADDR;

			// CDI = hash(uds, hash(app), uss)
			compute_cdi(digest, use_uss, uss);

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

		default:
			puts("Unknown firmware state 0x");
			puthex(state);
			lf();
			forever_redflash();
			break; // Not reached
		}

		uint8_t in;
		if (state == FW_STATE_LOADING) {
			*led = LED_WHITE;
			in = readbyte();
		} else {
			in = readbyte_ledflash(LED_WHITE, 800000);
		}

		if (parseframe(in, &hdr) == -1) {
			puts("Couldn't parse header\n");
			continue;
		}

		memset(cmd, 0, CMDLEN_MAXBYTES);
		// Now we know the size of the cmd frame, read it all
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

			memcpy(rsp, &namever.name0, 4);
			memcpy(rsp + 4, &namever.name1, 4);
			memcpy(rsp + 8, &namever.version, 4);
			fwreply(hdr, FW_RSP_NAME_VERSION, rsp);
			// state unchanged
			break;

		case FW_CMD_GET_UDI:
			puts("cmd: get-udi\n");
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
			// state unchanged
			break;

		case FW_CMD_LOAD_APP:
			puts("cmd: load-app(size, uss)\n");
			if (hdr.len != 128) {
				// Bad length
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP, rsp);
				break;
			}

			// cmd[1..4] contains the size.
			uint32_t local_app_size = cmd[1] + (cmd[2] << 8) +
						  (cmd[3] << 16) +
						  (cmd[4] << 24);

			puts("app size: ");
			putinthex(local_app_size);
			lf();

			if (local_app_size == 0 ||
			    local_app_size > TK1_APP_MAX_SIZE) {
				rsp[0] = STATUS_BAD;
				fwreply(hdr, FW_RSP_LOAD_APP, rsp);
				break;
			}

			*app_size = local_app_size;

			// Do we have a USS at all?
			if (cmd[5] != 0) {
				// Yes
				use_uss = TRUE;
				memcpy(uss, cmd + 6, 32);
			} else {
				use_uss = FALSE;
			}

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_APP, rsp);

			state = FW_STATE_INIT_LOADING;
			break;

		case FW_CMD_LOAD_APP_DATA:
			puts("cmd: load-app-data\n");
			if (hdr.len != 128 || (state != FW_STATE_INIT_LOADING &&
					       state != FW_STATE_LOADING)) {
				// Bad cmd length or state
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

				// Compute Blake2S digest of the app, storing
				// it for FW_STATE_RUN
				blake2s_ctx ctx;

				blake2s(digest, 32, NULL, 0,
					(const void *)TK1_APP_ADDR, *app_size,
					&ctx);
				print_digest(digest);

				// And return the digest in final response
				rsp[0] = STATUS_OK;
				memcpy(&rsp[1], &digest, 32);
				fwreply(hdr, FW_RSP_LOAD_APP_DATA_READY, rsp);

				state = FW_STATE_RUN;
				break;
			}

			rsp[0] = STATUS_OK;
			fwreply(hdr, FW_RSP_LOAD_APP_DATA, rsp);

			state = FW_STATE_LOADING;
			break;

		default:
			puts("Got unknown firmware cmd: 0x");
			puthex(cmd[0]);
			lf();
		}
	}

	return (int)0xcafebabe;
}
