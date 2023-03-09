/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1_mem.h"
#include "assert.h"
#include "blake2s/blake2s.h"
#include "led.h"
#include "lib.h"
#include "proto.h"
#include "state.h"
#include "types.h"

// clang-format off
static volatile uint32_t *uds             = (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
static volatile uint32_t *switch_app      = (volatile uint32_t *)TK1_MMIO_TK1_SWITCH_APP;
static volatile uint32_t *name0           = (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
static volatile uint32_t *name1           = (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
static volatile uint32_t *ver             = (volatile uint32_t *)TK1_MMIO_TK1_VERSION;
static volatile uint32_t *udi             = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
static volatile uint32_t *cdi             = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
static volatile uint32_t *app_addr        = (volatile uint32_t *)TK1_MMIO_TK1_APP_ADDR;
static volatile uint32_t *app_size        = (volatile uint32_t *)TK1_MMIO_TK1_APP_SIZE;
static volatile uint32_t *fw_blake2s_addr = (volatile uint32_t *)TK1_MMIO_TK1_BLAKE2S;
static volatile uint32_t *trng_status     = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
static volatile uint32_t *trng_entropy    = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
static volatile uint32_t *timer           = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_status    = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
static volatile uint32_t *timer_ctrl      = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
static volatile uint32_t *ram_aslr        = (volatile uint32_t *)TK1_MMIO_TK1_RAM_ASLR;
static volatile uint32_t *ram_scramble    = (volatile uint32_t *)TK1_MMIO_TK1_RAM_SCRAMBLE;
// clang-format on

// Context for the loading of a TKey program
struct context {
	int left;	    // Bytes left to receive
	uint8_t digest[32]; // Program digest
	uint8_t *loadaddr;  // Where we are currently loading a TKey program
	uint8_t use_uss;    // Use USS?
	uint8_t uss[32];    // User Supplied Secret, if any
};

struct namever {
	char name0[4];
	char name1[4];
	uint32_t version;
};

static void print_hw_version(struct namever namever)
{
	htif_puts("Hello, I'm ");
	htif_hexdump((uint8_t *)&namever.name0, 4);
	htif_putc(namever.name0[0]);
	htif_putc(namever.name0[1]);
	htif_putc(namever.name0[2]);
	htif_putc(namever.name0[3]);
	htif_putc('-');
	htif_putc(namever.name1[0]);
	htif_putc(namever.name1[1]);
	htif_putc(namever.name1[2]);
	htif_putc(namever.name1[3]);
	htif_putc(':');
	htif_putinthex(namever.version);
	htif_lf();
}

static struct namever get_hw_version(uint32_t name0, uint32_t name1,
				     uint32_t ver)
{
	struct namever namever;

	htif_hexdump((uint8_t *)&name0, 4);
	htif_putinthex(name0);
	htif_lf();

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
	htif_puts("The app digest:\n");
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 8; i++) {
			htif_puthex(md[i + 8 * j]);
		}
		htif_lf();
	}
	htif_lf();
}

uint32_t rnd_word()
{
	while ((*trng_status & (1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
	}
	return *trng_entropy;
}

// CDI = blake2s(uds, blake2s(app), uss)
static void compute_cdi(const uint8_t digest[32], const uint8_t use_uss,
			const uint8_t uss[32])
{
	uint32_t local_cdi[8];
	blake2s_ctx secure_ctx;
	uint32_t rnd;
	int blake2err;

	// Prepare to sleep a random number of cycles before reading out UDS
	*timer_prescaler = 1;
	rnd = rnd_word();
	// Up to 65536 cycles
	rnd &= 0xffff;
	*timer = (rnd == 0 ? 1 : rnd);
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);
	while (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
	}

	blake2err = blake2s_init(&secure_ctx, 32, NULL, 0);
	assert(blake2err == 0);

	// Update hash with UDS. This means UDS will live for a short
	// while on the firmware stack which is in the special fw_ram.
	blake2s_update(&secure_ctx, (const void *)uds, 32);

	// Update with TKey program digest
	blake2s_update(&secure_ctx, digest, 32);

	// Possibly hash in the USS as well
	if (use_uss != 0) {
		blake2s_update(&secure_ctx, uss, 32);
	}

	// Write hashed result to Compound Device Identity (CDI)
	blake2s_final(&secure_ctx, local_cdi);

	// Clear secure_ctx of any residue
	memset(&secure_ctx, 0, sizeof(secure_ctx));

	// CDI only word writable
	wordcpy_s((void *)cdi, 8, local_cdi, 8);
}

static int initial_commands(const struct frame_header *hdr, const uint8_t *cmd,
			    enum state state, struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};

	switch (cmd[0]) {
	case FW_CMD_NAME_VERSION: {
		struct namever namever = get_hw_version(*name0, *name1, *ver);

		htif_puts("cmd: name-version\n");
		if (hdr->len != 1) {
			// Bad length
			return FW_STATE_FAIL;
		}

		memcpy_s(rsp, CMDLEN_MAXBYTES, &namever.name0, 4);
		memcpy_s(&rsp[4], CMDLEN_MAXBYTES - 4, &namever.name1, 4);
		memcpy_s(&rsp[8], CMDLEN_MAXBYTES - 8, &namever.version, 4);
		htif_hexdump(rsp, 12);
		fwreply(*hdr, FW_RSP_NAME_VERSION, rsp);
		// state unchanged
		return state;
	} break;

	case FW_CMD_GET_UDI: {
		uint32_t udi_words[2];

		htif_puts("cmd: get-udi\n");
		if (hdr->len != 1) {
			// Bad length
			return FW_STATE_FAIL;
		}

		rsp[0] = STATUS_OK;
		wordcpy_s(udi_words, 2, (void *)udi, 2);
		memcpy_s(&rsp[1], CMDLEN_MAXBYTES - 1, udi_words, 2 * 4);
		fwreply(*hdr, FW_RSP_GET_UDI, rsp);
		// state unchanged
		return state;
	} break;

	case FW_CMD_LOAD_APP: {
		uint32_t local_app_size;

		htif_puts("cmd: load-app(size, uss)\n");
		if (hdr->len != 512) {
			// Bad length
			return FW_STATE_FAIL;
		}

		// cmd[1..4] contains the size.
		local_app_size =
		    cmd[1] + (cmd[2] << 8) + (cmd[3] << 16) + (cmd[4] << 24);

		htif_puts("app size: ");
		htif_putinthex(local_app_size);
		htif_lf();

		if (local_app_size == 0 || local_app_size > TK1_APP_MAX_SIZE) {
			rsp[0] = STATUS_BAD;
			fwreply(*hdr, FW_RSP_LOAD_APP, rsp);

			return state;
		}

		*app_size = local_app_size;

		// Do we have a USS at all?
		if (cmd[5] != 0) {
			// Yes
			ctx->use_uss = TRUE;
			memcpy_s(ctx->uss, 32, &cmd[6], 32);
		} else {
			ctx->use_uss = FALSE;
		}

		rsp[0] = STATUS_OK;
		fwreply(*hdr, FW_RSP_LOAD_APP, rsp);

		assert(*app_size != 0);
		assert(*app_size <= TK1_APP_MAX_SIZE);

		ctx->left = *app_size;

		return FW_STATE_LOADING;
	} break;

	default:
		htif_puts("Got unknown firmware cmd: 0x");
		htif_puthex(cmd[0]);
		htif_lf();
	}

	return FW_STATE_FAIL;
}

static int loading_commands(const struct frame_header *hdr, const uint8_t *cmd,
			    enum state state, struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};
	int nbytes;

	switch (cmd[0]) {
	case FW_CMD_LOAD_APP_DATA:
		htif_puts("cmd: load-app-data\n");
		if (hdr->len != 512) {
			// Bad length
			return FW_STATE_FAIL;
		}

		if (ctx->left > (512 - 1)) {
			nbytes = 512 - 1;
		} else {
			nbytes = ctx->left;
		}
		memcpy_s(ctx->loadaddr, ctx->left, cmd + 1, nbytes);
		ctx->loadaddr += nbytes;
		ctx->left -= nbytes;

		if (ctx->left == 0) {
			htif_puts("Fully loaded ");
			htif_putinthex(*app_size);
			htif_lf();

			// Compute Blake2S digest of the app,
			// storing it for FW_STATE_RUN
			blake2s_ctx b2s_ctx;

			blake2s(&ctx->digest, 32, NULL, 0,
				(const void *)TK1_RAM_BASE, *app_size,
				&b2s_ctx);
			print_digest(ctx->digest);

			// And return the digest in final
			// response
			rsp[0] = STATUS_OK;
			memcpy_s(&rsp[1], CMDLEN_MAXBYTES - 1, &ctx->digest,
				 32);
			fwreply(*hdr, FW_RSP_LOAD_APP_DATA_READY, rsp);

			return FW_STATE_RUN;
		}

		rsp[0] = STATUS_OK;
		fwreply(*hdr, FW_RSP_LOAD_APP_DATA, rsp);
		break;

	default:
		htif_puts("Got unknown firmware cmd: 0x");
		htif_puthex(cmd[0]);
		htif_lf();
		return FW_STATE_FAIL;
	}

	return state;
}

static void run(const struct context *ctx)
{
	*app_addr = TK1_RAM_BASE;

	// CDI = hash(uds, hash(app), uss)
	compute_cdi(ctx->digest, ctx->use_uss, ctx->uss);

	htif_puts("Flipping to app mode!\n");
	htif_puts("Jumping to ");
	htif_putinthex(*app_addr);
	htif_lf();

	// Clear the firmware stack
	// clang-format off
	asm volatile(
		"li a0, 0xd0000000;" // FW_RAM
		"li a1, 0xd0000800;" // End of 2 KB FW_RAM (just past the end)
		"loop:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop;"
		::: "memory");
	// clang-format on

	// Flip over to application mode
	*switch_app = 1;

	// XXX Firmware stack now no longer available
	// Don't use any function calls!

	// Jump to app - doesn't return
	// clang-format off
	asm volatile(
		// Get value at TK1_MMIO_TK1_APP_ADDR
		"lui a0,0xff000;"
		"lw a0,0x030(a0);"
		// Jump to it
		"jalr x0,0(a0);"
		::: "memory");
	// clang-format on

	__builtin_unreachable();
}

void scramble_ram()
{
	uint32_t *ram = (uint32_t *)(TK1_RAM_BASE);
	uint32_t rnd = rnd_word();
	uint32_t rnd_incr = rnd_word();

	// Set RAM address and data scrambling values
	*ram_aslr = rnd_word();
	*ram_scramble = rnd_word();

	// Fill RAM with random data (FW does not use RAM, has its stack in
	// FW_RAM)
	for (uint32_t w = 0; w < TK1_RAM_SIZE / 4; w++) {
		ram[w] = rnd;
		rnd += rnd_incr;
	}

	// Set new scrambling values, for all use of RAM by app
	*ram_aslr = rnd_word();
	*ram_scramble = rnd_word();
}

int main()
{
	struct context ctx = {0};
	struct frame_header hdr; // Used in both directions
	uint8_t cmd[CMDLEN_MAXBYTES];
	enum state state = FW_STATE_INITIAL;

	// Let the app know the function adddress for blake2s()
	*fw_blake2s_addr = (uint32_t)blake2s;

	ctx.loadaddr = (uint8_t *)TK1_RAM_BASE;
	ctx.use_uss = FALSE;

	scramble_ram();

	for (;;) {
		switch (state) {
		case FW_STATE_INITIAL:
			if (readcommand(&hdr, cmd, state) == -1) {
				state = FW_STATE_FAIL;
				break;
			}

			state = initial_commands(&hdr, cmd, state, &ctx);
			break;

		case FW_STATE_LOADING:
			if (readcommand(&hdr, cmd, state) == -1) {
				state = FW_STATE_FAIL;
				break;
			}

			state = loading_commands(&hdr, cmd, state, &ctx);
			break;

		case FW_STATE_RUN:
			htif_puts("state_run\n");
			run(&ctx);
			break; // This is never reached!

		case FW_STATE_FAIL:
			// fallthrough
		default:
			htif_puts("firmware state 0x");
			htif_puthex(state);
			htif_lf();
			forever_redflash();
			break; // Not reached
		}
	}

	return (int)0xcafebabe;
}
