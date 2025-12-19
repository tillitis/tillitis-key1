// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <blake2s/blake2s.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "mgmt_app.h"
#include "partition_table.h"
#include "preload_app.h"
#include "proto.h"
#include "reset.h"
#include "state.h"
#include "syscall_enable.h"

#define DOMAIN_USS_MASK 0x80
#define DOMAIN_MEASURED_ID_MASK 0x40

// clang-format off
static volatile uint32_t *uds              = (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
static volatile uint32_t *name0            = (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
static volatile uint32_t *name1            = (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
static volatile uint32_t *ver              = (volatile uint32_t *)TK1_MMIO_TK1_VERSION;
static volatile uint32_t *udi              = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
static volatile uint32_t *cdi              = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
static volatile uint32_t *app_addr         = (volatile uint32_t *)TK1_MMIO_TK1_APP_ADDR;
static volatile uint32_t *app_size         = (volatile uint32_t *)TK1_MMIO_TK1_APP_SIZE;
static volatile uint32_t *trng_status      = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
static volatile uint32_t *trng_entropy     = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
static volatile uint32_t *timer            = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler  = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_status     = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
static volatile uint32_t *timer_ctrl       = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
static volatile uint32_t *ram_addr_rand    = (volatile uint32_t *)TK1_MMIO_TK1_RAM_ADDR_RAND;
static volatile uint32_t *ram_data_rand    = (volatile uint32_t *)TK1_MMIO_TK1_RAM_DATA_RAND;
static volatile struct reset *resetinfo    = (volatile struct reset *)TK1_MMIO_RESETINFO_BASE;
// clang-format on

struct partition_table_storage part_table_storage;

enum app_source {
	APP_SRC_NONE = 0,
	APP_SRC_FLASH0 = 1,
	APP_SRC_FLASH1 = 2,
	APP_SRC_CLIENT = 3,
};

// Context for the loading of a TKey program
struct context {
	uint32_t left;	    // Bytes left to receive
	uint8_t digest[32]; // Program digest
	uint8_t *loadaddr;  // Where we are currently loading a TKey program
	bool use_uss;	    // Use USS?
	uint8_t uss[32];    // User Supplied Secret, if any
	uint8_t flash_slot; // App is loaded from flash slot number
	/*@null@*/ volatile uint8_t
	    *ver_digest;	 // Verify loaded app against this digest
	enum app_source app_src; // Where app was loaded from
};

static void print_hw_version(void);
static void print_digest(uint8_t *md);
static uint32_t rnd_word(void);
static void compute_cdi(uint8_t domain, const uint8_t *digest,
			const uint8_t use_uss, const uint8_t *uss);
static void copy_name(uint8_t *buf, const size_t bufsiz, const uint32_t word);
static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx);
static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx);
#if !defined(SIMULATION)
static uint32_t xorwow(uint32_t state, uint32_t acc);
#endif
static void scramble_ram(void);
static int compute_app_digest(uint8_t *digest);
static int load_flash_app(struct partition_table *part_table,
			  uint8_t digest[32], uint8_t slot);
static enum state start_where(struct context *ctx);

static void print_hw_version(void)
{
	debug_puts("Hello, I'm firmware with");
	debug_puts(" tk1_name0:");
	debug_putinthex(*name0);
	debug_puts(" tk1_name1:");
	debug_putinthex(*name1);
	debug_puts(" tk1_version:");
	debug_putinthex(*ver);
	debug_lf();
}

static void print_digest(uint8_t *md)
{
	debug_puts("The app digest:\n");
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 8; i++) {
			debug_puthex(md[i + 8 * j]);
			(void)md;
		}
		debug_lf();
	}
	debug_lf();
}

static uint32_t rnd_word(void)
{
	while ((*trng_status & (1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
	}
	return *trng_entropy;
}

// CDI = blake2s(uds, blake2s(app), uss)
static void compute_cdi(uint8_t domain, const uint8_t *digest,
			const uint8_t use_uss, const uint8_t *uss)
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

	// Update with domain
	blake2s_update(&secure_ctx, &domain, 1);

	// Update with TKey program digest
	blake2s_update(&secure_ctx, digest, 32);

	// Possibly hash in the USS as well
	if (use_uss != 0) {
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
	assert(buf != NULL);

	buf[0] = word >> 24;
	buf[1] = word >> 16;
	buf[2] = word >> 8;
	buf[3] = word;
}

static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDSIZE] = {0};

	switch (cmd[0]) {
	case FW_CMD_NAME_VERSION:
		debug_puts("cmd: name-version\n");
		if (hdr->len != 1) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		copy_name(rsp, CMDSIZE, *name0);
		copy_name(&rsp[4], CMDSIZE - 4, *name1);
		wordcpy_s(&rsp[8], CMDSIZE - 8, (void *)ver, 1);

		fwreply(*hdr, FW_RSP_NAME_VERSION, rsp);
		// still initial state
		break;

	case FW_CMD_GET_UDI: {
		uint32_t udi_words[2];

		debug_puts("cmd: get-udi\n");
		if (hdr->len != 1) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		rsp[0] = STATUS_OK;
		wordcpy_s(&udi_words, 2, (void *)udi, 2);
		memcpy_s(&rsp[1], CMDSIZE - 1, &udi_words, 2 * 4);
		fwreply(*hdr, FW_RSP_GET_UDI, rsp);
		// still initial state
		break;
	}

	case FW_CMD_LOAD_APP: {
		uint32_t local_app_size;

		debug_puts("cmd: load-app(size, uss)\n");
		if (hdr->len != 128) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		// cmd[1..4] contains the size.
		local_app_size =
		    cmd[1] + (cmd[2] << 8) + (cmd[3] << 16) + (cmd[4] << 24);

		debug_puts("app size: ");
		debug_putinthex(local_app_size);
		debug_lf();

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

		led_set(LED_BLACK);

		state = FW_STATE_LOADING;
		break;
	}

	default:
		debug_puts("Got unknown firmware cmd: 0x");
		debug_puthex(cmd[0]);
		debug_lf();
		state = FW_STATE_FAIL;
		break;
	}

	return state;
}

static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDSIZE] = {0};
	uint32_t nbytes = 0;

	switch (cmd[0]) {
	case FW_CMD_LOAD_APP_DATA:
		debug_puts("cmd: load-app-data\n");
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
			int blake2err = 0;

			debug_puts("Fully loaded ");
			debug_putinthex(*app_size);
			debug_lf();

			// Compute Blake2S digest of the app,
			// storing it for FW_STATE_RUN
			blake2err = compute_app_digest(ctx->digest);
			assert(blake2err == 0);
			print_digest(ctx->digest);

			// And return the digest in final
			// response
			rsp[0] = STATUS_OK;
			memcpy_s(&rsp[1], CMDSIZE - 1, &ctx->digest, 32);
			fwreply(*hdr, FW_RSP_LOAD_APP_DATA_READY, rsp);

			state = FW_STATE_START;
			break;
		}

		rsp[0] = STATUS_OK;
		fwreply(*hdr, FW_RSP_LOAD_APP_DATA, rsp);
		// still loading state
		break;

	default:
		debug_puts("Got unknown firmware cmd: 0x");
		debug_puthex(cmd[0]);
		debug_lf();
		state = FW_STATE_FAIL;
		break;
	}

	return state;
}

static void jump_to_app(void)
{
	/* Start of app is always at the beginning of RAM */
	*app_addr = TK1_RAM_BASE;

	debug_puts("Flipping to app mode!\n");
	debug_puts("Jumping to ");
	debug_putinthex(*app_addr);
	debug_lf();

	// Clear the firmware stack
	// clang-format off
#ifndef S_SPLINT_S
	asm volatile(
		"la a0, _sstack;"
		"la a1, _estack;"
		"loop:;"
		"sw zero, 0(a0);"
		"addi a0, a0, 4;"
		"blt a0, a1, loop;"
		::: "memory");
#endif
	// clang-format on

	syscall_enable();

	// Jump to app - doesn't return
	// Hardware is responsible for switching to app mode
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

static int load_flash_app(struct partition_table *part_table,
			  uint8_t digest[32], uint8_t slot)
{
	if (slot >= N_PRELOADED_APP) {
		return -1;
	}

	if (preload_load(part_table, slot) == -1) {
		return -1;
	}

	*app_size = part_table->pre_app_data[slot].size;
	if (*app_size > TK1_APP_MAX_SIZE) {
		return -1;
	}

	int digest_err = compute_app_digest(digest);
	assert(digest_err == 0);
	print_digest(digest);

	return 0;
}

#if !defined(SIMULATION)
static uint32_t xorwow(uint32_t state, uint32_t acc)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	state += acc;
	return state;
}
#endif

static void scramble_ram(void)
{
	// Can't fill RAM if we are simulating, data has already been loaded
	// into RAM.
#if !defined(SIMULATION)
	uint32_t *ram = (uint32_t *)(TK1_RAM_BASE);

	// Fill RAM with random data
	// Get random state and accumulator seeds.
	uint32_t data_state = rnd_word();
	uint32_t data_acc = rnd_word();

	for (uint32_t w = 0; w < TK1_RAM_SIZE / 4; w++) {
		data_state = xorwow(data_state, data_acc);
		ram[w] = data_state;
	}
#endif

	// Set RAM address and data scrambling parameters
	*ram_addr_rand = rnd_word();
	*ram_data_rand = rnd_word();
}

/* Computes the blake2s digest of the app loaded into RAM */
static int compute_app_digest(uint8_t *digest)
{
	return blake2s(digest, 32, NULL, 0, (const void *)TK1_RAM_BASE,
		       *app_size);
}

static enum state start_where(struct context *ctx)
{
	assert(ctx != NULL);

	debug_puts("resetinfo->type: ");
	debug_putinthex(resetinfo->type);
	debug_lf();

	debug_puts("  -> mask: ");
	debug_puthex(resetinfo->mask);
	debug_lf();

	debug_puts("  ->app_digest: \n");
	debug_hexdump((void *)resetinfo->app_digest, RESET_DIGEST_SIZE);
	debug_lf();

	debug_puts("  ->next_app_data: \n");
	debug_hexdump((void *)resetinfo->next_app_data, RESET_DATA_SIZE);
	debug_lf();

	// Where do we start?
	switch (resetinfo->type) {
	case START_DEFAULT:
		// fallthrough
	case START_FLASH0:
		ctx->flash_slot = 0;
		ctx->app_src = APP_SRC_FLASH0;
		ctx->ver_digest = mgmt_app_allowed_digest();

		return FW_STATE_LOAD_FLASH_MGMT;

	case START_FLASH1:
		ctx->flash_slot = 1;
		ctx->app_src = APP_SRC_FLASH1;
		ctx->ver_digest = NULL;

		return FW_STATE_LOAD_FLASH;

	case START_FLASH0_VER:
		ctx->flash_slot = 0;
		ctx->app_src = APP_SRC_FLASH0;
		ctx->ver_digest = resetinfo->app_digest;

		return FW_STATE_LOAD_FLASH;

	case START_FLASH1_VER:
		ctx->flash_slot = 1;
		ctx->app_src = APP_SRC_FLASH1;
		ctx->ver_digest = resetinfo->app_digest;

		return FW_STATE_LOAD_FLASH;

	case START_CLIENT:
		ctx->app_src = APP_SRC_CLIENT;
		ctx->ver_digest = NULL;

		return FW_STATE_WAITCOMMAND;

	case START_CLIENT_VER:
		ctx->app_src = APP_SRC_CLIENT;
		ctx->ver_digest = resetinfo->app_digest;

		return FW_STATE_WAITCOMMAND;

	default:
		debug_puts("Unknown startfrom\n");

		return FW_STATE_FAIL;
	}
}

int main(void)
{
	struct context ctx = {0};
	struct frame_header hdr = {0};
	uint8_t cmd[CMDSIZE] = {0};
	enum state state = FW_STATE_INITIAL;

	print_hw_version();

	/*@-mustfreeonly@*/
	/* Yes, splint, this points directly to RAM and we don't care
	 * about freeing anything was pointing to 0x0 before.
	 */
	ctx.loadaddr = (uint8_t *)TK1_RAM_BASE;
	/*@+mustfreeonly@*/
	ctx.use_uss = false;

	scramble_ram();

	if (part_table_read(&part_table_storage) != 0) {
		// Couldn't read partition table
		debug_puts("Couldn't read partition table\n");
		assert(1 == 2);
	}

	// Reset the USB controller to only enable the USB CDC
	// endpoint and the internal command channel.
	config_endpoints(IO_CDC | IO_CH552);

	led_set(LED_WHITE);

#if defined(SIMULATION)
	run(&ctx);
#endif

	for (;;) {
		switch (state) {
		case FW_STATE_INITIAL:
			state = start_where(&ctx);
			break;

		case FW_STATE_WAITCOMMAND:
			if (readcommand(&hdr, cmd, state) == -1) {
				state = FW_STATE_FAIL;
				break;
			}

			debug_puts("cmd: \n");
			debug_hexdump(cmd, hdr.len);

			state = initial_commands(&hdr, cmd, state, &ctx);
			break;

		case FW_STATE_LOADING:
			if (readcommand(&hdr, cmd, state) == -1) {
				state = FW_STATE_FAIL;
				break;
			}

			state = loading_commands(&hdr, cmd, state, &ctx);
			break;

		case FW_STATE_LOAD_FLASH:
			if (load_flash_app(&part_table_storage.table,
					   ctx.digest, ctx.flash_slot) < 0) {
				debug_puts("Couldn't load app from flash\n");
				state = FW_STATE_FAIL;
				break;
			}

			state = FW_STATE_START;
			break;

		case FW_STATE_LOAD_FLASH_MGMT:
			if (load_flash_app(&part_table_storage.table,
					   ctx.digest, ctx.flash_slot) < 0) {
				debug_puts("Couldn't load app from flash\n");
				state = FW_STATE_FAIL;
				break;
			}

			if (mgmt_app_init(ctx.digest) != 0) {
				state = FW_STATE_FAIL;
				break;
			}

			state = FW_STATE_START;
			break;

		case FW_STATE_START: {
			// CDI = hash(uds, domain, hash(app), uss)
			//
			// or, if RESET_SEED is set,
			//
			// CDI = hash(uds, domain, measured_id, uss)
			uint8_t domain = ctx.app_src;
			domain |= ctx.use_uss ? DOMAIN_USS_MASK : 0;

			if (resetinfo->mask & RESET_SEED) {
				domain |= DOMAIN_MEASURED_ID_MASK;
				compute_cdi(
				    domain,
				    (const uint8_t *)resetinfo->measured_id,
				    ctx.use_uss, ctx.uss);
			} else {
				compute_cdi(domain, ctx.digest, ctx.use_uss,
					    ctx.uss);
			}

			// Reset resetinfo to default. Leave
			// next_app_data intact, if any. We also leave
			// app_digest since it might be used in the digest
			// verification below.
			resetinfo->type = START_DEFAULT;
			resetinfo->mask &= ~RESET_SEED;
			(void)memset((void *)resetinfo->measured_id, 0,
				     RESET_DIGEST_SIZE);

			if (ctx.ver_digest != NULL) {
				print_digest(ctx.digest);
				if (!memeq(ctx.digest, (void *)ctx.ver_digest,
					   sizeof(ctx.digest))) {
					debug_puts("Digests do not match\n");
					state = FW_STATE_FAIL;
					break;
				}
			}

			(void)memset((void *)resetinfo->app_digest, 0,
				     RESET_DIGEST_SIZE);

			jump_to_app();
			break; // Not reached
		}

		case FW_STATE_FAIL:
			// fallthrough
		default:
			debug_puts("firmware state 0x");
			debug_puthex(state);
			debug_lf();
			assert(1 == 2);
			break; // Not reached
		}
	}

	/*@ -compdestroy @*/
	// We don't care about memory leaks here.

	return (int)0xcafebabe;
}
