/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1_mem.h"
#include "assert.h"
#include "blake2s/blake2s.h"
#include "lib.h"
#include "proto.h"
#include "state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
static volatile uint32_t *ram_addr_rand   = (volatile uint32_t *)TK1_MMIO_TK1_RAM_ADDR_RAND;
static volatile uint32_t *ram_data_rand   = (volatile uint32_t *)TK1_MMIO_TK1_RAM_DATA_RAND;
// clang-format on

// Context for the loading of a TKey program
struct context {
	uint32_t left;	    // Bytes left to receive
	uint8_t digest[32]; // Program digest
	uint8_t *loadaddr;  // Where we are currently loading a TKey program
	bool use_uss;	    // Use USS?
	uint8_t uss[32];    // User Supplied Secret, if any
};

static void print_hw_version(void);
static void print_digest(uint8_t *md);
static uint32_t rnd_word(void);
static void compute_cdi(const uint8_t *digest, const bool use_uss,
			const uint8_t *uss);
static void copy_name(uint8_t *buf, const size_t bufsiz, const uint32_t word);
static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx);
static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx);
static void run(const struct context *ctx);
static uint32_t xorwow(uint32_t state, uint32_t acc);
static void scramble_ram(void);

static void print_hw_version(void)
{
	htif_puts("Hello, I'm firmware with");
	htif_puts(" tk1_name0:");
	htif_putinthex(*name0);
	htif_puts(" tk1_name1:");
	htif_putinthex(*name1);
	htif_puts(" tk1_version:");
	htif_putinthex(*ver);
	htif_lf();
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

static uint32_t rnd_word(void)
{
	while ((*trng_status & (1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
	}
	return *trng_entropy;
}

// CDI = blake2s(uds, blake2s(app), uss)
static void compute_cdi(const uint8_t *digest, const bool use_uss,
			const uint8_t *uss)
{
	uint32_t local_uds[8] = {0};
	uint32_t local_cdi[8] = {0};
	blake2s_ctx secure_ctx = {0};
	uint32_t rnd_sleep = 0;
	int blake2err = 0;

	// Prepare to sleep a random number of cycles before reading out UDS
	*timer_prescaler = 1;
	rnd_sleep = rnd_word();
	// Up to 65536 cycles
	rnd_sleep &= 0xffff;
	*timer = (uint32_t)(rnd_sleep == 0 ? 1 : rnd_sleep);
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);
	while (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
	}

	blake2err = blake2s_init(&secure_ctx, 32, NULL, 0);
	assert(blake2err == 0);

	// Update hash with UDS. This means UDS will live for a short
	// while on the firmware stack which is in the special fw_ram.
	wordcpy_s(local_uds, 8, (void *)uds, 8);
	blake2s_update(&secure_ctx, (const void *)local_uds, 32);
	(void)secure_wipe(local_uds, sizeof(local_uds));

	// Update with TKey program digest
	blake2s_update(&secure_ctx, digest, 32);

	// Possibly hash in the USS as well
	if (use_uss) {
		blake2s_update(&secure_ctx, uss, 32);
	}

	// Write hashed result to Compound Device Identity (CDI)
	blake2s_final(&secure_ctx, &local_cdi);

	// Clear secure_ctx of any residue of UDS. Don't want to keep
	// that for long even though fw_ram is cleared later.
	(void)secure_wipe(&secure_ctx, sizeof(secure_ctx));

	// CDI only word writable
	wordcpy_s((void *)cdi, 8, &local_cdi, 8);
}

static void copy_name(uint8_t *buf, const size_t bufsiz, const uint32_t word)
{
	assert(bufsiz >= 4);

	buf[0] = word >> 24;
	buf[1] = word >> 16;
	buf[2] = word >> 8;
	buf[3] = word;
}

static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};

	switch (cmd[0]) {
	case FW_CMD_NAME_VERSION:
		htif_puts("cmd: name-version\n");
		if (hdr->len != 1) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		copy_name(rsp, CMDLEN_MAXBYTES, *name0);
		copy_name(&rsp[4], CMDLEN_MAXBYTES - 4, *name1);
		wordcpy_s(&rsp[8], CMDLEN_MAXBYTES / 4 - 2, (void *)ver, 1);

		fwreply(*hdr, FW_RSP_NAME_VERSION, rsp);
		// still initial state
		break;

	case FW_CMD_GET_UDI: {
		uint32_t udi_words[2];

		htif_puts("cmd: get-udi\n");
		if (hdr->len != 1) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		rsp[0] = STATUS_OK;
		wordcpy_s(&udi_words, 2, (void *)udi, 2);
		memcpy_s(&rsp[1], CMDLEN_MAXBYTES - 1, &udi_words, 2 * 4);
		fwreply(*hdr, FW_RSP_GET_UDI, rsp);
		// still initial state
		break;
	}

	case FW_CMD_LOAD_APP: {
		uint32_t local_app_size;

		htif_puts("cmd: load-app(size, uss)\n");
		if (hdr->len != 128) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
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
			// still initial state
			break;
		}

		*app_size = local_app_size;

		// Do we have a USS at all?
		if (cmd[5] != 0) {
			// Yes
			ctx->use_uss = true;
			memcpy_s(ctx->uss, 32, &cmd[6], 32);
		} else {
			ctx->use_uss = false;
		}

		rsp[0] = STATUS_OK;
		fwreply(*hdr, FW_RSP_LOAD_APP, rsp);

		assert(*app_size != 0);
		assert(*app_size <= TK1_APP_MAX_SIZE);

		ctx->left = *app_size;

		state = FW_STATE_LOADING;
		break;
	}

	default:
		htif_puts("Got unknown firmware cmd: 0x");
		htif_puthex(cmd[0]);
		htif_lf();
		state = FW_STATE_FAIL;
		break;
	}

	return state;
}

static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};
	uint32_t nbytes = 0;

	switch (cmd[0]) {
	case FW_CMD_LOAD_APP_DATA:
		htif_puts("cmd: load-app-data\n");
		if (hdr->len != 128) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		if (ctx->left > (128 - 1)) {
			nbytes = 128 - 1;
		} else {
			nbytes = ctx->left;
		}
		memcpy_s(ctx->loadaddr, ctx->left, cmd + 1, nbytes);
		/*@-mustfreeonly@*/
		ctx->loadaddr += nbytes;
		/*@+mustfreeonly@*/
		ctx->left -= nbytes;

		if (ctx->left == 0) {
			blake2s_ctx b2s_ctx = {0};
			int blake2err = 0;

			htif_puts("Fully loaded ");
			htif_putinthex(*app_size);
			htif_lf();

			// Compute Blake2S digest of the app,
			// storing it for FW_STATE_RUN
			blake2err = blake2s(&ctx->digest, 32, NULL, 0,
					    (const void *)TK1_RAM_BASE,
					    *app_size, &b2s_ctx);
			assert(blake2err == 0);
			print_digest(ctx->digest);

			// And return the digest in final
			// response
			rsp[0] = STATUS_OK;
			memcpy_s(&rsp[1], CMDLEN_MAXBYTES - 1, &ctx->digest,
				 32);
			fwreply(*hdr, FW_RSP_LOAD_APP_DATA_READY, rsp);

			state = FW_STATE_RUN;
			break;
		}

		rsp[0] = STATUS_OK;
		fwreply(*hdr, FW_RSP_LOAD_APP_DATA, rsp);
		// still loading state
		break;

	default:
		htif_puts("Got unknown firmware cmd: 0x");
		htif_puthex(cmd[0]);
		htif_lf();
		state = FW_STATE_FAIL;
		break;
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
#ifndef S_SPLINT_S
	asm volatile(
		"li a0, 0xd0000000;" // FW_RAM
		"li a1, 0xd0000800;" // End of 2 KB FW_RAM (just past the end)
		"loop:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop;"
		::: "memory");
#endif
	// clang-format on

	// Flip over to application mode
	*switch_app = 1;

	// XXX Firmware stack now no longer available
	// Don't use any function calls!

	// Jump to app - doesn't return
	// clang-format off
#ifndef S_SPLINT_S
	asm volatile(
		// Get value at TK1_MMIO_TK1_APP_ADDR
		"lui a0,0xff000;"
		"lw a0,0x030(a0);"
		// Jump to it
		"jalr x0,0(a0);"
		::: "memory");
#endif
	// clang-format on

	__builtin_unreachable();
}

static uint32_t xorwow(uint32_t state, uint32_t acc)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	state += acc;
	return state;
}

static void scramble_ram(void)
{
	uint32_t *ram = (uint32_t *)(TK1_RAM_BASE);

	// Fill RAM with random data
	// Get random state and accumulator seeds.
	uint32_t data_state = rnd_word();
	uint32_t data_acc = rnd_word();

	for (uint32_t w = 0; w < TK1_RAM_SIZE / 4; w++) {
		data_state = xorwow(data_state, data_acc);
		ram[w] = data_state;
	}

	// Set RAM address and data scrambling parameters
	*ram_addr_rand = rnd_word();
	*ram_data_rand = rnd_word();
}

int main(void)
{
	struct context ctx = {0};
	struct frame_header hdr = {0};
	uint8_t cmd[CMDLEN_MAXBYTES] = {0};
	enum state state = FW_STATE_INITIAL;

	print_hw_version();

	// Let the app know the function adddress for blake2s()
	*fw_blake2s_addr = (uint32_t)blake2s;

	/*@-mustfreeonly@*/
	/* Yes, splint, this points directly to RAM and we don't care
	 * about freeing anything was pointing to 0x0 before.
	 */
	ctx.loadaddr = (uint8_t *)TK1_RAM_BASE;
	/*@+mustfreeonly@*/
	ctx.use_uss = false;

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
			run(&ctx);
			break; // This is never reached!

		case FW_STATE_FAIL:
			// fallthrough
		default:
			htif_puts("firmware state 0x");
			htif_puthex(state);
			htif_lf();
			assert(1 == 2);
			break; // Not reached
		}
	}

	/*@ -compdestroy @*/
	/* We don't care about memory leaks here. */
	return (int)0xcafebabe;
}
